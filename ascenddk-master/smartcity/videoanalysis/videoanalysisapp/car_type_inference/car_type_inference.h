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
#ifndef CAR_TYPE_INFERENCE_H_
#define CAR_TYPE_INFERENCE_H_

#include "hiaiengine/api.h"
#include "hiaiengine/ai_model_manager.h"
#include "hiaiengine/ai_types.h"
#include "hiaiengine/data_type.h"
#include "hiaiengine/engine.h"
#include "hiaiengine/multitype_queue.h"
#include "hiaiengine/data_type_reg.h"
#include "hiaiengine/ai_tensor.h"
#include "ascenddk/ascend_ezdvpp/dvpp_process.h"

#include "video_analysis_params.h"

#define INPUT_SIZE 2
#define OUTPUT_SIZE 1
#define BATCH_SIZE 10

class CarTypeInferenceEngine : public hiai::Engine {
 public:
  /**
   * @brief constructor
   */
  CarTypeInferenceEngine()
      : input_que_(INPUT_SIZE - 1) {
    batch_size_ = BATCH_SIZE;
  }
  /**
   * @brief  init config of video post by aiConfig
   * @param [in] config:  initialized aiConfig
   * @param [in] model_desc:  modelDesc
   * @return  success --> HIAI_OK ; fail --> HIAI_ERROR
   */
  HIAI_StatusT Init(const hiai::AIConfig& config,
                    const std::vector<hiai::AIModelDescription>& model_desc);
  /**
   * @brief HIAI_DEFINE_PROCESS : override Engine Process logic
   * @param[in]: define a input port, a output port
   */
  HIAI_DEFINE_PROCESS(INPUT_SIZE, OUTPUT_SIZE);

 private:
  // How many image numbers of a batch.
  int batch_size_;
  // used to cache the input queue.
  hiai::MultiTypeQueue input_que_;
  // Define a AIModelManager type smart pointer.
  std::shared_ptr<hiai::AIModelManager> ai_model_manager_;
  /**
   * @brief call ez_dvpp interface for resizing image.
   * @param [in] batch_image_input:  batch image from previous engine.
   * @param [out] batch_image_output: batch image for processing.
   */
  void BatchImageResize(
      std::shared_ptr<BatchCroppedImageParaT>& batch_image_input,
      std::shared_ptr<BatchCroppedImageParaT>& batch_image_output);
  /**
   * @brief  send result data to next engine.
   * @param [in] tran_data: the data will be sent to next engine.
   * @return  success --> HIAI_OK ; fail --> HIAI_ERROR
   */
  HIAI_StatusT SendResultData(const std::shared_ptr<BatchCarInfoT>& tran_data);
  /**
   * @brief  batch inference
   * @param [in] image_handle: batch image for processing.
   * @param [in] tran_data:  the data will be sent to next engine.
   * @return  success --> HIAI_OK ; fail --> HIAI_ERROR
   */
  HIAI_StatusT BatchInferenceProcess(
      const std::shared_ptr<BatchCroppedImageParaT>& image_handle,
      std::shared_ptr<BatchCarInfoT> tran_data);
  /**
   * @brief  construct batch buffer as a input for process
   * @param [in] batch_index:  batch index of input image;
   * @param [in] image_handle:  batch image for processing.
   * @param [in] temp:  apply memory for image origin data.
   * @return  success --> true ; fail --> fail
   */
  bool ConstructBatchBuffer(
      int batch_index,
      const std::shared_ptr<BatchCroppedImageParaT>& image_handle,
      uint8_t* temp);
  /**
   * @brief  analyze inference result
   * @param [in] output_data_vec:  inference output from model
   * @param [in] batch_index:  batch index of input image;
   * @param [in] image_handle:  batch image for processing.
   * @param [in] tran_data: the data will be sent to next engine.
   * @return  success --> true ; fail --> fail
   */
  bool ConstructInferenceResult(
      const std::vector<std::shared_ptr<hiai::IAITensor> >& output_data_vec,
      int batch_index,
      const std::shared_ptr<BatchCroppedImageParaT>& image_handle,
      const std::shared_ptr<BatchCarInfoT>& tran_data);
};

#endif
