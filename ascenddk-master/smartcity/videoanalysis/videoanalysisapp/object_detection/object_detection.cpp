
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

#include "object_detection.h"
#include <unistd.h>
#include <memory>
#include <sstream>
#include "ascenddk/ascend_ezdvpp/dvpp_data_type.h"
#include "ascenddk/ascend_ezdvpp/dvpp_process.h"

using ascend::utils::DvppCropOrResizePara;
using ascend::utils::DvppOutput;
using ascend::utils::DvppProcess;
using ascend::utils::DvppVpcImageType;
using hiai::ImageData;
using hiai::IMAGEFORMAT;
using namespace std;

namespace {
const uint32_t kInputPort = 0;
const uint32_t kOutputPort = 0;

// ssd input image width and height.
const uint32_t kInputWidth = 512;
const uint32_t kInputHeight = 512;

const int kDvppProcSuccess = 0;     // call dvpp success return.
const int kSleepMicroSecs = 20000;  // waiting time after queue is full.

const string kModelPath = "model_path";
}  // namespace

HIAI_REGISTER_DATA_TYPE("VideoImageInfoT", VideoImageInfoT);
HIAI_REGISTER_DATA_TYPE("VideoImageParaT", VideoImageParaT);
HIAI_REGISTER_DATA_TYPE("OutputT", OutputT);
HIAI_REGISTER_DATA_TYPE("DetectionEngineTransT", DetectionEngineTransT);

HIAI_StatusT ObjectDetectionInferenceEngine::Init(
    const hiai::AIConfig& config,
    const vector<hiai::AIModelDescription>& model_desc) {
  HIAI_ENGINE_LOG(HIAI_DEBUG_INFO, "[ODInferenceEngine] start to initialize!");

  if (ai_model_manager_ == nullptr) {
    ai_model_manager_ = make_shared<hiai::AIModelManager>();
  }

  vector<hiai::AIModelDescription> od_model_descs;
  hiai::AIModelDescription model_description;

  // load model path.
  for (int index = 0; index < config.items_size(); ++index) {
    const ::hiai::AIConfigItem& item = config.items(index);
    if (item.name() == kModelPath) {
      const char* model_path = item.value().data();
      model_description.set_path(model_path);
    }
  }
  od_model_descs.push_back(model_description);

  // init ssd model
  HIAI_StatusT ret = ai_model_manager_->Init(config, od_model_descs);
  if (ret != hiai::SUCCESS) {
    HIAI_ENGINE_LOG(HIAI_GRAPH_INVALID_VALUE,
                    "[ODInferenceEngine] failed to initialize AI model!");
    return HIAI_ERROR;
  }

  HIAI_ENGINE_LOG(HIAI_DEBUG_INFO, "[ODInferenceEngine] engine initialized!");
  return HIAI_OK;
}

HIAI_StatusT ObjectDetectionInferenceEngine::ImagePreProcess(
    const ImageData<u_int8_t>& src_img, ImageData<u_int8_t>& resized_img) {
  if (src_img.format != IMAGEFORMAT::YUV420SP) {
    // input image must be yuv420sp nv12.
    HIAI_ENGINE_LOG(HIAI_ENGINE_RUN_ARGS_NOT_RIGHT,
                    "[ODInferenceEngine] input image type does not match");
    return HIAI_ERROR;
  }

  // assemble resize param struct
  DvppCropOrResizePara dvpp_resize_param;
  dvpp_resize_param.src_resolution.height = src_img.height;
  dvpp_resize_param.src_resolution.width = src_img.width;

  // the value of horz_max and vert_max must be odd.
  dvpp_resize_param.horz_max =
      src_img.width % 2 == 0 ? src_img.width - 1 : src_img.width;
  dvpp_resize_param.vert_max =
      src_img.height % 2 == 0 ? src_img.height - 1 : src_img.height;
  dvpp_resize_param.dest_resolution.width = kInputWidth;
  dvpp_resize_param.dest_resolution.height = kInputHeight;

  // the input image is aligned in memory.
  dvpp_resize_param.is_input_align = true;

  DvppProcess dvpp_process(dvpp_resize_param);

  DvppOutput dvpp_out;
  int ret = dvpp_process.DvppOperationProc(
      reinterpret_cast<char*>(src_img.data.get()), src_img.size, &dvpp_out);

  if (ret != kDvppProcSuccess) {
    HIAI_ENGINE_LOG(HIAI_ENGINE_RUN_ARGS_NOT_RIGHT,
                    "[ODInferenceEngine] call dvpp resize failed with code %d!",
                    ret);
    return HIAI_ERROR;
  }

  // dvpp_out->pbuf
  resized_img.data.reset(dvpp_out.buffer, default_delete<uint8_t[]>());
  resized_img.size = dvpp_out.size;

  return HIAI_OK;
}

HIAI_StatusT ObjectDetectionInferenceEngine::PerformInference(
    shared_ptr<DetectionEngineTransT>& detection_trans,
    ImageData<u_int8_t>& input_img) {
  // init neural buffer.
  shared_ptr<hiai::AINeuralNetworkBuffer> neural_buffer =
      shared_ptr<hiai::AINeuralNetworkBuffer>(
          new hiai::AINeuralNetworkBuffer(),
          default_delete<hiai::AINeuralNetworkBuffer>());

  neural_buffer->SetBuffer((void*)input_img.data.get(), input_img.size);

  shared_ptr<hiai::IAITensor> input_tensor =
      static_pointer_cast<hiai::IAITensor>(neural_buffer);

  vector<shared_ptr<hiai::IAITensor>> input_tensors;
  vector<shared_ptr<hiai::IAITensor>> output_tensors;
  input_tensors.push_back(input_tensor);

  HIAI_StatusT ret = hiai::SUCCESS;
  ret = ai_model_manager_->CreateOutputTensor(input_tensors, output_tensors);
  if (ret != hiai::SUCCESS) {
    SendDetectionResult(detection_trans, false,
                        "[ODInferenceEngine] output tensor created failed!");
    return HIAI_ERROR;
  }
  hiai::AIContext ai_context;

  // neural network inference.
  ret =
      ai_model_manager_->Process(ai_context, input_tensors, output_tensors, 0);
  if (ret != hiai::SUCCESS) {
    SendDetectionResult(detection_trans, false,
                        "[ODInferenceEngine] image inference failed!");
    return HIAI_ERROR;
  }

  // set trans_data
  detection_trans->status = true;
  for (uint32_t index = 0; index < output_tensors.size(); ++index) {
    shared_ptr<hiai::AINeuralNetworkBuffer> result_tensor =
        static_pointer_cast<hiai::AINeuralNetworkBuffer>(output_tensors[index]);
    OutputT out;
    out.size = result_tensor->GetSize();
    out.data = std::shared_ptr<uint8_t>(new uint8_t[out.size],
                                        std::default_delete<uint8_t[]>());
    errno_t ret = memcpy_s(out.data.get(), out.size, result_tensor->GetBuffer(),
                           out.size);
    if (ret != EOK) {
      HIAI_ENGINE_LOG(HIAI_ENGINE_RUN_ARGS_NOT_RIGHT,
                      "[ODInferenceEngine] output tensor copy failure!");
      continue;
    }
    detection_trans->output_datas.push_back(out);
  }
  if (detection_trans->output_datas.empty()) {
    SendDetectionResult(detection_trans, false,
                        "[ODInferenceEngine] result tensor is empty!");
    return HIAI_ERROR;
  }

  // sendData
  return SendDetectionResult(detection_trans);
}

HIAI_StatusT ObjectDetectionInferenceEngine::SendDetectionResult(
    shared_ptr<DetectionEngineTransT>& detection_trans, bool inference_success,

    string err_msg) {
  if (!inference_success) {
    // inference error.
    HIAI_ENGINE_LOG(HIAI_ENGINE_RUN_ARGS_NOT_RIGHT, err_msg.c_str());
    detection_trans->status = false;
    detection_trans->msg = err_msg;
  }
  HIAI_StatusT ret;
  do {
    // send data to next engine.
    ret = SendData(kOutputPort, "DetectionEngineTransT",
                   static_pointer_cast<void>(detection_trans));
    if (ret == HIAI_QUEUE_FULL) {
      HIAI_ENGINE_LOG(HIAI_DEBUG_INFO, "[ODInferenceEngine] output queue full");
      usleep(kSleepMicroSecs);
    }
  } while (ret == HIAI_QUEUE_FULL);
  if (ret != HIAI_OK) {
    HIAI_ENGINE_LOG(HIAI_ENGINE_RUN_ARGS_NOT_RIGHT,
                    "[ODInferenceEngine] send inference data failed!");
    return HIAI_ERROR;
  }
  return HIAI_OK;
}

HIAI_IMPL_ENGINE_PROCESS("object_detection", ObjectDetectionInferenceEngine,
                         INPUT_SIZE) {
  HIAI_ENGINE_LOG(HIAI_DEBUG_INFO, "[ODInferenceEngine] start process!");

  if (arg0 == nullptr) {
    // inputer data is nullptr.
    HIAI_ENGINE_LOG(HIAI_ENGINE_RUN_ARGS_NOT_RIGHT,
                    "[ODInferenceEngine] input data is null!");
    return HIAI_ERROR;
  }
  shared_ptr<VideoImageParaT> video_image =
      static_pointer_cast<VideoImageParaT>(arg0);

  // init inference results tensor shared_ptr.
  shared_ptr<DetectionEngineTransT> detection_trans =
      make_shared<DetectionEngineTransT>();

  detection_trans->video_image = *video_image;
  if (detection_trans->video_image.video_image_info.is_finished) {
    // input is finished.
    HIAI_ENGINE_LOG(HIAI_DEBUG_INFO,
                    "[ODInferenceEngine] input video finished!");
    return SendDetectionResult(detection_trans);
  }

  // resize input image.
  ImageData<u_int8_t> resized_img;
  HIAI_StatusT dvpp_ret =
      ImagePreProcess(detection_trans->video_image.img, resized_img);

  if (dvpp_ret != HIAI_OK) {
    // if preprocess error,send input image to the next engine.
    SendDetectionResult(detection_trans, false,
                        "[ODInferenceEngine] image preprocessed failure!");
    return HIAI_ERROR;
  }

  // inference
  return PerformInference(detection_trans, resized_img);
}
