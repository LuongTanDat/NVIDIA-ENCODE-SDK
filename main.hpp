#include <thread>
#include <chrono>
#include <atomic>
#include <fstream>
#include <iostream>
#include <memory>
#include <cuda.h>
#include "../Utils/NvCodecUtils.h"
#include "NvCodec/NvEncoder/NvEncoderCuda.h"
#include "NvCodec/NvEncoder/NvEncoderOutputInVidMemCuda.h"
#include "../Utils/Logger.h"
#include "../Utils/NvEncoderCLIOptions.h"
#include "opencv2/opencv.hpp"

// TODO: Minimize, optimize this
struct pipeline_data
{
    cv::Mat frame;
    std::string uuid;
    pipeline_data(){};
    pipeline_data(cv::Mat frame) : frame(frame){};
};

template <typename T>
class send_one_replaceable_object
{
    std::atomic<T *> a_ptr = {nullptr};

public:
    void send(T const &_obj)
    {
        T *new_ptr = new T;
        *new_ptr = _obj;
        // TODO: The `unique_ptr` prevents a scary memory leak, why?
        std::unique_ptr<T> old_ptr(a_ptr.exchange(new_ptr));
    }

    T receive()
    {
        std::unique_ptr<T> ptr;
        do
        {
            while (!a_ptr)
                std::this_thread::sleep_for(std::chrono::milliseconds(3));
            ptr.reset(a_ptr.exchange(nullptr));
        } while (!ptr);
        return *ptr;
    }
};

void ShowEncoderCapability()
{
    ck(cuInit(0));
    int nGpu = 0;
    ck(cuDeviceGetCount(&nGpu));
    std::cout << "Encoder Capability" << std::endl
              << std::endl;
    for (int iGpu = 0; iGpu < nGpu; iGpu++)
    {
        CUdevice cuDevice = 0;
        ck(cuDeviceGet(&cuDevice, iGpu));
        char szDeviceName[80];
        ck(cuDeviceGetName(szDeviceName, sizeof(szDeviceName), cuDevice));
        CUcontext cuContext = NULL;
        ck(cuCtxCreate(&cuContext, 0, cuDevice));
        NvEncoderCuda enc(cuContext, 1280, 720, NV_ENC_BUFFER_FORMAT_NV12);

        std::cout << "GPU " << iGpu << " - " << szDeviceName << std::endl
                  << std::endl;
        std::cout << "\tH264:\t\t"
                  << "  " << (enc.GetCapabilityValue(NV_ENC_CODEC_H264_GUID, NV_ENC_CAPS_SUPPORTED_RATECONTROL_MODES) ? "yes" : "no") << std::endl
                  << "\tH264_444:\t"
                  << "  " << (enc.GetCapabilityValue(NV_ENC_CODEC_H264_GUID, NV_ENC_CAPS_SUPPORT_YUV444_ENCODE) ? "yes" : "no") << std::endl
                  << "\tH264_ME:\t"
                  << "  " << (enc.GetCapabilityValue(NV_ENC_CODEC_H264_GUID, NV_ENC_CAPS_SUPPORT_MEONLY_MODE) ? "yes" : "no") << std::endl
                  << "\tH264_WxH:\t"
                  << "  " << (enc.GetCapabilityValue(NV_ENC_CODEC_H264_GUID, NV_ENC_CAPS_WIDTH_MAX)) << "*" << (enc.GetCapabilityValue(NV_ENC_CODEC_H264_GUID, NV_ENC_CAPS_HEIGHT_MAX)) << std::endl
                  << "\tHEVC:\t\t"
                  << "  " << (enc.GetCapabilityValue(NV_ENC_CODEC_HEVC_GUID, NV_ENC_CAPS_SUPPORTED_RATECONTROL_MODES) ? "yes" : "no") << std::endl
                  << "\tHEVC_Main10:\t"
                  << "  " << (enc.GetCapabilityValue(NV_ENC_CODEC_HEVC_GUID, NV_ENC_CAPS_SUPPORT_10BIT_ENCODE) ? "yes" : "no") << std::endl
                  << "\tHEVC_Lossless:\t"
                  << "  " << (enc.GetCapabilityValue(NV_ENC_CODEC_HEVC_GUID, NV_ENC_CAPS_SUPPORT_LOSSLESS_ENCODE) ? "yes" : "no") << std::endl
                  << "\tHEVC_SAO:\t"
                  << "  " << (enc.GetCapabilityValue(NV_ENC_CODEC_HEVC_GUID, NV_ENC_CAPS_SUPPORT_SAO) ? "yes" : "no") << std::endl
                  << "\tHEVC_444:\t"
                  << "  " << (enc.GetCapabilityValue(NV_ENC_CODEC_HEVC_GUID, NV_ENC_CAPS_SUPPORT_YUV444_ENCODE) ? "yes" : "no") << std::endl
                  << "\tHEVC_ME:\t"
                  << "  " << (enc.GetCapabilityValue(NV_ENC_CODEC_HEVC_GUID, NV_ENC_CAPS_SUPPORT_MEONLY_MODE) ? "yes" : "no") << std::endl
                  << "\tHEVC_WxH:\t"
                  << "  " << (enc.GetCapabilityValue(NV_ENC_CODEC_HEVC_GUID, NV_ENC_CAPS_WIDTH_MAX)) << "*" << (enc.GetCapabilityValue(NV_ENC_CODEC_HEVC_GUID, NV_ENC_CAPS_HEIGHT_MAX)) << std::endl;

        std::cout << std::endl;

        enc.DestroyEncoder();
        ck(cuCtxDestroy(cuContext));
    }
}
