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

#include "ascenddk/ascendcamera/camera.h"

#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include <iostream>
#include "ascenddk/ascendcamera/ascend_camera_common.h"

using namespace std;

namespace ascend {
namespace ascendcamera {

Camera::Camera(const CameraPara& camera_para) {
  // Initialization instance
  camera_instance_para_ = camera_para;
  image_size_ = camera_instance_para_.resolution.width
      * camera_instance_para_.resolution.height * kYuv420spSizeNumerator
      / kYuv420spSizeDenominator;
  frame_id_ = 0;  //init framId = 0

}

Camera::~Camera() {
}

int Camera::InitCamera() {
  int ret = kCameraInitOk;

  // init driver of camera
  ret = MediaLibInit();
  if (ret == LIBMEDIA_STATUS_FAILED) {
    return kCameraMediaStatusError;
  }

  // check camera status
  CameraStatus status = QueryCameraStatus(camera_instance_para_.channel_id);
  if (status != CAMERA_STATUS_CLOSED) {
    ASC_LOG_ERROR("Camera[%d] is unavailable,status is %d.",
                  camera_instance_para_.channel_id, status);
    return kCameraStatusError;
  }

  // open camera
  ret = OpenCamera(camera_instance_para_.channel_id);
  if (ret == kCameraReturnInvalid) {
    ASC_LOG_ERROR("Camera[%d] open failed.", camera_instance_para_.channel_id);
    return kCameraOpenError;
  }

  // set fps
  ret = SetCameraProperty(camera_instance_para_.channel_id, CAMERA_PROP_FPS,
                          &(camera_instance_para_.fps));
  if (ret == kCameraReturnInvalid) {
    ASC_LOG_ERROR("Camera[%d] set fps[%d] failed.",
                  camera_instance_para_.channel_id, camera_instance_para_.fps);
    return kCameraSetFpsError;
  }

  // set image format
  ret = SetCameraProperty(camera_instance_para_.channel_id,
                          CAMERA_PROP_IMAGE_FORMAT,
                          &(camera_instance_para_.image_format));
  if (ret == kCameraReturnInvalid) {
    ASC_LOG_ERROR("Camera[%d] set image_fromat[%d] failed.",
                  camera_instance_para_.channel_id,
                  camera_instance_para_.image_format);
    return kCameraSetFormatError;
  }

  // set image resolution .
  ret = SetCameraProperty(camera_instance_para_.channel_id,
                          CAMERA_PROP_RESOLUTION,
                          &(camera_instance_para_.resolution));
  if (ret == kCameraReturnInvalid) {
    ASC_LOG_ERROR("Camera[%d] set resolution[%d x %d] failed.",
                  camera_instance_para_.channel_id,
                  camera_instance_para_.resolution.width,
                  camera_instance_para_.resolution.height);
    return kCameraSetResolutionError;
  }

  CameraCapMode camera_mode = CAMERA_CAP_ACTIVE;
  // set work mode
  ret = SetCameraProperty(camera_instance_para_.channel_id,
                          CAMERA_PROP_CAP_MODE, &(camera_mode));
  if (ret == kCameraReturnInvalid) {
    ASC_LOG_ERROR("Camera[%d] set cap mode[%d] failed.",
                  camera_instance_para_.channel_id, camera_mode);
    return kCameraSetWorkModeError;
  }

  return kCameraInitOk;
}

int Camera::CaptureCameraInfo(CameraOutputPara *output_para) {
  int ret = kCameraReturnValid;
  int result = kCameraRunOk;
  int size = image_size_;
  shared_ptr<char> data(new char[size], default_delete<char[]>());

  // read info from camera
  ret = ReadFrameFromCamera(camera_instance_para_.channel_id,
                            (void *) (data.get()), &size);

  if ((ret == kCameraReturnValid) && (size == image_size_)) {
    // success to get a frame from camera
    output_para->frame_id = ++frame_id_;
    output_para->timestamp = time(NULL);
    output_para->data = data;
    output_para->size = size;
    output_para->channel_id = camera_instance_para_.channel_id;
    output_para->resolution = camera_instance_para_.resolution;
    result = kCameraRunOk;
  } else {  // failed to read a frame data from camera
    CameraStatus status = QueryCameraStatus(camera_instance_para_.channel_id);
    ASC_LOG_ERROR("Camera[%d] get image failed status is %d.",
                  camera_instance_para_.channel_id, status);
    result = kCameraGetInfoError;
  }
  return result;
}

int Camera::GetChannelId() const {
  return camera_instance_para_.channel_id;
}

int Camera::GetUserTimeout() const {
  return camera_instance_para_.timeout;
}

void Camera::PrintErrorInfo(int code) const {

  static ErrorDescription camera_description[] = { { kCameraInitError,
      "Failed to initialize camera." }, { kCameraStatusError,
      "The camera status is not correct ." }, { kCameraOpenError,
      "Failed to open camera." }, { kCameraSetFpsError, "Failed to set fps." },
      { kCameraSetFormatError, "Failed to set format." }, {
          kCameraSetResolutionError, "Failed to set resolution." }, {
          kCameraSetWorkModeError, "Failed to set work mode." }, {
          kCameraGetInfoError, "Failed to get info from camera." }, };

  // find same errorcode and get error description
  int num = sizeof(camera_description) / sizeof(ErrorDescription);
  for (int i = 0; i < num; i++) {
    if (code == camera_description[i].code) {
      cerr << "[ERROR] " << camera_description[i].code_info.c_str() << endl;
      return;
    }
  }

  cerr << "[ERROR] Other error." << endl;
}

}
}
