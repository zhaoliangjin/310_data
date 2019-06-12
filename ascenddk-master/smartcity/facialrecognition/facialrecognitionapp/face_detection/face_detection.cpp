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

#include "face_detection.h"

#include <vector>
#include <sstream>

#include "hiaiengine/log.h"
#include "ascenddk/ascend_ezdvpp/dvpp_process.h"

using hiai::Engine;
using hiai::ImageData;
using namespace std;
using namespace ascend::utils;

namespace {

// output port (engine port begin with 0)
const uint32_t kSendDataPort = 0;

// model need resized image to 300 * 300
const float kResizeWidth = 300.0;
const float kResizeHeight = 300.0;

// confidence parameter key in graph.confi
const string kConfidenceParamKey = "confidence";

// confidence range (0.0, 1.0]
const float kConfidenceMin = 0.0;
const float kConfidenceMax = 1.0;

// results
// inference output result index
const int32_t kResultIndex = 0;
// each result size (7 float)
const int32_t kEachResultSize = 7;
// attribute index
const int32_t kAttributeIndex = 1;
// score index
const int32_t kScoreIndex = 2;
// left top X-axis coordinate point
const int32_t kLeftTopXaxisIndex = 3;
// left top Y-axis coordinate point
const int32_t kLeftTopYaxisIndex = 4;
// right bottom X-axis coordinate point
const int32_t kRightBottomXaxisIndex = 5;
// right bottom Y-axis coordinate point
const int32_t kRightBottomYaxisIndex = 6;

// face attribute
const float kAttributeFaceLabelValue = 1.0;
const float kAttributeFaceDeviation = 0.00001;

// ratio
const float kMinRatio = 0.0;
const float kMaxRatio = 1.0;

// image source from register
const uint32_t kRegisterSrc = 1;

// sleep interval when queue full (unit:microseconds)
const __useconds_t kSleepInterval = 200000;
}

// register custom data type
HIAI_REGISTER_DATA_TYPE("FaceRecognitionInfo", FaceRecognitionInfo);
HIAI_REGISTER_DATA_TYPE("FaceRectangle", FaceRectangle);
HIAI_REGISTER_DATA_TYPE("FaceImage", FaceImage);

FaceDetection::FaceDetection() {
  ai_model_manager_ = nullptr;
  confidence_ = -1.0;  // initialized as invalid value
}

HIAI_StatusT FaceDetection::Init(
  const hiai::AIConfig& config,
  const vector<hiai::AIModelDescription>& model_desc) {
  HIAI_ENGINE_LOG("Start initialize!");

  // initialize aiModelManager
  if (ai_model_manager_ == nullptr) {
    ai_model_manager_ = make_shared<hiai::AIModelManager>();
  }

  // get parameters from graph.config
  // set model path to AI model description
  hiai::AIModelDescription fd_model_desc;
  for (int index = 0; index < config.items_size(); index++) {
    const ::hiai::AIConfigItem& item = config.items(index);
    // get model path
    if (item.name() == kModelPathParamKey) {
      const char* model_path = item.value().data();
      fd_model_desc.set_path(model_path);
    } else if (item.name() == kConfidenceParamKey) {  // get confidence
      stringstream ss(item.value());
      ss >> confidence_;
    }
    // else: noting need to do
  }

  // validate confidence
  if (!IsValidConfidence(confidence_)) {
    HIAI_ENGINE_LOG(HIAI_GRAPH_INVALID_VALUE,
                    "confidence invalid, please check your configuration.");
    return HIAI_ERROR;
  }

  // initialize model manager
  vector<hiai::AIModelDescription> model_desc_vec;
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

bool FaceDetection::IsValidConfidence(float confidence) {
  return (confidence > kConfidenceMin) && (confidence <= kConfidenceMax);
}

bool FaceDetection::IsValidResults(float attr, float score,
                                   const FaceRectangle &rectangle) {
  // attribute is not face (background)
  if (abs(attr - kAttributeFaceLabelValue) > kAttributeFaceDeviation) {
    return false;
  }

  // confidence check
  if ((score < confidence_) || !IsValidConfidence(score)) {
    return false;
  }

  // position check : lt == rb invalid
  if ((rectangle.lt.x == rectangle.rb.x)
      && (rectangle.lt.y == rectangle.rb.y)) {
    return false;
  }
  return true;
}

float FaceDetection::CorrectionRatio(float ratio) {
  float tmp = (ratio < kMinRatio) ? kMinRatio : ratio;
  return (tmp > kMaxRatio) ? kMaxRatio : tmp;
}

bool FaceDetection::PreProcess(
  const shared_ptr<FaceRecognitionInfo> &image_handle,
  ImageData<u_int8_t> &resized_image) {
  // input size is less than zero, return failed
  int32_t img_size = image_handle->org_img.size;
  if (img_size <= 0) {
    HIAI_ENGINE_LOG(HIAI_ENGINE_RUN_ARGS_NOT_RIGHT,
                    "original image size less than or equal zero, size=%d",
                    img_size);
    return false;
  }

  // call ez_dvpp to resize image
  DvppCropOrResizePara resize_para;
  resize_para.image_type = image_handle->frame.org_img_format;
  resize_para.rank = image_handle->frame.org_img_rank;



  // get original image size and set to resize parameter
  int32_t width = image_handle->org_img.width;
  int32_t height = image_handle->org_img.height;

  // set from 0 to width_max(width_max should be even)
  resize_para.horz_min = 0;
  resize_para.horz_max = width - 1;

  // set from 0 to height_max(height_max should be even)
  resize_para.vert_min = 0;
  resize_para.vert_max = height - 1;

  // set source resolution ratio
  resize_para.src_resolution.width = width;
  resize_para.src_resolution.height = height;

  // set destination resolution ratio
  resize_para.dest_resolution.width = kResizeWidth;
  resize_para.dest_resolution.height = kResizeHeight;

  // set input image align or not
  resize_para.is_input_align = image_handle->frame.img_aligned;

  // call
  DvppProcess dvpp_resize_img(resize_para);
  DvppOutput dvpp_output;
  int ret = dvpp_resize_img.DvppOperationProc(
              reinterpret_cast<char*>(image_handle->org_img.data.get()), img_size,
              &dvpp_output);
  if (ret != kDvppOperationOk) {
    HIAI_ENGINE_LOG(HIAI_ENGINE_RUN_ARGS_NOT_RIGHT,
                    "call ez_dvpp failed, failed to resize image.");
    return false;
  }

  // call success, set data and size
  resized_image.data.reset(dvpp_output.buffer, default_delete<u_int8_t[]>());
  resized_image.size = dvpp_output.size;
  return true;
}

bool FaceDetection::Inference(
  const ImageData<u_int8_t> &resized_image,
  vector<shared_ptr<hiai::IAITensor>> &output_data_vec) {
  // neural buffer
  shared_ptr<hiai::AINeuralNetworkBuffer> neural_buf = shared_ptr <
      hiai::AINeuralNetworkBuffer > (
        new hiai::AINeuralNetworkBuffer(),
        default_delete<hiai::AINeuralNetworkBuffer>());
  neural_buf->SetBuffer((void*) resized_image.data.get(), resized_image.size);

  // input data
  shared_ptr<hiai::IAITensor> input_data = static_pointer_cast<hiai::IAITensor>(
        neural_buf);
  vector<shared_ptr<hiai::IAITensor>> input_data_vec;
  input_data_vec.push_back(input_data);

  // Call Process
  // 1. create output tensor
  hiai::AIContext ai_context;
  hiai::AIStatus ret = ai_model_manager_->CreateOutputTensor(input_data_vec,
                       output_data_vec);
  // create failed
  if (ret != hiai::SUCCESS) {
    HIAI_ENGINE_LOG(HIAI_ENGINE_RUN_ARGS_NOT_RIGHT,
                    "call CreateOutputTensor failed");
    return false;
  }

  // 2. process
  HIAI_ENGINE_LOG("aiModelManager->Process start!");
  ret = ai_model_manager_->Process(ai_context, input_data_vec, output_data_vec,
                                   AI_MODEL_PROCESS_TIMEOUT);
  // process failed, also need to send data to post process
  if (ret != hiai::SUCCESS) {
    HIAI_ENGINE_LOG(HIAI_ENGINE_RUN_ARGS_NOT_RIGHT, "call Process failed");
    return false;
  }
  HIAI_ENGINE_LOG("aiModelManager->Process end!");
  return true;
}

bool FaceDetection::PostProcess(
  shared_ptr<FaceRecognitionInfo> &image_handle,
  const vector<shared_ptr<hiai::IAITensor>> &output_data_vec) {
  // inference result vector only need get first result
  // because batch is fixed as 1
  shared_ptr<hiai::AISimpleTensor> result_tensor = static_pointer_cast <
      hiai::AISimpleTensor > (output_data_vec[kResultIndex]);

  // copy data to float array
  int32_t size = result_tensor->GetSize() / sizeof(float);
  if(size <= 0){
    HIAI_ENGINE_LOG(HIAI_ENGINE_RUN_ARGS_NOT_RIGHT,
                    "the result tensor's size is not correct, size is %d", size);
    return false;
  }
  float result[size];
  errno_t mem_ret = memcpy_s(result, sizeof(result), result_tensor->GetBuffer(),
                             result_tensor->GetSize());
  if (mem_ret != EOK) {
    HIAI_ENGINE_LOG(HIAI_ENGINE_RUN_ARGS_NOT_RIGHT,
                    "post process call memcpy_s() error=%d", mem_ret);
    return false;
  }

  uint32_t width = image_handle->org_img.width;
  uint32_t height = image_handle->org_img.height;

  // every inference result needs 8 float
  // loop the result for every result
  float *ptr = result;
  for (int32_t i = 0; i < size - kEachResultSize; i += kEachResultSize) {
    ptr = result + i;
    // attribute
    float attr = ptr[kAttributeIndex];
    // confidence
    float score = ptr[kScoreIndex];

    // position
    FaceRectangle rectangle;
    rectangle.lt.x = CorrectionRatio(ptr[kLeftTopXaxisIndex]) * width;
    rectangle.lt.y = CorrectionRatio(ptr[kLeftTopYaxisIndex]) * height;
    rectangle.rb.x = CorrectionRatio(ptr[kRightBottomXaxisIndex]) * width;
    rectangle.rb.y = CorrectionRatio(ptr[kRightBottomYaxisIndex]) * height;

    // check results is invalid, skip it
    if (!IsValidResults(attr, score, rectangle)) {
      continue;
    }

    HIAI_ENGINE_LOG("attr=%f, score=%f, lt.x=%d, lt.y=%d, rb.x=%d, rb.y=%d",
                    attr, score, rectangle.lt.x, rectangle.lt.y, rectangle.rb.x,
                    rectangle.rb.y);

    // push back to image_handle
    FaceImage faceImage;
    faceImage.rectangle = rectangle;
    image_handle->face_imgs.emplace_back(faceImage);
  }
  return true;
}

void FaceDetection::HandleErrors(
  AppErrorCode err_code, const string &err_msg,
  shared_ptr<FaceRecognitionInfo> &image_handle) {
  // write error log
  HIAI_ENGINE_LOG(HIAI_ENGINE_RUN_ARGS_NOT_RIGHT, err_msg.c_str());

  // set error information
  image_handle->err_info.err_code = err_code;
  image_handle->err_info.err_msg = err_msg;

  // send data
  SendResult(image_handle);
}

void FaceDetection::SendResult(
  const shared_ptr<FaceRecognitionInfo> &image_handle) {

  // when register face, can not discard when queue full
  HIAI_StatusT hiai_ret;
  do {
    hiai_ret = SendData(kSendDataPort, "FaceRecognitionInfo",
                        static_pointer_cast<void>(image_handle));
    // when queue full, sleep
    if (hiai_ret == HIAI_QUEUE_FULL) {
      HIAI_ENGINE_LOG("queue full, sleep 200ms");
      usleep(kSleepInterval);
    }
  } while (hiai_ret == HIAI_QUEUE_FULL
           && image_handle->frame.image_source == kRegisterSrc);

  // send failed
  if (hiai_ret != HIAI_OK) {
    HIAI_ENGINE_LOG(HIAI_ENGINE_RUN_ARGS_NOT_RIGHT,
                    "call SendData failed, err_code=%d", hiai_ret);
  }
}

HIAI_StatusT FaceDetection::Detection(
  shared_ptr<FaceRecognitionInfo> &image_handle) {
  string err_msg = "";
  if (image_handle->err_info.err_code != AppErrorCode::kNone) {
    HIAI_ENGINE_LOG(HIAI_ENGINE_RUN_ARGS_NOT_RIGHT,
                    "front engine dealing failed, err_code=%d, err_msg=%s",
                    image_handle->err_info.err_code,
                    image_handle->err_info.err_msg.c_str());
    SendResult(image_handle);
    return HIAI_ERROR;
  }

  // resize image
  ImageData<u_int8_t> resized_image;
  if (!PreProcess(image_handle, resized_image)) {
    err_msg = "face_detection call ez_dvpp to resize image failed.";
    HandleErrors(AppErrorCode::kDetection, err_msg, image_handle);
    return HIAI_ERROR;
  }

  // inference
  vector<shared_ptr<hiai::IAITensor>> output_data;
  if (!Inference(resized_image, output_data)) {
    err_msg = "face_detection inference failed.";
    HandleErrors(AppErrorCode::kDetection, err_msg, image_handle);
    return HIAI_ERROR;
  }

  // post process
  if (!PostProcess(image_handle, output_data)) {
    err_msg = "face_detection deal result failed.";
    HandleErrors(AppErrorCode::kDetection, err_msg, image_handle);
    return HIAI_ERROR;
  }

  // send result
  SendResult(image_handle);
  return HIAI_OK;
}

HIAI_IMPL_ENGINE_PROCESS("face_detection",
                         FaceDetection, INPUT_SIZE) {
  HIAI_StatusT ret = HIAI_OK;

  // deal arg0 (camera input)
  if (arg0 != nullptr) {
    HIAI_ENGINE_LOG("camera input will be dealing!");
    shared_ptr<FaceRecognitionInfo> camera_img = static_pointer_cast <
        FaceRecognitionInfo > (arg0);
    ret = Detection(camera_img);
  }

  // deal arg1 (register input)
  if (arg1 != nullptr) {
    HIAI_ENGINE_LOG("register input will be dealing!");
    shared_ptr<FaceRecognitionInfo> register_img = static_pointer_cast <
        FaceRecognitionInfo > (arg1);
    ret = Detection(register_img);
  }
  return ret;
}
