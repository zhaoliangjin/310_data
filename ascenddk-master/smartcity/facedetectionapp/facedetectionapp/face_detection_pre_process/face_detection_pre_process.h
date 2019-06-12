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
#ifndef FACE_DETECTION_PRE_PROCESS_H_
#define FACE_DETECTION_PRE_PROCESS_H_

#include <string>
#include <vector>
#include "dvpp/dvpp_config.h"
#include "dvpp/idvppapi.h"
#include "face_detection_params.h"
#include "hiaiengine/data_type.h"
#include "hiaiengine/data_type_reg.h"
#include "hiaiengine/engine.h"

#define INPUT_SIZE 1   // number of input params.
#define OUTPUT_SIZE 1  // number of output params.

using hiai::AIConfig;
using hiai::AIModelDescription;
using hiai::BatchImagePara;
using hiai::BatchInfo;
using hiai::Engine;
using hiai::ImageData;

// configuration paramters struct.
typedef struct DvppConfig {
  float resize_width;
  float resize_height;

  DvppConfig() {
    resize_width = 0;
    resize_height = 0;
  }
} DvppConfig;

// define FaceDetectionPreProcess
class FaceDetectionPreProcess : public Engine {
 public:
  FaceDetectionPreProcess();
  ~FaceDetectionPreProcess();
  /**
   *@brief override base class Init method.
   @return HIAI_StatusT
   */
  HIAI_StatusT Init(const AIConfig &config,
                    const std::vector<AIModelDescription> &model_desc);

  /**
   * @ingroup hiaiengine
   * @brief Override the processing logic of the engine.
   * @param [in] Define an input port and an output port.
   */
  HIAI_DEFINE_PROCESS(INPUT_SIZE, OUTPUT_SIZE);

 private:
  /**
   * @brief clear output data.
   */
  void ClearData();
  /**
   * @brief  align image memory before calling dvpp.
   * @param [in] input image data.
   * @return HIAI_StatusT
   */
  HIAI_StatusT HandleVpc(const hiai::ImageData<u_int8_t> &img);
  /**
   * @brief  call dvpp to crop&resize.
   * @param [in] buffer:input buffer.
   * @param [in] buffer_size:input buffer size.
   * @param [in] img：input image data.
   * @param [in] width:byte-aligned image width.
   * @param [in] height:byte-aligned image height.
   * @return HIAI_StatusT
   */
  HIAI_StatusT HandleVpcWithParam(unsigned char *buffer, long buffer_size,
                                  const ImageData<u_int8_t> &img,
                                  int align_width, int align_height);
  /**
   * @brief  send batchImageOut to the next engine.
   * @return HIAI_StatusT
   */
  HIAI_StatusT SendImage();
  /**
   * @brief filter the batchImageIn according to the image type before
   * calling HandleVpc.
   * @return HIAI_StatusT
   */
  HIAI_StatusT HandleDvpp();
  /**
   * @brief check if string is a number.
   * @return true if str is a number else return false;
   */
  static bool IsStrNum(const string &str);

 private:
  // configuration parameters.
  std::shared_ptr<DvppConfig> dvpp_config_;
  // input batch images.
  std::shared_ptr<BatchImageParaWithScaleT> batch_image_in_;
  // output batch images.
  std::shared_ptr<BatchPreProcessedImageT> batch_image_out_;
  // dvpp handle.
  IDVPPAPI *dvpp_api_;
};

#endif  // FACE_DETECTION_PRE_PROCESS_H_
