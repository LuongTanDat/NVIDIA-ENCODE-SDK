#include "main.hpp"

simplelogger::Logger *logger = simplelogger::LoggerFactory::CreateConsoleLogger();

int main(int argc, char **argv)
{
    send_one_replaceable_object<pipeline_data> stream2show;
    ShowEncoderCapability();
    std::string szInFilePath = "./your_name.mp4";
    std::string szOutFilePath = "./EMoi.h264";
    NV_ENC_BUFFER_FORMAT eFormat = NV_ENC_BUFFER_FORMAT_IYUV;
    int iGpu = 0;
    bool stop_video = false;

    std::ifstream fpIn(szInFilePath, std::ifstream::in | std::ifstream::binary);
    if (!fpIn)
    {
        std::ostringstream err;
        err << "Unable to open input file: " << szInFilePath << std::endl;
        throw std::invalid_argument(err.str());
    }
    fpIn.close();

    cv::VideoCapture cap("filesrc location=" + szInFilePath + " ! decodebin ! autovideoconvert ! appsink", cv::CAP_GSTREAMER);
    if (!cap.isOpened())
    {
        std::cout << "Error opening video stream or file" << std::endl;
        stop_video = false;
    }
    else
    {
        stop_video = true;
    }

    std::thread retrieve_frame_thead(
        [&]()
        {
            cv::Mat frame;
            while (stop_video)
            {
                bool state = cap.read(frame);
                if (!state)
                {
                    stop_video = false;
                    break;
                }
                pipeline_data p_data(frame);
                stream2show.send(p_data);
            }
        });

    int nHeight = static_cast<int>(cap.get(cv::CAP_PROP_FRAME_HEIGHT));
    int nWidth = static_cast<int>(cap.get(cv::CAP_PROP_FRAME_WIDTH));

    try
    {
        // InitParam
        NvEncoderInitParam encodeCLIOptions{""};
        int cuStreamType = -1;
        bool bOutputInVideoMem = false;

        // GPU-0
        ck(cuInit(0));
        int nGpu = 0;
        ck(cuDeviceGetCount(&nGpu));
        if (iGpu < 0 || iGpu >= nGpu)
        {
            std::cout << "GPU ordinal out of range. Should be within [" << 0 << ", " << nGpu - 1 << "]" << std::endl;
            return 1;
        }
        CUdevice cuDevice = 0;
        ck(cuDeviceGet(&cuDevice, iGpu));
        char szDeviceName[80];
        ck(cuDeviceGetName(szDeviceName, sizeof(szDeviceName), cuDevice));
        std::cout << "GPU in use: " << szDeviceName << std::endl;
        CUcontext cuContext = NULL;
        ck(cuCtxCreate(&cuContext, 0, cuDevice));

        // Open output file
        std::ofstream fpOut(szOutFilePath, std::ios::out | std::ios::binary);
        if (!fpOut)
        {
            std::ostringstream err;
            err << "Unable to open output file: " << szOutFilePath << std::endl;
            throw std::invalid_argument(err.str());
        }

        // EncodeCuda
        std::unique_ptr<NvEncoderCuda> pEnc(new NvEncoderCuda(cuContext, nWidth, nHeight, eFormat));
        NV_ENC_INITIALIZE_PARAMS initializeParams = {NV_ENC_INITIALIZE_PARAMS_VER};
        NV_ENC_CONFIG encodeConfig = {NV_ENC_CONFIG_VER};
        initializeParams.encodeConfig = &encodeConfig;
        pEnc->CreateDefaultEncoderParams(&initializeParams, encodeCLIOptions.GetEncodeGUID(), encodeCLIOptions.GetPresetGUID(), encodeCLIOptions.GetTuningInfo());
        encodeCLIOptions.SetInitParams(&initializeParams, eFormat);
        pEnc->CreateEncoder(&initializeParams);
        int nFrameSize = pEnc->GetFrameSize();
        std::unique_ptr<uint8_t[]> pHostFrame(new uint8_t[nFrameSize]);
        std::vector<std::vector<uint8_t>> vPacket;
        int count = 0;

        while (stop_video)
        {
            pipeline_data pdata = stream2show.receive();
            // cap >> frame;
            // if (frame.empty())
            // break;

            cv::Mat mat_yuv;
            cv::cvtColor(pdata.frame, mat_yuv, cv::COLOR_BGR2YUV_IYUV);
            memcpy(static_cast<void *>(pHostFrame.get()), static_cast<void *>(mat_yuv.data), nFrameSize);

            // For receiving encoded packets
            const NvEncInputFrame *encoderInputFrame = pEnc->GetNextInputFrame();
            NvEncoderCuda::CopyToDeviceFrame(cuContext, pHostFrame.get(), 0, (CUdeviceptr)encoderInputFrame->inputPtr,
                                             (int)encoderInputFrame->pitch,
                                             pEnc->GetEncodeWidth(),
                                             pEnc->GetEncodeHeight(),
                                             CU_MEMORYTYPE_HOST,
                                             encoderInputFrame->bufferFormat,
                                             encoderInputFrame->chromaOffsets,
                                             encoderInputFrame->numChromaPlanes);
            pEnc->EncodeFrame(vPacket);
            for (std::vector<uint8_t> &packet : vPacket)
            {
                // For each encoded packet
                fpOut.write(reinterpret_cast<char *>(packet.data()), packet.size());
            }
            cv::imshow("Frame", pdata.frame);
            std::cout << "count\t" << count++ << std::endl;
            char c = (char)cv::waitKey(1);
            if (c == 27)
                break;
        }
        // pEnc->EndEncode(vPacket);
    }
    catch (const std::exception &ex)
    {
        std::cout << ex.what();
        return 1;
    }

    stop_video = false;
    if (retrieve_frame_thead.joinable())
        retrieve_frame_thead.join();
    return 0;
}
