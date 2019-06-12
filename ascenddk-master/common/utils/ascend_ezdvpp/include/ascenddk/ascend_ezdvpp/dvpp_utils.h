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

#ifndef ASCENDDK_ASCEND_EZDVPP_DVPP_UTILS_H_
#define ASCENDDK_ASCEND_EZDVPP_DVPP_UTILS_H_

#include "dvpp_data_type.h"
#include "toolchain/slog.h"

#define CHECK_MEMCPY_RESULT(ret, buffer) \
if (ret != EOK) { \
    ASC_LOG_ERROR("Failed to copy memory,Ret=%d.", ret); \
    unsigned char *buf = buffer; \
    if (buf != nullptr){ \
        delete[] buf; \
    } \
    return kDvppErrorMemcpyFail; \
}

#define CHECK_CROP_RESIZE_MEMCPY_RESULT(ret, buffer) \
if (ret != EOK) { \
    ASC_LOG_ERROR("Failed to copy memory,Ret=%d.", ret); \
    char *buf = buffer; \
    if (buf != nullptr){ \
        free(buf); \
    } \
    return kDvppErrorMemcpyFail; \
}

#define CHECK_VPC_MEMCPY_S_RESULT(err_ret, in_buffer, p_dvpp_api) \
if (err_ret != EOK) { \
    ASC_LOG_ERROR("Failed to copy memory,Ret=%d.", err_ret); \
    if (in_buffer != nullptr) \
    { \
        free(in_buffer); \
    } \
    IDVPPAPI *dvpp_api = p_dvpp_api; \
    if (p_dvpp_api != nullptr) \
    { \
        DestroyDvppApi(dvpp_api); \
    } \
    return kDvppErrorMemcpyFail; \
}

#define CHECK_NEW_RESULT(buffer) \
if (buffer == nullptr) { \
    ASC_LOG_ERROR("Failed to new memory."); \
    return kDvppErrorNewFail; \
}

#define ASC_LOG_ERROR(fmt, ...) \
dlog_error(ASCENDDK, "[%s:%d] " fmt "\n", __FILE__, __LINE__, ##__VA_ARGS__)

namespace ascend {
namespace utils {

class DvppUtils {
 public:
  DvppUtils();
  virtual ~DvppUtils();

  /**
   * @brief check dvppBgrChangeToYuv function parameter
   * @param [in] input_buf:input image data
   *             (dvpp need char *,so pInputBuf do not use const)
   * @param [in] input_size: input image data size
   * @param [in] output_size: output image data size
   * @param [in] output_buf: image data after conversion
   * @return enum DvppErrorCode
   */
  int CheckBgrToYuvParam(const char *input_buf, int input_size, int output_size,
                         unsigned char *output_buf);

  /**
   * @brief check DvppCropOrResize function parameter
   * @param [in] input_buf:input image data
   *             (dvpp need char *,so pInputBuf do not use const)
   * @param [in] input_size: input image data size
   * @param [in] output_size: output image data size
   * @param [in] output_buf: image data after conversion
   * @return enum DvppErrorCode
   */
  int CheckCropOrResizeParam(const char *input_buf, int input_size,
                             int output_size, unsigned char *output_buf);

  /**
   * @brief check DvppJpegChangeToYuv function parameter
   * @param [in] input_buf:input image data
   *             (dvpp need char *,so pInputBuf do not use const)
   * @param [in] input_size: input image data size
   * @param [in] output_data: image data after conversion
   * @return enum DvppErrorCode
   */
  int CheckJpegChangeToYuvParam(const char *input_buf, int input_size,
                                jpegd_yuv_data_info *output_data);

  /**
   * @brief check data size
   * @param [in] data_size: input or output data size
   * @return enum DvppErrorCode
   */
  int CheckDataSize(int data_size);

  /**
   * @brief check resize increase parameter
   * @param [in] hinc: Horizontal magnification
   * @param [in] vinc: Vertical magnification
   * @return enum DvppErrorCode
   */
  int CheckIncreaseParam(double hinc, double vinc);

  /**
   * @brief check whether the image needs alignment
   * @param [in] width: input image width
   * @param [in] high: input image high
   * @return IMAGE_NEED_ALIGN: image need align
   *         IMAGE_NOT_NEED_ALIGN: image don't need align
   */
  int CheckImageNeedAlign(int width, int high);

  /**
   * @brief alloc buffer for vpc interface
   * @param [in] src_data: source image data
   * @param [in] input_size: source image data size
   * @param [in] is_input_align: true: input image aligned;
   *                             false: input image not aligned
   * @param [in] format: input image format
   * @param [in] width: image width
   * @param [in] high: image high
   * @param [out] width_stride: image stride in width direction
   * @param [out] buffer_size: image data size after align
   * @param [out] dest_data: image data after align
   * @return enum DvppErrorCode
   */
  int AllocBuffer(const char * src_data, int input_size, bool is_input_align,
                  int format, int width, int high, int &width_stride,
                  int &buffer_size, char **dest_data);

  /**
   * @brief alloc buffer for yuv420_sp image
   * @param [in] src_data: source image data
   * @param [in] input_size: source image data size
   * @param [in] is_input_align: true: input image aligned;
   *                             false: input image not aligned
   * @param [in] format: input image format
   * @param [in] width: image width
   * @param [in] align_width: image width after align
   * @param [in] high: image high
   * @param [in] align_high: image high after align
   * @param [out] buffer_size: image data size after align
   * @param [out] dest_data: image data after align
   * @return enum DvppErrorCode
   */
  int AllocYuv420SPBuffer(const char * src_data, int input_size,
                          bool is_input_align, int width, int align_width,
                          int high, int align_high, int buffer_size,
                          char * dest_data);

  /**
   * @brief alloc buffer for yuv422_sp image
   * @param [in] src_data: source image data
   * @param [in] input_size: source image data size
   * @param [in] is_input_align: true: input image aligned;
   *                             false: input image not aligned
   * @param [in] format: input image format
   * @param [in] width: image width
   * @param [in] align_width: image width after align
   * @param [in] high: image high
   * @param [in] align_high: image high after align
   * @param [out] buffer_size: image data size after align
   * @param [out] dest_data: image data after align
   * @return enum DvppErrorCode
   */
  int AllocYuv422SPBuffer(const char * src_data, int input_size,
                          bool is_input_align, int width, int align_width,
                          int high, int align_high, int buffer_size,
                          char * dest_data);

  /**
   * @brief alloc buffer for yuv444_sp image
   * @param [in] src_data: source image data
   * @param [in] input_size: source image data size
   * @param [in] is_input_align: true: input image aligned;
   *                             false: input image not aligned
   * @param [in] format: input image format
   * @param [in] width: image width
   * @param [in] y_align_width: image width of y component after align
   * @param [in] uv_align_width: image width of uv component after align
   * @param [in] high: image high
   * @param [in] align_high: image high after align
   * @param [out] buffer_size: image data size after align
   * @param [out] dest_data: image data after align
   * @return enum DvppErrorCode
   */
  int AllocYuv444SPBuffer(const char * src_data, int input_size,
                          bool is_input_align, int width, int y_align_width,
                          int uv_align_width, int high, int align_high,
                          int buffer_size, char * dest_data);

  /**
   * @brief alloc buffer for yuv packed image or rgb packed image,
   *        inclued yuv444, yuv422, rgb888, xrgb8888
   * @param [in] src_data: source image data
   * @param [in] input_size: source image data size
   * @param [in] is_input_align: true: input image aligned;
   *                             false: input image not aligned
   * @param [in] format: input image format
   * @param [in] width: image width
   * @param [in] align_width: image width after align
   * @param [in] high: image high
   * @param [in] align_high: image high after align
   * @param [out] buffer_size: image data size after align
   * @param [out] dest_data: image data after align
   * @return enum DvppErrorCode
   */
  int AllocYuvOrRgbPackedBuffer(const char * src_data, int input_size,
                                bool is_input_align, int width, int align_width,
                                int high, int align_high, int buffer_size,
                                char * dest_data);

};

} /* namespace utils */
} /* namespace ascend */

#endif /* ASCENDDK_ASCEND_EZDVPP_DVPP_UTILS_H_ */
