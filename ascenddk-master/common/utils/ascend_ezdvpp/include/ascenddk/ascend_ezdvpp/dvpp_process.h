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

#ifndef ASCENDDK_ASCEND_EZDVPP_DVPP_PROCESS_H_
#define ASCENDDK_ASCEND_EZDVPP_DVPP_PROCESS_H_

#include "dvpp/idvppapi.h"
#include "dvpp_utils.h"

namespace ascend {
namespace utils {

/*
 *
 */
class DvppProcess {
 public:
  /**
   * @brief class constructor
   * @param [in] DvppToJpgPara para: instance jpg object.
   */
  DvppProcess(const DvppToJpgPara &para);

  /**
   * @brief class constructor
   * @param [in] DvppToJpgPara para: instance h264 object
   */
  DvppProcess(const DvppToH264Para &para);

  /**
   * @brief class constructor
   * @param [in] DvppToYuvPara para: instance yuv object
   */
  DvppProcess(const DvppToYuvPara &para);

  /**
   * @brief class constructor
   * @param [in] DvppCropOrResizePara para: instance crop or resize object
   */
  DvppProcess(const DvppCropOrResizePara &para);

  /**
   * @brief class constructor
   * @param [in] DvppJpegDInPara para: instance jpeg decode object
   */
  DvppProcess(const DvppJpegDInPara &para);

  //class destructor
  virtual ~DvppProcess();

  /**
   * @brief Dvpp change from yuv to jpg or h264(if you use jpg object,
   *        then dvpp output jpeg data.if you use h264 object,then dvpp
   *        output h264 data.)
   * @param [in] char *input_buf: yuv data buffer
   *             (dvpp need char *,so pInputBuf do not use const)
   * @param [in] int input_size  : size of yuv data buffer
   * @param [out]DvppOutput *output_data :dvpp output buffer and size
   * @return  enum DvppErrorCode
   */
  int DvppOperationProc(const char *input_buf, int input_size,
                        DvppOutput *output_data);

  /**
   * @brief Dvpp decode jpeg and change jpeg to yuv
   * @param [in] char *input_buf: jpeg data buffer
   * @param [in] int input_size  : size of jpeg data buffer
   * @param [out]DvppJpegDOutput *output_data :dvpp output buffer and size
   * @return  enum DvppErrorCode
   */
  int DvppJpegDProc(const char *input_buf, int input_size,
                    DvppJpegDOutput *output_data);

  /**
   * @brief get a error message according to error code.
   * @param [in] int code: error code.
   */
  void PrintErrorInfo(int code) const;

  /**
   * @brief get conversion mode.
   * @param [in] enum CaptureObjFlag: conversion mode.
   */
  int GetMode() const;

 private:
  /**
   * @brief Dvpp change from yuv to jpg
   * @param [in] char *input_buf: yuv data buffer
   *             (dvpp need char *,so pInputBuf do not use const)
   * @param [in] int input_size  : size of yuv data buffer
   * @param [out]sJpegeOut *output_data :dvpp output buffer and size
   * @return  enum DvppErrorCode
   */
  int DvppYuvChangeToJpeg(const char *input_buf, int input_size,
                          sJpegeOut *output_data);

  /**
   * @brief Dvpp deal with yuv to jpg
   * @param [in] sJpegeIn &input_data: yuv data buffer
   * @param [out]sJpegeOut *output_data :dvpp output buffer and size
   * @return  enum DvppErrorCode
   */
  int DvppProc(const sJpegeIn &input_data, sJpegeOut *output_data);

  /**
   * @brief Dvpp change from yuv to h264
   * @param [in] char *input_buf: yuv data buffer
   *             (dvpp need char *,so pInputBuf do not use const)
   * @param [in] int input_size  : size of yuv data buffer
   * @param [out]shared_ptr<AutoBuffer> *output_buf :dvpp output buffer
   *        and size
   * @return  enum DvppErrorCode
   */
  int DvppYuvChangeToH264(const char *input_buf, int input_size,
                          shared_ptr<AutoBuffer> *output_buf);

  /**
   * @brief convert image from BGR to YUV420SP_NV12
   * @param [in] input_buf:input image data
   *             (dvpp need char *,so pInputBuf do not use const)
   * @param [in] input_size: input image data size
   * @param [in] output_size: output image data size
   * @param [out] output_buf: image data after conversion
   * @return enum DvppErrorCode
   */
  int DvppBgrChangeToYuv(const char *input_buf, int input_size, int output_size,
                         unsigned char *output_buf);

  /**
   * @brief crop or resize origin image
   * @param [in] input_buf:input image data
   *             (dvpp need char *,so pInputBuf do not use const)
   * @param [in] input_size: input image data size
   * @param [in] output_size: output image data size
   * @param [out] output_buf: image data after conversion
   * @return enum DvppErrorCode
   */
  int DvppCropOrResize(const char *input_buf, int input_size, int output_size,
                       unsigned char *output_buf);

  /**
   * @brief change jpeg image to yuv
   * @param [in] input_buf:input image data
   * @param [in] input_size: input image data size
   * @param [out] jpegd_output_data: image data after jpegd
   * @return enum DvppErrorCode
   */
  int DvppJpegChangeToYuv(const char *input_buf, int input_size,
                          jpegd_yuv_data_info *jpegd_output_data);

  // used for storage attributes of dvpp class
  struct DvppPara dvpp_instance_para_;

  // DVPP instance mode(jpg or h264).
  int convert_mode_;
};
}
}
#endif /* ASCENDDK_ASCEND_EZDVPP_DVPP_PROCESS_H_ */
