/**
 * ============================================================================
 *
 * Copyright (C) 2018, Hisilicon Technologies Co., Ltd. All Rights Reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 *   1 Redistributions of source code must retain the above copyright notice,
 *     this list of conditions and the following disclaimer.
 *
 *   2 Redistributions in binary form must reproduce the above copyright notice,
 *     this list of conditions and the following disclaimer in the documentation
 *     and/or other materials provided with the distribution.
 *
 *   3 Neither the names of the copyright holders nor the names of the
 *   contributors may be used to endorse or promote products derived from this
 *   software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 * ============================================================================
 */

#include "ascenddk/ascend_ezdvpp/dvpp_utils.h"
#include <malloc.h>

namespace ascend {
namespace utils {

DvppUtils::DvppUtils() {
  // TODO Auto-generated constructor stub

}

DvppUtils::~DvppUtils() {
  // TODO Auto-generated destructor stub
}

int DvppUtils::CheckBgrToYuvParam(const char *input_buf, int input_size,
                                  int output_size, unsigned char *output_buf) {
  // null pointer and extreme value check
  if (input_buf == nullptr || output_buf == nullptr || input_size <= 0
      || output_size <= 0) {
    ASC_LOG_ERROR("Input param and output param can not be null!");
    return kDvppErrorInvalidParameter;
  }

  return kDvppOperationOk;
}

int DvppUtils::CheckCropOrResizeParam(const char *input_buf, int input_size,
                                      int output_size,
                                      unsigned char *output_buf) {
  // null pointer and extreme value check
  if (input_buf == nullptr || output_buf == nullptr || input_size <= 0
      || output_size <= 0) {
    ASC_LOG_ERROR(
        "Crop or resize input param and output param can not be null!");
    return kDvppErrorInvalidParameter;
  }

  return kDvppOperationOk;
}

int DvppUtils::CheckJpegChangeToYuvParam(const char *input_buf, int input_size,
                                         jpegd_yuv_data_info *output_data) {
  // null pointer and extreme value check
  if (input_buf == nullptr || input_size <= 0 || output_data == nullptr) {
    ASC_LOG_ERROR(
        "Jpeg change to yuv input param or output param can not be null!");
    return kDvppErrorInvalidParameter;
  }

  return kDvppOperationOk;
}

int DvppUtils::CheckDataSize(int data_size) {
  if (data_size <= 0 || data_size > kAllowedMaxImageMemory) {
    ASC_LOG_ERROR(
        "To prevent excessive memory, data size should be in (0, 64]M! "
        "Now data size is %d byte.",
        data_size);
    return kDvppErrorCheckMemorySizeFail;
  }

  return kDvppOperationOk;
}

int DvppUtils::CheckIncreaseParam(double hinc, double vinc) {
  // extreme value check
  if (hinc < kMinIncrease || hinc > kMaxIncrease || vinc < kMinIncrease
      || vinc > kMaxIncrease) {
    ASC_LOG_ERROR(
        "resize hinc param must be [0.03125, 1) or (1, 4] and "
        "vinc param must be [0.03125, 1) or (1, 4]!");
    return kDvppErrorInvalidParameter;
  }
  return kDvppOperationOk;
}

int DvppUtils::CheckImageNeedAlign(int width, int high) {
  // check width and height whether need to align
  if ((width % kVpcWidthAlign) == 0 && (high % kVpcHeightAlign) == 0) {
    return kImageNotNeedAlign;
  }

  return kImageNeedAlign;
}

int DvppUtils::AllocBuffer(const char * src_data, int input_size,
                           bool is_input_align, int format, int width, int high,
                           int &width_stride, int &buffer_size,
                           char **dest_data) {
  // height of image need 16-byte alignment
  int align_high = ALIGN_UP(high, kVpcHeightAlign);
  int ret = kDvppOperationOk;

  switch (format) {
    case kVpcYuv420SemiPlannar: {
      // width of image need 128-byte alignment
      int align_width = ALIGN_UP(width, kVpcWidthAlign);
      width_stride = align_width;

      // The memory size of yuv420 image is 1.5 times width * height
      buffer_size = align_width * align_high * DVPP_YUV420SP_SIZE_MOLECULE
          / DVPP_YUV420SP_SIZE_DENOMINATOR;

      // input data address 128 byte alignment
      *dest_data = (char *) memalign(kVpcAddressAlign, buffer_size);
      CHECK_NEW_RESULT(dest_data);

      // alloc yuv420sp buffer
      ret = AllocYuv420SPBuffer(src_data, input_size, is_input_align, width,
                                align_width, high, align_high, buffer_size,
                                *dest_data);
      break;
    }
    case kVpcYuv422SemiPlannar: {
      // width of image need 128-byte alignment
      int align_width = ALIGN_UP(width, kVpcWidthAlign);
      width_stride = align_width;

      // The memory size of yuv422 image is 2 times width * height
      buffer_size = align_width * align_high * kYuv422SPWidthMul;

      // input data address 128 byte alignment
      *dest_data = (char *) memalign(kVpcAddressAlign, buffer_size);
      CHECK_NEW_RESULT(dest_data);

      // alloc yuv422sp buffer
      ret = AllocYuv422SPBuffer(src_data, input_size, is_input_align, width,
                                align_width, high, align_high, buffer_size,
                                *dest_data);
      break;
    }
    case kVpcYuv444SemiPlannar: {
      // y channel width of yuv444sp equals to image width and need 128-byte
      // alignment
      int y_align_width = ALIGN_UP(width, kVpcWidthAlign);
      width_stride = y_align_width;

      // uv channel width of yuv444sp is 2 times image width and need 128-byte
      // alignment
      int uv_align_width = ALIGN_UP(width * kYuv444SPWidthMul, kVpcWidthAlign);

      // memory size of yuv444sp = memory size of y channel + memory size of uv
      // channel
      buffer_size = y_align_width * align_high + uv_align_width * align_high;

      // input data address 128 byte alignment
      *dest_data = (char *) memalign(kVpcAddressAlign, buffer_size);
      CHECK_NEW_RESULT(dest_data);

      // alloc yuv444sp buffer
      ret = AllocYuv444SPBuffer(src_data, input_size, is_input_align, width,
                                y_align_width, uv_align_width, high, align_high,
                                buffer_size, *dest_data);
      break;
    }
    case kVpcYuv422Packed: {
      //  The memory size of each row in yuv422 packed is 2 times width of image
      int yuv422_packed_width = width * kYuv422PackedWidthMul;

      // The memory size of each row need 128-byte alignment
      int align_width = ALIGN_UP(yuv422_packed_width, kVpcWidthAlign);
      width_stride = align_width;

      // memory size of yuv422 packed
      buffer_size = align_width * align_high;

      // input data address 128 byte alignment
      *dest_data = (char *) memalign(kVpcAddressAlign, buffer_size);
      CHECK_NEW_RESULT(dest_data);

      // alloc yuv422 packed buffer
      ret = AllocYuvOrRgbPackedBuffer(src_data, input_size, is_input_align,
                                      yuv422_packed_width, align_width, high,
                                      align_high, buffer_size, *dest_data);
      break;
    }
    case kVpcYuv444Packed: {
      // The memory size of each row in yuv444 packed is 3 times width of image
      int yuv444_packed_width = width * kYuv444PackedWidthMul;

      // The memory size of each row need 128-byte alignment
      int align_width = ALIGN_UP(yuv444_packed_width, kVpcWidthAlign);
      width_stride = align_width;

      // memory size of yuv444 packed
      buffer_size = align_width * align_high;

      // input data address 128 byte alignment
      *dest_data = (char *) memalign(kVpcAddressAlign, buffer_size);
      CHECK_NEW_RESULT(dest_data);

      // alloc yuv444 packed buffer
      ret = AllocYuvOrRgbPackedBuffer(src_data, input_size, is_input_align,
                                      yuv444_packed_width, align_width, high,
                                      align_high, buffer_size, *dest_data);
      break;
    }
    case kVpcRgb888Packed: {
      // The memory size of each row in rgb888 packed is 3 times width of image
      int rgb888_width = width * kRgb888WidthMul;

      // The memory size of each row need 128-byte alignment
      int align_width = ALIGN_UP(rgb888_width, kVpcWidthAlign);
      width_stride = align_width;

      // memory size of rgb888 packed
      buffer_size = align_width * align_high;

      // input data address 128 byte alignment
      *dest_data = (char *) memalign(kVpcAddressAlign, buffer_size);
      CHECK_NEW_RESULT(dest_data);

      // alloc rgb888 packed buffer
      ret = AllocYuvOrRgbPackedBuffer(src_data, input_size, is_input_align,
                                      rgb888_width, align_width, high,
                                      align_high, buffer_size, *dest_data);
      break;
    }
    case kVpcXrgb8888Packed: {
      // The memory size of each row in xrgb8888 packed is 4 times width of
      // image
      int xrgb8888_width = width * kXrgb888WidthMul;

      // The memory size of each row need 128-byte alignment
      int align_width = ALIGN_UP(xrgb8888_width, kVpcWidthAlign);
      width_stride = align_width;

      // memory size of xrgb8888 packed
      buffer_size = align_width * align_high;

      // input data address 128 byte alignment
      *dest_data = (char *) memalign(kVpcAddressAlign, buffer_size);
      CHECK_NEW_RESULT(dest_data);

      // alloc xrgb8888 packed buffer
      ret = AllocYuvOrRgbPackedBuffer(src_data, input_size, is_input_align,
                                      xrgb8888_width, align_width, high,
                                      align_high, buffer_size, *dest_data);
      break;
    }
    case kVpcYuv400SemiPlannar: {

      // The memory size of each row need 128-byte alignment
      int align_width = ALIGN_UP(width, kVpcWidthAlign);
      width_stride = align_width;

      // yuv400sp image apply for the same size space as yuv420sp image
      buffer_size = align_width * align_high * DVPP_YUV420SP_SIZE_MOLECULE
          / DVPP_YUV420SP_SIZE_DENOMINATOR;

      // input data address 128 byte alignment
      *dest_data = (char *) memalign(kVpcAddressAlign, buffer_size);
      CHECK_NEW_RESULT(dest_data);

      // alloc yuv400sp buffer
      ret = AllocYuv420SPBuffer(src_data, input_size, is_input_align, width,
                                align_width, high, align_high, buffer_size,
                                *dest_data);
      break;
    }
    default: {
      ASC_LOG_ERROR(
          "The current image format is not supported, " "so space cannot be allocated!");
      ret = kDvppErrorInvalidParameter;
      break;
    }
  }

  if (ret == kDvppOperationOk && *dest_data == nullptr) {
    ASC_LOG_ERROR("alloc dest_data memory failed!");
    ret = kDvppErrorMallocFail;
  }

  return ret;
}

int DvppUtils::AllocYuv420SPBuffer(const char * src_data, int input_size,
                                   bool is_input_align, int width,
                                   int align_width, int high, int align_high,
                                   int buffer_size, char * dest_data) {
  int ret = EOK;

  // If the input image is aligned , directly copy all memory.
  if ((width == align_width && high == align_high) || is_input_align) {
    ret = memcpy_s(dest_data, buffer_size, src_data, input_size);
    CHECK_CROP_RESIZE_MEMCPY_RESULT(ret, dest_data);
  } else {      // If image is not aligned, memory copy from line to line.
    int remain_buffer_size = buffer_size;

    // y channel data copy
    for (int i = 0; i < high; ++i) {
      ret = memcpy_s(dest_data + ((ptrdiff_t) i * align_width),
                     remain_buffer_size, src_data, width);
      CHECK_CROP_RESIZE_MEMCPY_RESULT(ret, dest_data);
      remain_buffer_size -= align_width;
      src_data += width;
    }

    dest_data += (ptrdiff_t) (align_high - high) * align_width;
    remain_buffer_size -= (align_high - high) * align_width;

    // uv channel data copy
    for (int j = high; j < high + high / 2; ++j) {
      ret = memcpy_s(dest_data + ((ptrdiff_t) j * align_width),
                     remain_buffer_size, src_data, width);
      CHECK_CROP_RESIZE_MEMCPY_RESULT(ret, dest_data);
      remain_buffer_size -= align_width;
      src_data += width;
    }
  }

  return kDvppOperationOk;
}

int DvppUtils::AllocYuv422SPBuffer(const char * src_data, int input_size,
                                   bool is_input_align, int width,
                                   int align_width, int high, int align_high,
                                   int buffer_size, char * dest_data) {
  int ret = EOK;

  // If the input image is aligned , directly copy all memory.
  if ((width == align_width && high == align_high) || is_input_align) {
    ret = memcpy_s(dest_data, buffer_size, src_data, input_size);
    CHECK_CROP_RESIZE_MEMCPY_RESULT(ret, dest_data);
  } else {      // If image is not aligned, memory copy from line to line.
    int remain_buffer_size = buffer_size;

    // y channel data copy
    for (int i = 0; i < high; ++i) {
      ret = memcpy_s(dest_data + ((ptrdiff_t) i * align_width),
                     remain_buffer_size, src_data, width);
      CHECK_CROP_RESIZE_MEMCPY_RESULT(ret, dest_data);
      remain_buffer_size -= align_width;
      src_data += width;
    }

    dest_data += (ptrdiff_t) (align_high - high) * align_width;
    remain_buffer_size -= (align_high - high) * align_width;

    // uv channel data copy
    for (int j = high; j < high * 2; ++j) {
      ret = memcpy_s(dest_data + ((ptrdiff_t) j * align_width),
                     remain_buffer_size, src_data, width);
      CHECK_CROP_RESIZE_MEMCPY_RESULT(ret, dest_data);
      remain_buffer_size -= align_width;
      src_data += width;
    }
  }
  return kDvppOperationOk;
}

int DvppUtils::AllocYuv444SPBuffer(const char * src_data, int input_size,
                                   bool is_input_align, int width,
                                   int y_align_width, int uv_align_width,
                                   int high, int align_high, int buffer_size,
                                   char * dest_data) {
  int ret = EOK;

  // If the input image is aligned , directly copy all memory.
  if ((width == y_align_width && high == align_high) || is_input_align) {
    ret = memcpy_s(dest_data, buffer_size, src_data, input_size);
    CHECK_CROP_RESIZE_MEMCPY_RESULT(ret, dest_data);
  } else {      // If image is not aligned, memory copy from line to line.
    int remain_buffer_size = buffer_size;

    // y channel data copy
    for (int i = 0; i < high; ++i) {
      ret = memcpy_s(dest_data + ((ptrdiff_t) i * y_align_width),
                     remain_buffer_size, src_data, width);
      CHECK_CROP_RESIZE_MEMCPY_RESULT(ret, dest_data);
      remain_buffer_size -= y_align_width;
      src_data += width;
    }

    dest_data += (ptrdiff_t) align_high * y_align_width;
    remain_buffer_size -= (align_high - high) * y_align_width;

    // uv channel data copy
    for (int j = high; j < high * 2; ++j) {
      ret = memcpy_s(dest_data + ((ptrdiff_t) (j - high) * uv_align_width),
                     remain_buffer_size, src_data, width * kYuv444SPWidthMul);
      CHECK_CROP_RESIZE_MEMCPY_RESULT(ret, dest_data);
      remain_buffer_size -= uv_align_width;
      src_data += width * kYuv444SPWidthMul;
    }
  }
  return kDvppOperationOk;
}

int DvppUtils::AllocYuvOrRgbPackedBuffer(const char * src_data, int input_size,
                                         bool is_input_align, int width,
                                         int align_width, int high,
                                         int align_high, int buffer_size,
                                         char * dest_data) {
  int ret = EOK;

  // If the input image is aligned , directly copy all memory.
  if ((width == align_width && high == align_high) || is_input_align) {
    ret = memcpy_s(dest_data, buffer_size, src_data, input_size);
    CHECK_CROP_RESIZE_MEMCPY_RESULT(ret, dest_data);
  } else {      // If image is not aligned, memory copy from line to line.
    int remain_buffer_size = buffer_size;

    // y channel and uv channel data copy
    for (int i = 0; i < high; ++i) {
      ret = memcpy_s(dest_data + ((ptrdiff_t) i * align_width),
                     remain_buffer_size, src_data, width);
      CHECK_CROP_RESIZE_MEMCPY_RESULT(ret, dest_data);
      remain_buffer_size -= align_width;
      src_data += width;
    }
  }
  return kDvppOperationOk;
}
} /* namespace utils */
} /* namespace ascend */
