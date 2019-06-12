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

#include "face_post_process.h"

#include <memory>
#include <fstream>
#include <sstream>
#include <cstdio>
#include <cstring>
#include <string>

#include "hiaiengine/log.h"
#include "hiaiengine/data_type_reg.h"
#include "ascenddk/ascend_ezdvpp/dvpp_process.h"

using namespace std;
using namespace ascend::presenter;
using namespace ascend::utils;
using namespace google::protobuf;

// register custom data type
HIAI_REGISTER_DATA_TYPE("FaceRecognitionInfo", FaceRecognitionInfo);
HIAI_REGISTER_DATA_TYPE("FaceRectangle", FaceRectangle);
HIAI_REGISTER_DATA_TYPE("FaceImage", FaceImage);

// constants
namespace {
// level for call DVPP
const int32_t kDvppToJpegLevel = 100;
}

HIAI_StatusT FacePostProcess::Init(
    const hiai::AIConfig &config,
    const std::vector<hiai::AIModelDescription> &model_desc) {
  // need do nothing
  return HIAI_OK;
}

HIAI_StatusT FacePostProcess::CheckSendMessageRes(
    const PresenterErrorCode &error_code) {
  if (error_code == PresenterErrorCode::kNone) {
    HIAI_ENGINE_LOG("send message to presenter server successfully.");
    return HIAI_OK;
  }

  HIAI_ENGINE_LOG(HIAI_ENGINE_RUN_ARGS_NOT_RIGHT,
                  "send message to presenter server failed. error=%d",
                  error_code);
  return HIAI_ERROR;
}

HIAI_StatusT FacePostProcess::SendFeature(
    const shared_ptr<FaceRecognitionInfo> &info) {
  // get channel for send feature (data from camera)
  Channel *channel = PresenterChannels::GetInstance().GetPresenterChannel();
  if (channel == nullptr) {
    HIAI_ENGINE_LOG(HIAI_ENGINE_RUN_ARGS_NOT_RIGHT,
                    "get channel for send FrameInfo failed.");
    return HIAI_ERROR;
  }

  // front engine deal failed, skip this frame
  if (info->err_info.err_code != AppErrorCode::kNone) {
    HIAI_ENGINE_LOG(
        HIAI_ENGINE_RUN_ARGS_NOT_RIGHT,
        "front engine dealing failed, skip this frame, err_code=%d, err_msg=%s",
        info->err_info.err_code, info->err_info.err_msg.c_str());
    return HIAI_ERROR;
  }

  // generate frame information
  // 1. JPEG image (call DVPP change NV12 to jpeg)
  DvppToJpgPara dvpp_to_jpeg_para;
  dvpp_to_jpeg_para.format = JPGENC_FORMAT_NV12;
  dvpp_to_jpeg_para.level = kDvppToJpegLevel;
  dvpp_to_jpeg_para.resolution.height = info->org_img.height;
  dvpp_to_jpeg_para.resolution.width = info->org_img.width;
  DvppProcess dvpp_to_jpeg(dvpp_to_jpeg_para);
  DvppOutput dvpp_output;
  int32_t ret = dvpp_to_jpeg.DvppOperationProc(
      reinterpret_cast<char*>(info->org_img.data.get()), info->org_img.size,
      &dvpp_output);
  // failed, no need to send to presenter
  if (ret != kDvppOperationOk) {
    HIAI_ENGINE_LOG(HIAI_ENGINE_RUN_ARGS_NOT_RIGHT,
                    "Failed to convert NV12 to JPEG, skip this frame.");
    return HIAI_ERROR;
  }

  facial_recognition::FrameInfo frame_info;
  frame_info.set_image(
      string(reinterpret_cast<char*>(dvpp_output.buffer), dvpp_output.size));
  delete[] dvpp_output.buffer;

  // 2. repeated FaceFeature
  vector<FaceImage> face_imgs = info->face_imgs;
  facial_recognition::FaceFeature *feature = nullptr;
  for (int i = 0; i < face_imgs.size(); i++) {
    // every face feature
    feature = frame_info.add_feature();

    // box
    feature->mutable_box()->set_lt_x(face_imgs[i].rectangle.lt.x);
    feature->mutable_box()->set_lt_y(face_imgs[i].rectangle.lt.y);
    feature->mutable_box()->set_rb_x(face_imgs[i].rectangle.rb.x);
    feature->mutable_box()->set_rb_y(face_imgs[i].rectangle.rb.y);

    HIAI_ENGINE_LOG("position is (%d,%d),(%d,%d)",face_imgs[i].rectangle.lt.x,face_imgs[i].rectangle.lt.y,face_imgs[i].rectangle.rb.x,face_imgs[i].rectangle.rb.y);

    // vector
    for (int j = 0; j < face_imgs[i].feature_vector.size(); j++) {
      feature->add_vector(face_imgs[i].feature_vector[j]);
    }
  }

  // send frame information to presenter server
  unique_ptr<Message> resp;
  PresenterErrorCode error_code = channel->SendMessage(frame_info, resp);
  return CheckSendMessageRes(error_code);
}

HIAI_StatusT FacePostProcess::ReplyFeature(
    const shared_ptr<FaceRecognitionInfo> &info) {
  // get channel for reply feature (data from register)
  Channel *channel = PresenterChannels::GetInstance().GetChannel();
  if (channel == nullptr) {
    HIAI_ENGINE_LOG(HIAI_ENGINE_RUN_ARGS_NOT_RIGHT,
                    "get channel for send FaceResult failed.");
    return HIAI_ERROR;
  }

  // generate FaceResult
  facial_recognition::FaceResult result;
  result.set_id(info->frame.face_id);
  unique_ptr<Message> resp;

  // 1. front engine dealing failed, send error message
  if (info->err_info.err_code != AppErrorCode::kNone) {
    HIAI_ENGINE_LOG(
        HIAI_ENGINE_RUN_ARGS_NOT_RIGHT,
        "front engine dealing failed, reply error response to server");

    result.mutable_response()->set_ret(facial_recognition::kErrorOther);
    result.mutable_response()->set_message(info->err_info.err_msg);

    // send
    PresenterErrorCode error_code = channel->SendMessage(result, resp);
    return CheckSendMessageRes(error_code);
  }

  // 2. dealing success, need set FaceFeature
  result.mutable_response()->set_ret(facial_recognition::kErrorNone);
  vector<FaceImage> face_imgs = info->face_imgs;
  facial_recognition::FaceFeature *face_feature = nullptr;
  for (int i = 0; i < face_imgs.size(); i++) {
    // every face feature
    face_feature = result.add_feature();

    // box
    face_feature->mutable_box()->set_lt_x(face_imgs[i].rectangle.lt.x);
    face_feature->mutable_box()->set_lt_y(face_imgs[i].rectangle.lt.y);
    face_feature->mutable_box()->set_rb_x(face_imgs[i].rectangle.rb.x);
    face_feature->mutable_box()->set_rb_y(face_imgs[i].rectangle.rb.y);

    // vector
    for (int j = 0; j < face_imgs[i].feature_vector.size(); j++) {
      face_feature->add_vector(face_imgs[i].feature_vector[j]);
    }
  }

  PresenterErrorCode error_code = channel->SendMessage(result, resp);
  return CheckSendMessageRes(error_code);
}

HIAI_IMPL_ENGINE_PROCESS("face_post_process", FacePostProcess, INPUT_SIZE) {

  // deal arg0 (engine only have one input)
  if (arg0 != nullptr) {
    shared_ptr<FaceRecognitionInfo> image_handle = static_pointer_cast<
        FaceRecognitionInfo>(arg0);

    // deal data from camera
    if (image_handle->frame.image_source == 0) {
      HIAI_ENGINE_LOG("post process dealing data from camera.");
      return SendFeature(image_handle);
    }

    // deal data from register
    HIAI_ENGINE_LOG("post process dealing data from register.");
    return ReplyFeature(image_handle);
  }
  HIAI_ENGINE_LOG(HIAI_ENGINE_RUN_ARGS_NOT_RIGHT, "arg0 is null!");
  return HIAI_ERROR;
}
