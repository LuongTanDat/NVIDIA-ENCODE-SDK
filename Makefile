# NVIDIA ENCODER
include common.mk

NVCCFLAGS := $(CCFLAGS)
# CCFLAGS += -fPIC -Wall -Wextra -O3 -g 

LDFLAGS += -L$(CUDA_PATH)/lib64 -lcudart -lnvcuvid -pthread -lnvidia-encode
LDFLAGS += $(shell pkg-config --libs libavcodec libavutil libavformat opencv)
INCLUDES += $(shell pkg-config --cflags libavcodec libavutil libavformat opencv)

# Target rules
all: build

build: app

NvEncoder.o: NvCodec/NvEncoder/NvEncoder.cpp \
				NvCodec/NvEncoder/NvEncoder.h
	$(GCC) $(CCFLAGS) $(INCLUDES) -o $@ -c $<

NvEncoderCuda.o: NvCodec/NvEncoder/NvEncoderCuda.cpp \
					NvCodec/NvEncoder/NvEncoderCuda.h \
                	NvCodec/NvEncoder/NvEncoder.h
	$(GCC) $(CCFLAGS) $(INCLUDES) -o $@ -c $<

NvEncoderOutputInVidMemCuda.o: NvCodec/NvEncoder/NvEncoderOutputInVidMemCuda.cpp \
								NvCodec/NvEncoder/NvEncoderOutputInVidMemCuda.h \
								NvCodec/NvEncoder/NvEncoder.h \
								NvCodec/NvEncoder/NvEncoderCuda.h
	$(GCC) $(CCFLAGS) $(INCLUDES) -o $@ -c $<

crc.o: Utils/crc.cu
	$(NVCC) $(NVCCFLAGS) $(INCLUDES) -o $@ -c $<

ColorSpace.o: Utils/ColorSpace.cu
	$(NVCC) $(NVCCFLAGS) $(INCLUDES) -o $@ -c $<

main.o: main.cpp main.hpp \
		NvCodec/NvEncoder/NvEncoder.h \
		NvCodec/NvEncoder/NvEncoderOutputInVidMemCuda.h \
		NvCodec/NvEncoder/NvEncoderCuda.h \
		Utils/NvCodecUtils.h \
		Utils/NvEncoderCLIOptions.h \
		Utils/Logger.h \
		Utils/NvCodecUtils.h \
		Utils/FFmpegDemuxer.h \
		Utils/Logger.h \
		Common/AppDecUtils.h
	$(GCC) $(CCFLAGS) $(INCLUDES) -o $@ -c $<

app: main.o \
		NvEncoder.o \
		NvEncoderCuda.o \
		NvEncoderOutputInVidMemCuda.o \
		ColorSpace.o \
		crc.o
	$(GCC) $(CCFLAGS) -o $@ $^ $(LDFLAGS)

clean:
	rm -rf app \
			AppEncCuda \
			AppEncCuda.o \
			NvEncoder.o \
                        NvDecoder.o \
			ColorSpace.o \
			NvEncoderCuda.o \
			NvEncoderOutputInVidMemCuda.o \
			crc.o \
			main.o
