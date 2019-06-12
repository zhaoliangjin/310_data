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

#ifndef PEDESTRIAN_ATTR_INFERENCE_H_
#define PEDESTRIAN_ATTR_INFERENCE_H_

#include "video_analysis_params.h"
#include "hiaiengine/api.h"
#include "hiaiengine/ai_model_manager.h"
#include "hiaiengine/ai_types.h"
#include "hiaiengine/data_type.h"
#include "hiaiengine/engine.h"
#include "hiaiengine/multitype_queue.h"
#include "hiaiengine/data_type_reg.h"
#include "hiaiengine/ai_tensor.h"
#include "ascenddk/ascend_ezdvpp/dvpp_process.h"

#define INPUT_SIZE 2
#define OUTPUT_SIZE 1

class PedestrianAttrInference : public hiai::Engine {
 public:

  /**
   * @brief PedestrianAttrInference constructor
   */
  PedestrianAttrInference()
      : input_que_(INPUT_SIZE - 1) {
    batch_size_ = kDefaultBatchSize;
  }

  /**
   * @brief current engine initialize function
   * @param [in] config: hiai engine config
   * @param [in] model_desc: hiai AI model description
   * @return HIAI_OK: initialize success; HIAI_ERROR:initialize failed
   */
  HIAI_StatusT Init(const hiai::AIConfig &config,
                    const std::vector<hiai::AIModelDescription> &model_desc);
  /**
   * @brief HIAI_DEFINE_PROCESS : override Engine Process logic
   * @param [in] INPUT_SIZE: the input size of engine
   * @param [in] OUTPUT_SIZE: the output size of engine
   */
HIAI_DEFINE_PROCESS(INPUT_SIZE, OUTPUT_SIZE)

 private:
  const int kDefaultBatchSize = 1; // default batch size

  int batch_size_; // model inference batch size

  // used for cache the input queue
  hiai::MultiTypeQueue input_que_;

  // used for AI model manage
  std::shared_ptr<hiai::AIModelManager> ai_model_manager_;

  /**
   * @brief batch resize image
   * @param [in] batch_image_input: batch input images
   * @param [out] batch_image_output: batch out images
   */
  void BatchImageResize(
      std::shared_ptr<BatchCroppedImageParaT> &batch_image_input,
      std::shared_ptr<BatchCroppedImageParaT> &batch_image_output);

  /**
   * @brief send result data to next engine
   * @param [in] tran_data: the data used for transmit
   * @return HIAI_OK: send success; HIAI_ERROR:send failed
   */
  HIAI_StatusT SendResultData(
      const std::shared_ptr<BatchPedestrianInfoT> &tran_data);

  /**
   * @brief send result data to next engine
   * @param [in] image_handle: the image data after resized
   * @param [in] tran_data: the data used for transmit
   * @return HIAI_OK: batch inference success; HIAI_ERROR:batch inference failed
   */
  HIAI_StatusT BatchInferenceProcess(
      const std::shared_ptr<BatchCroppedImageParaT> &image_handle,
      std::shared_ptr<BatchPedestrianInfoT> tran_data);

  /**
   * @brief send result data to next engine
   * @param [in] image_handle: the image data after resized
   * @param [in] batch_buffer: used for record image data
   * @return HIAI_OK: batch construct success; HIAI_ERROR:batch construct failed
   */
  bool ConstructBatchBuffer(
      int i, const std::shared_ptr<BatchCroppedImageParaT> &image_handle,
      uint8_t* batch_buffer);

  /**
   * @brief construct inference result
   * @param [in] output_data_vec: the vector used for record output data
   * @param [in] batch_index: batch index
   * @param [in] image_handle: the image data after resized
   * @param [out] tran_data: the data used for transmit
   * @return true: construct result success; HIAI_ERROR: construct result failed
   */
  bool ConstructInferenceResult(
      const std::vector<std::shared_ptr<hiai::IAITensor>> &output_data_vec,
      int batch_index,
      const std::shared_ptr<BatchCroppedImageParaT> &image_handle,
      const std::shared_ptr<BatchPedestrianInfoT> &tran_data);

  /**
   * @brief extract valid pedestrian attribute confidence
   * @param [in] batch_result_index: batch result index
   * @param [out] out_data: the out put data
   * @param [in] result: the result of each pedestrian attribute confidence
   */
  void ExtractResults(int batch_result_index, PedestrianInfoT &out_data,
                      float* &result);
};

#endif /* PEDESTRIAN_ATTR_INFERENCE_H_ */
