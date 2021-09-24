# NVIDIA-ENCODE-SDK

The NVIDIA video encoder API is designed to accept raw video frames (in YUV or RGB format) and output the H.264 or HEVC bitstream

## Step

1. Initialize the encoder.

2. Set up the desired encoding parameters.

3. Allocate input/output buffers.

4. Copy frames to input buffers and read bitstream from the output buffers. This can be done synchronously (Windows & Linux) or asynchronously (Windows 7 and above only).

5. Clean-up - release all allocated input/output buffers.

6. Close the encoding session.

## Note

- `Bitstream`: data found in a stream of bits used in digital communication or data storage application.

- Before creating encoder, we should query capabilities of GPU to ensure that required functional is supported.


