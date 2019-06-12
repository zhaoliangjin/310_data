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
#include "face_detection_inference.h"
#include <vector>
#include "hiaiengine/log.h"
#include "hiaiengine/ai_types.h"
#include "hiaiengine/ai_model_parser.h"

using hiai::Engine;

namespace {
// output port (engine port begin with 0)
const uint32_t kSendDataPort = 0;
}

// register custom data type
HIAI_REGISTER_DATA_TYPE("EngineTransT", EngineTransT);
HIAI_REGISTER_DATA_TYPE("OutputT", OutputT);
HIAI_REGISTER_DATA_TYPE("ScaleInfoT", ScaleInfoT);
HIAI_REGISTER_DATA_TYPE("NewImageParaT", NewImageParaT);
HIAI_REGISTER_DATA_TYPE("BatchImageParaWithScaleT", BatchImageParaWithScaleT);
HIAI_REGISTER_DATA_TYPE("BatchPreProcessedImageT", BatchPreProcessedImageT);

FaceDetectionInference::FaceDetectionInference() {
  ai_model_manager_ = nullptr;
}

HIAI_StatusT FaceDetectionInference::Init(
    const hiai::AIConfig& config,
    const std::vector<hiai::AIModelDescription>& model_desc) {
  HIAI_ENGINE_LOG("Start initialize!");

  // initialize aiModelManager
  if (ai_model_manager_ == nullptr) {
    ai_model_manager_ = std::make_shared<hiai::AIModelManager>();
  }

  // get parameters from graph.config
  // set model path and passcode to AI model description
  hiai::AIModelDescription fd_model_desc;
  for (int index = 0; index < config.items_size(); index++) {
    const ::hiai::AIConfigItem& item = config.items(index);
    // get model path
    if (item.name() == "model_path") {
      const char* model_path = item.value().data();
      fd_model_desc.set_path(model_path);
    }
  }

  // initialize model manager
  std::vector<hiai::AIModelDescription> model_desc_vec;
  model_desc_vec.push_back(fd_model_desc);
  hiai::AIStatus ret = ai_model_manager_->Init(config, model_desc_vec);
  // initialize AI model manager failed
  if (ret != hiai::SUCCESS) {
    HIAI_ENGINE_LOG(HIAI_GRAPH_INVALID_VALUE, "initialize AI model failed");
    return HIAI_ERROR;
  }

  HIAI_ENGINE_LOG("End initialize!");
  return HIAI_OK;
}

HIAI_IMPL_ENGINE_PROCESS("face_detection_inference",
    FaceDetectionInference, INPUT_SIZE) {
  HIAI_ENGINE_LOG("Start process!");

  // check arg0 is null or not
  if (arg0 == nullptr) {
    HIAI_ENGINE_LOG(HIAI_ENGINE_RUN_ARGS_NOT_RIGHT,
                    "Failed to process invalid message.");
    return HIAI_ERROR;
  }

  // initialize as zero
  uint32_t all_input_size = 0;
  std::shared_ptr<BatchPreProcessedImageT> image_handle =
      std::static_pointer_cast<BatchPreProcessedImageT>(arg0);
  for (uint32_t i = 0; i < image_handle->batch_info.batch_size; i++) {
    // get all input size
    all_input_size += image_handle->processed_imgs[i].img.size
        * sizeof(uint8_t);
  }
  // input size is less than zero, do not need to inference
  if (all_input_size <= 0) {
    HIAI_ENGINE_LOG(HIAI_ENGINE_RUN_ARGS_NOT_RIGHT,
                    "all input image size=%u is less than zero",
                    all_input_size);
    return HIAI_ERROR;
  }

  // copy original data
  std::shared_ptr<EngineTransT> trans_data = std::make_shared<EngineTransT>();
  trans_data->b_info = image_handle->batch_info;
  trans_data->imgs = image_handle->original_imgs;

  // copy image data
  std::shared_ptr<uint8_t> temp = std::shared_ptr<uint8_t>(
      new uint8_t[all_input_size], std::default_delete<uint8_t[]>());
  uint32_t last_size = 0;
  for (uint32_t i = 0; i < image_handle->batch_info.batch_size; i++) {
    // copy memory according to each size
    uint32_t each_size = image_handle->processed_imgs[i].img.size
        * sizeof(uint8_t);
    HIAI_ENGINE_LOG("each input image size: %u", each_size);
    errno_t mem_ret = memcpy_s(temp.get() + last_size,
                               all_input_size - last_size,
                               image_handle->processed_imgs[i].img.data.get(),
                               each_size);
    // memory copy failed, no need to inference, send original image
    if (mem_ret != EOK) {
      HIAI_ENGINE_LOG(HIAI_ENGINE_RUN_ARGS_NOT_RIGHT,
                      "prepare image data: memcpy_s() error=%d", mem_ret);
      trans_data->status = false;
      trans_data->msg = "HiAIInference Engine memcpy_s image data failed";
      // send data to engine output port 0
      SendData(kSendDataPort, "EngineTransT",
               std::static_pointer_cast<void>(trans_data));
      return HIAI_ERROR;
    }
    last_size += each_size;
  }

  // neural buffer
  std::shared_ptr<hiai::AINeuralNetworkBuffer> neural_buf = std::shared_ptr<
      hiai::AINeuralNetworkBuffer>(
      new hiai::AINeuralNetworkBuffer(),
      std::default_delete<hiai::AINeuralNetworkBuffer>());
  neural_buf->SetBuffer((void*) temp.get(), all_input_size);

  // input data
  std::shared_ptr<hiai::IAITensor> input_data = std::static_pointer_cast<
      hiai::IAITensor>(neural_buf);
  std::vector<std::shared_ptr<hiai::IAITensor>> input_data_vec;
  input_data_vec.push_back(input_data);

  // Call Process
  // 1. create output tensor
  hiai::AIContext ai_context;
  std::vector<std::shared_ptr<hiai::IAITensor>> output_data_vector;
  hiai::AIStatus ret = ai_model_manager_->CreateOutputTensor(
      input_data_vec, output_data_vector);
  // create failed, also need to send data to post process
  if (ret != hiai::SUCCESS) {
    HIAI_ENGINE_LOG(HIAI_ENGINE_RUN_ARGS_NOT_RIGHT,
                    "failed to create output tensor");
    trans_data->status = false;
    trans_data->msg = "HiAIInference Engine CreateOutputTensor failed";
    // send data to engine output port 0
    SendData(kSendDataPort, "EngineTransT",
             std::static_pointer_cast<void>(trans_data));
    return HIAI_ERROR;
  }

  // 2. process
  HIAI_ENGINE_LOG("aiModelManager->Process start!");
  ret = ai_model_manager_->Process(ai_context, input_data_vec,
                                   output_data_vector,
                                   AI_MODEL_PROCESS_TIMEOUT);
  // process failed, also need to send data to post process
  if (ret != hiai::SUCCESS) {
    HIAI_ENGINE_LOG(HIAI_ENGINE_RUN_ARGS_NOT_RIGHT,
                    "failed to process ai_model");
    trans_data->status = false;
    trans_data->msg = "HiAIInference Engine Process failed";
    // send data to engine output port 0
    SendData(kSendDataPort, "EngineTransT",
             std::static_pointer_cast<void>(trans_data));
    return HIAI_ERROR;
  }
  HIAI_ENGINE_LOG("aiModelManager->Process end!");

  // generate output data
  trans_data->status = true;
  for (uint32_t i = 0; i < output_data_vector.size(); i++) {
    std::shared_ptr<hiai::AISimpleTensor> result_tensor =
        std::static_pointer_cast<hiai::AISimpleTensor>(output_data_vector[i]);
    OutputT out;
    out.size = result_tensor->GetSize();
    out.data = std::shared_ptr<uint8_t>(new uint8_t[out.size],
                                        std::default_delete<uint8_t[]>());
    errno_t mem_ret = memcpy_s(out.data.get(), out.size,
                               result_tensor->GetBuffer(),
                               result_tensor->GetSize());
    // memory copy failed, skip this result
    if (mem_ret != EOK) {
      HIAI_ENGINE_LOG(HIAI_ENGINE_RUN_ARGS_NOT_RIGHT,
                      "dealing results: memcpy_s() error=%d", mem_ret);
      continue;
    }
    trans_data->output_datas.push_back(out);
  }

  // send results and original image data to post process (port 0)
  HIAI_StatusT hiai_ret = SendData(kSendDataPort, "EngineTransT",
                                   std::static_pointer_cast<void>(trans_data));
  HIAI_ENGINE_LOG("End process!");
  return hiai_ret;
}
