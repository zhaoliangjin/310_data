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
#ifndef OBJECT_DETECTION_OBJECT_DETECTION_H_
#define OBJECT_DETECTION_OBJECT_DETECTION_H_
#include <string>
#include <vector>
#include "hiaiengine/ai_model_manager.h"
#include "hiaiengine/ai_tensor.h"
#include "hiaiengine/ai_types.h"
#include "hiaiengine/api.h"
#include "hiaiengine/data_type.h"
#include "hiaiengine/data_type_reg.h"
#include "hiaiengine/engine.h"
#include "video_analysis_params.h"

#define INPUT_SIZE 2
#define OUTPUT_SIZE 1

class ObjectDetectionInferenceEngine : public hiai::Engine {
 public:
  /**
   * @brief Engine init method.
   * @return HIAI_StatusT
   */
  HIAI_StatusT Init(const hiai::AIConfig& config,
                    const std::vector<hiai::AIModelDescription>& model_desc);

  /**
   * @ingroup hiaiengine
   * @brief HIAI_DEFINE_PROCESS : override Engine Process logic.
   * @[in]: define a input port, a output port
   * @return HIAI_StatusT
   */
  HIAI_DEFINE_PROCESS(INPUT_SIZE, OUTPUT_SIZE);

 private:
  /**
   * @brief : image preprocess function.
   * @param [in] src_img: input image data.
   * @param [out] resized_img: resized image data.
   * @return HIAI_StatusT
   */
  HIAI_StatusT ImagePreProcess(const hiai::ImageData<u_int8_t>& src_img,
                               hiai::ImageData<u_int8_t>& resized_img);

  /**
   * @brief : object detection function.
   * @param [in] input_img: input image data.
   * @param [out] detection_trans: inference results tensor.
   * @return HIAI_StatusT
   */
  HIAI_StatusT PerformInference(
      std::shared_ptr<DetectionEngineTransT>& detection_trans,
      hiai::ImageData<u_int8_t>& input_img);

  /**
   * @brief : send inference results to next engine.
   * @param [in] detection_trans: inference results tensor.
   * @param [in] success: inference success or not.
   * @param [in] err_msg: save error message if detection failed.
   * @return HIAI_StatusT
   */
  HIAI_StatusT SendDetectionResult(
      std::shared_ptr<DetectionEngineTransT>& detection_trans,
      bool inference_success = true, std::string err_msg = "");

  // shared ptr to load ai model.
  std::shared_ptr<hiai::AIModelManager> ai_model_manager_;
};

#endif /* OBJECT_DETECTION_OBJECT_DETECTION_H_ */
