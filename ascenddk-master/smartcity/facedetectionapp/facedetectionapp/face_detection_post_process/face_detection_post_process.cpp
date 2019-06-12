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
#include "face_detection_post_process.h"
#include <vector>
#include <sstream>
#include <cmath>
#include <regex>
#include "hiaiengine/log.h"

using hiai::Engine;
using namespace ascend::presenter;

// register data type
HIAI_REGISTER_DATA_TYPE("EngineTransT", EngineTransT);
HIAI_REGISTER_DATA_TYPE("OutputT", OutputT);
HIAI_REGISTER_DATA_TYPE("ScaleInfoT", ScaleInfoT);
HIAI_REGISTER_DATA_TYPE("NewImageParaT", NewImageParaT);
HIAI_REGISTER_DATA_TYPE("BatchImageParaWithScaleT", BatchImageParaWithScaleT);

// constants
namespace {
//// parameters for drawing box and label begin////
// face box color
const uint8_t kFaceBoxColorR = 255;
const uint8_t kFaceBoxColorG = 190;
const uint8_t kFaceBoxColorB = 0;

// face box border width
const int kFaceBoxBorderWidth = 2;

// face label color
const uint8_t kFaceLabelColorR = 255;
const uint8_t kFaceLabelColorG = 255;
const uint8_t kFaceLabelColorB = 0;

// face label font
const double kFaceLabelFontSize = 0.7;
const int kFaceLabelFontWidth = 2;

// face label text prefix
const std::string kFaceLabelTextPrefix = "Face:";
const std::string kFaceLabelTextSuffix = "%";
//// parameters for drawing box and label end////

// port number range
const int32_t kPortMinNumber = 0;
const int32_t kPortMaxNumber = 65535;

// confidence range
const float kConfidenceMin = 0.0;
const float kConfidenceMax = 1.0;

// face detection function return value
const int32_t kFdFunSuccess = 0;
const int32_t kFdFunFailed = -1;

// OSD function return value
const int32_t kOsdCallSuccess = 0;

// DVPP function return value
const int32_t kDvppCallSuccess = 0;

// level for call DVPP
const int32_t kDvppToJpegLevel = 100;

// need to deal results when index is 2
const int32_t kDealResultIndex = 2;

// each results size
const int32_t kEachResultSize = 7;

// attribute index
const int32_t kAttributeIndex = 1;

// score index
const int32_t kScoreIndex = 2;

// anchor_lt.x index
const int32_t kAnchorLeftTopAxisIndexX = 3;

// anchor_lt.y index
const int32_t kAnchorLeftTopAxisIndexY = 4;

// anchor_rb.x index
const int32_t kAnchorRightBottomAxisIndexX = 5;

// anchor_rb.y index
const int32_t kAnchorRightBottomAxisIndexY = 6;

// face attribute
const float kAttributeFaceLabelValue = 1.0;
const float kAttributeFaceDeviation = 0.00001;

// percent
const int32_t kScorePercent = 100;

// IP regular expression
const std::string kIpRegularExpression =
    "^((25[0-5]|2[0-4]\\d|[1]{1}\\d{1}\\d{1}|[1-9]{1}\\d{1}|\\d{1})($|(?!\\.$)\\.)){4}$";

// channel name regular expression
const std::string kChannelNameRegularExpression = "[a-zA-Z0-9/]+";
}

FaceDetectionPostProcess::FaceDetectionPostProcess() {
  fd_post_process_config_ = nullptr;
  presenter_channel_ = nullptr;
}

HIAI_StatusT FaceDetectionPostProcess::Init(
    const hiai::AIConfig& config,
    const std::vector<hiai::AIModelDescription>& model_desc) {
  HIAI_ENGINE_LOG("Begin initialize!");

  // get configurations
  if (fd_post_process_config_ == nullptr) {
    fd_post_process_config_ = std::make_shared<FaceDetectionPostConfig>();
  }

  // get parameters from graph.config
  for (int index = 0; index < config.items_size(); index++) {
    const ::hiai::AIConfigItem& item = config.items(index);
    const std::string& name = item.name();
    const std::string& value = item.value();
    std::stringstream ss;
    ss << value;
    if (name == "Confidence") {
      ss >> (*fd_post_process_config_).confidence;
      // validate confidence
      if (IsInvalidConfidence(fd_post_process_config_->confidence)) {
        HIAI_ENGINE_LOG(HIAI_GRAPH_INVALID_VALUE,
                        "Confidence=%s which configured is invalid.",
                        value.c_str());
        return HIAI_ERROR;
      }
    } else if (name == "PresenterIp") {
      // validate presenter server IP
      if (IsInValidIp(value)) {
        HIAI_ENGINE_LOG(HIAI_GRAPH_INVALID_VALUE,
                        "PresenterIp=%s which configured is invalid.",
                        value.c_str());
        return HIAI_ERROR;
      }
      ss >> (*fd_post_process_config_).presenter_ip;
    } else if (name == "PresenterPort") {
      ss >> (*fd_post_process_config_).presenter_port;
      // validate presenter server port
      if (IsInValidPort(fd_post_process_config_->presenter_port)) {
        HIAI_ENGINE_LOG(HIAI_GRAPH_INVALID_VALUE,
                        "PresenterPort=%s which configured is invalid.",
                        value.c_str());
        return HIAI_ERROR;
      }
    } else if (name == "ChannelName") {
      // validate channel name
      if (IsInValidChannelName(value)) {
        HIAI_ENGINE_LOG(HIAI_GRAPH_INVALID_VALUE,
                        "ChannelName=%s which configured is invalid.",
                        value.c_str());
        return HIAI_ERROR;
      }
      ss >> (*fd_post_process_config_).channel_name;
    }
    // else : nothing need to do
  }

  // call presenter agent, create connection to presenter server
  uint16_t u_port = static_cast<uint16_t>(fd_post_process_config_
      ->presenter_port);
  OpenChannelParam channel_param = { fd_post_process_config_->presenter_ip,
      u_port, fd_post_process_config_->channel_name, ContentType::kVideo };
  Channel *chan = nullptr;
  PresenterErrorCode err_code = OpenChannel(chan, channel_param);
  // open channel failed
  if (err_code != PresenterErrorCode::kNone) {
    HIAI_ENGINE_LOG(HIAI_GRAPH_INIT_FAILED,
                    "Open presenter channel failed, error code=%d", err_code);
    return HIAI_ERROR;
  }

  presenter_channel_.reset(chan);
  HIAI_ENGINE_LOG(HIAI_DEBUG_INFO, "End initialize!");
  return HIAI_OK;
}

bool FaceDetectionPostProcess::IsInValidIp(const std::string &ip) {
  regex re(kIpRegularExpression);
  smatch sm;
  return !regex_match(ip, sm, re);
}

bool FaceDetectionPostProcess::IsInValidPort(int32_t port) {
  return (port <= kPortMinNumber) || (port > kPortMaxNumber);
}

bool FaceDetectionPostProcess::IsInValidChannelName(
    const std::string &channel_name) {
  regex re(kChannelNameRegularExpression);
  smatch sm;
  return !regex_match(channel_name, sm, re);
}

bool FaceDetectionPostProcess::IsInvalidConfidence(float confidence) {
  return (confidence <= kConfidenceMin) || (confidence > kConfidenceMax);
}

bool FaceDetectionPostProcess::IsSupportFormat(hiai::IMAGEFORMAT format) {
  return format == hiai::YUV420SP;
}

bool FaceDetectionPostProcess::IsInvalidResults(float attr, float score,
                                                const Point &point_lt,
                                                const Point &point_rb) {
  // attribute is not face (background)
  if (std::abs(attr - kAttributeFaceLabelValue) > kAttributeFaceDeviation) {
    return true;
  }

  // confidence check
  if ((score < fd_post_process_config_->confidence)
      || IsInvalidConfidence(score)) {
    return true;
  }

  // rectangle position is a point or not: lt == rb
  if ((point_lt.x == point_rb.x) && (point_lt.y == point_rb.y)) {
    return true;
  }
  return false;
}

int32_t FaceDetectionPostProcess::SendImage(uint32_t height, uint32_t width,
                                            uint32_t size, u_int8_t *data, std::vector<DetectionResult>& detection_results) {
  // parameter
  int32_t status = kFdFunSuccess;
  ascend::utils::DvppToJpgPara dvpp_to_jpeg_para;
  dvpp_to_jpeg_para.format = JPGENC_FORMAT_NV12;
  dvpp_to_jpeg_para.level = kDvppToJpegLevel;
  dvpp_to_jpeg_para.resolution.height = height;
  dvpp_to_jpeg_para.resolution.width = width;
  ascend::utils::DvppProcess dvpp_to_jpeg(dvpp_to_jpeg_para);

  // call DVPP
  ascend::utils::DvppOutput dvpp_output;
  int32_t ret = dvpp_to_jpeg.DvppOperationProc(reinterpret_cast<char*>(data),
                                               size, &dvpp_output);
  // failed, no need to send to presenter
  if (ret != kDvppCallSuccess) {
    HIAI_ENGINE_LOG(HIAI_ENGINE_RUN_ARGS_NOT_RIGHT,
                    "Failed to convert YUV420SP to JPEG, skip it.");
    status = kFdFunFailed;
  } else {  // success, need sent jpeg to presenter
    ImageFrame image_frame_para;
    image_frame_para.format = ImageFormat::kJpeg;
    image_frame_para.width = width;
    image_frame_para.height = height;
    image_frame_para.size = dvpp_output.size;
    image_frame_para.data = dvpp_output.buffer;
    image_frame_para.detection_results = detection_results;

    PresenterErrorCode p_ret = PresentImage(presenter_channel_.get(),
                                            image_frame_para);
    // send to presenter failed
    if (p_ret != PresenterErrorCode::kNone) {
      HIAI_ENGINE_LOG(HIAI_ENGINE_RUN_ARGS_NOT_RIGHT,
                      "Send JPEG image to presenter failed, error code=%d",
                      p_ret);
      status = kFdFunFailed;
    }

    // delete data which DVPP new (call DvppOperationProc function)
    delete[] dvpp_output.buffer;
  }
  return status;
}

HIAI_StatusT FaceDetectionPostProcess::HandleOriginalImage(
    const std::shared_ptr<EngineTransT> &inference_res) {
  HIAI_StatusT status = HIAI_OK;
  std::vector<NewImageParaT> img_vec = inference_res->imgs;
  // dealing every original image
  for (uint32_t ind = 0; ind < inference_res->b_info.batch_size; ind++) {
    hiai::IMAGEFORMAT format = img_vec[ind].img.format;
    // format invalid, skip it
    if (!IsSupportFormat(format)) {
      HIAI_ENGINE_LOG(HIAI_ENGINE_RUN_ARGS_NOT_RIGHT,
                      "Only support YUV420SP format, index=%d, format=%d", ind,
                      format);
      status = HIAI_ERROR;
      continue;
    }
    uint32_t width = img_vec[ind].img.width;
    uint32_t height = img_vec[ind].img.height;
    uint32_t size = img_vec[ind].img.size;

    // call SendImage
    // 1. call DVPP to change YUV420SP image to JPEG
    // 2. send image to presenter
    vector<DetectionResult> detection_results;
    int32_t ret = SendImage(height, width, size, img_vec[ind].img.data.get(), detection_results);
    if (ret == kFdFunFailed) {
      status = HIAI_ERROR;
      continue;
    }
  }
  return status;
}

HIAI_StatusT FaceDetectionPostProcess::HandleResults(
    const std::shared_ptr<EngineTransT> &inference_res) {
  HIAI_StatusT status = HIAI_OK;
  std::vector<NewImageParaT> img_vec = inference_res->imgs;
  std::vector<OutputT> output_data_vec = inference_res->output_datas;
  // dealing every image
  for (uint32_t ind = 0; ind < inference_res->b_info.batch_size; ind++) {
    // get origin image parameters
    hiai::IMAGEFORMAT format = img_vec[ind].img.format;
    // format invalid, skip it
    if (!IsSupportFormat(format)) {
      HIAI_ENGINE_LOG(HIAI_ENGINE_RUN_ARGS_NOT_RIGHT,
                      "Only support YUV420SP format, index=%d, format=%d", ind,
                      format);
      status = HIAI_ERROR;
      continue;
    }

    // result
    int32_t out_index = ind * kDealResultIndex;
    OutputT out = output_data_vec[out_index];
    std::shared_ptr<hiai::AISimpleTensor> result_tensor = std::make_shared<
        hiai::AISimpleTensor>();
    result_tensor->SetBuffer(out.data.get(), out.size);
    int32_t size = result_tensor->GetSize() / sizeof(float);
    float result[size];
    errno_t mem_ret = memcpy_s(result, sizeof(result),
                               result_tensor->GetBuffer(),
                               result_tensor->GetSize());
    // memory copy failed, skip this image
    if (mem_ret != EOK) {
      HIAI_ENGINE_LOG(HIAI_ENGINE_RUN_ARGS_NOT_RIGHT,
                      "handle results: memcpy_s() error=%d", mem_ret);
      continue;
    }

    uint32_t width = img_vec[ind].img.width;
    uint32_t height = img_vec[ind].img.height;
    uint32_t img_size = img_vec[ind].img.size;

    // every inference result needs 8 float
    // loop the result for every inference result
    std::vector<DetectionResult> detection_results;
    float *ptr = result;
    for (int32_t k = 0; k < size - kEachResultSize; k += kEachResultSize) {
      ptr = result + k;
      // attribute
      float attr = ptr[kAttributeIndex];
      // confidence
      float score = ptr[kScoreIndex];

      //Detection result
      DetectionResult one_result;
      // left top
      Point point_lt, point_rb;
      point_lt.x = ptr[kAnchorLeftTopAxisIndexX] * width;
      point_lt.y = ptr[kAnchorLeftTopAxisIndexY] * height;
      // right bottom
      point_rb.x = ptr[kAnchorRightBottomAxisIndexX] * width;
      point_rb.y = ptr[kAnchorRightBottomAxisIndexY] * height;

      one_result.lt = point_lt;
      one_result.rb = point_rb;

      // check results is valid
      if (IsInvalidResults(attr, score, point_lt, point_rb)) {
        continue;
      }
      HIAI_ENGINE_LOG(HIAI_DEBUG_INFO,
                      "score=%f, lt.x=%d, lt.y=%d, rb.x=%d, rb.y=%d", score,
                      point_lt.x, point_lt.y, point_rb.x, point_rb.y);
      int32_t score_percent =  score * kScorePercent;
      one_result.result_text.append(kFaceLabelTextPrefix);
      one_result.result_text.append(to_string(score_percent));
      one_result.result_text.append(kFaceLabelTextSuffix);

      // push back
      detection_results.emplace_back(one_result);
    }

    int32_t ret;
    ret = SendImage(height, width, img_size, img_vec[ind].img.data.get(), detection_results);

    // check send result
    if (ret == kFdFunFailed) {
      status = HIAI_ERROR;
    }
  }
  return status;
}

HIAI_IMPL_ENGINE_PROCESS("face_detection_post_process",
    FaceDetectionPostProcess, INPUT_SIZE) {
  // check arg0 is null or not
  if (arg0 == nullptr) {
    HIAI_ENGINE_LOG(HIAI_ENGINE_RUN_ARGS_NOT_RIGHT,
                    "Failed to process invalid message.");
    return HIAI_ERROR;
  }

  // check original image is empty or not
  std::shared_ptr<EngineTransT> inference_res = std::static_pointer_cast<
      EngineTransT>(arg0);
  if (inference_res->imgs.empty()) {
    HIAI_ENGINE_LOG(
        HIAI_ENGINE_RUN_ARGS_NOT_RIGHT,
        "Failed to process invalid message, original image is null.");
    return HIAI_ERROR;
  }

  // inference failed, dealing original images
  if (!inference_res->status) {
    HIAI_ENGINE_LOG(HIAI_OK, inference_res->msg.c_str());
    HIAI_ENGINE_LOG(HIAI_OK, "will handle original image.");
    return HandleOriginalImage(inference_res);
  }

  // inference success, dealing inference results
  return HandleResults(inference_res);
}
