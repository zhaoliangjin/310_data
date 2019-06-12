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

#include "face_register.h"
#include <memory>
#include <fstream>
#include <sstream>
#include <stdio.h>
#include <string.h>
#include <cmath>
#include <regex>
#include "hiaiengine/log.h"
#include "hiaiengine/data_type_reg.h"
#include "face_recognition_params.h"
#include "ascenddk/ascend_ezdvpp/dvpp_process.h"

using hiai::Engine;
using namespace hiai;
using namespace std;
using namespace ascend::presenter;
using namespace ascend::presenter::facial_recognition;

bool FaceRegister::IsInvalidIp(const string &ip) {
  regex re(kIpRegularExpression);
  smatch sm;
  return !regex_match(ip, sm, re);
}

bool FaceRegister::IsInvalidPort(const string &port) {
  if (port == "") {
    return true;
  }
  for (auto &c : port) {
    if (c >= '0' && c <= '9') {
      continue;
    } else {
      return true;
    }
  }
  int port_tmp = atoi(port.data());
  return (port_tmp < kPortMinNumber) || (port_tmp > kPortMaxNumber);
}

bool FaceRegister::IsInvalidAppName(const string &app_name) {
  regex re(kAppNameRegularExpression);
  smatch sm;
  return !regex_match(app_name, sm, re);
}

HIAI_StatusT FaceRegister::Init(
    const hiai::AIConfig &config,
    const std::vector<hiai::AIModelDescription> &model_desc) {
  PresenterServerParams register_param;

  // get engine config and copy to register_param
  for (int index = 0; index < config.items_size(); index++) {
    const ::hiai::AIConfigItem& item = config.items(index);
    string name = item.name();
    string value = item.value();

    if (name == kPresenterServerIP) {
      // validate presenter server IP
      if (IsInvalidIp(value)) {
        HIAI_ENGINE_LOG(HIAI_GRAPH_INVALID_VALUE,
                        "host_ip = %s which configured is invalid.",
                        value.c_str());
        return HIAI_ERROR;
      }
      register_param.host_ip = value;
    } else if (name == kPresenterServerPort) {
      // validate presenter server port
      if (IsInvalidPort(value)) {
        HIAI_ENGINE_LOG(HIAI_GRAPH_INVALID_VALUE,
                        "port = %s which configured is invalid.",
                        value.c_str());
        return HIAI_ERROR;
      }
      register_param.port = atoi(value.data());
    } else if (name == kAppName) {
      // validate app name
      if (IsInvalidAppName(value)) {
        HIAI_ENGINE_LOG(HIAI_GRAPH_INVALID_VALUE,
                        "app_name = %s which configured is invalid.",
                        value.c_str());
        return HIAI_ERROR;
      }
      register_param.app_id = value;
    } else {
      HIAI_ENGINE_LOG("unused config name: %s", name.c_str());
    }
  }
  register_param.app_type = kAppType;
  HIAI_ENGINE_LOG("host_ip = %s,port = %d,app_name = %s",
                  register_param.host_ip.c_str(), register_param.port,
                  register_param.app_id.c_str());

  // Init class PresenterChannels by configs
  PresenterChannels::GetInstance().Init(register_param);

  // create agent channel and register app
  Channel* agent_channel = PresenterChannels::GetInstance().GetChannel();
  if (agent_channel == nullptr) {
    HIAI_ENGINE_LOG(HIAI_ENGINE_RUN_ARGS_NOT_RIGHT, "register app failed.");
    return HIAI_ERROR;
  }

  return HIAI_OK;
}

bool FaceRegister::DoRegisterProcess() {
  // get agent channel
  Channel* agent_channel = PresenterChannels::GetInstance().GetChannel();
  if (agent_channel == nullptr) {
    HIAI_ENGINE_LOG(HIAI_ENGINE_RUN_ARGS_NOT_RIGHT,
                    "get agent channel to send failed.");
    return false;
  }

  HIAI_StatusT hiai_ret = HIAI_OK;
  while (1) {
    // construct registered request Message and read message
    unique_ptr < google::protobuf::Message > response_rec;
    PresenterErrorCode agent_ret = agent_channel->ReceiveMessage(response_rec);
    if (agent_ret == PresenterErrorCode::kNone) {
      FaceInfo* face_register_req = (FaceInfo*) (response_rec.get());
      if (face_register_req == nullptr) {
        HIAI_ENGINE_LOG(HIAI_ENGINE_RUN_ARGS_NOT_RIGHT,
                        "[DoRegisterProcess]face_register_req is nullptr");
        continue;
      }

      // get face id and image
      uint32_t face_image_size = face_register_req->image().size();
      const char* face_image_buff = face_register_req->image().c_str();
      int face_id_size = face_register_req->id().size();
      HIAI_ENGINE_LOG("face_id = %s,face_image_size = %d,face_id_size = %d",
                      face_register_req->id().c_str(), face_image_size,
                      face_id_size);

      // ready to send registered info to next engine
      shared_ptr < FaceRecognitionInfo > pobj =
          make_shared<FaceRecognitionInfo>();

      // judge size of face id
      if (face_id_size > kMaxFaceIdLength) {
        HIAI_ENGINE_LOG(HIAI_ENGINE_RUN_ARGS_NOT_RIGHT,
                        "[DoRegisterProcess]length of face_id beyond range");
        pobj->err_info.err_code = AppErrorCode::kRegister;
        pobj->err_info.err_msg = "length of face_id beyond range";

        // send description information to next engine
        do {
          hiai_ret = SendData(0, "FaceRecognitionInfo",
                              static_pointer_cast<void>(pobj));

          // when queue full, sleep
          if (hiai_ret == HIAI_QUEUE_FULL) {
            HIAI_ENGINE_LOG("[DoRegisterProcess]queue full, sleep 200ms");
            usleep (kSleepInterval);
          }
        } while (hiai_ret == HIAI_QUEUE_FULL);
        if (hiai_ret != HIAI_OK) {
          HIAI_ENGINE_LOG(HIAI_ENGINE_RUN_ARGS_NOT_RIGHT,
                          "[DoRegisterProcess] SendData failed.");
        }
        continue;
      }

      // use dvpp convert jpg image to yuv
      ascend::utils::DvppJpegDInPara dvpp_to_yuv_para;
      dvpp_to_yuv_para.is_convert_yuv420 = true;
      ascend::utils::DvppProcess dvpp_yuv_process(dvpp_to_yuv_para);
      ascend::utils::DvppJpegDOutput dvpp_out_data = { 0 };
      int ret_dvpp = dvpp_yuv_process.DvppJpegDProc(face_image_buff,
                                                    face_image_size,
                                                    &dvpp_out_data);
      if (ret_dvpp != kDvppOperationOk) {
        HIAI_ENGINE_LOG(
            HIAI_ENGINE_RUN_ARGS_NOT_RIGHT,
            "[DoRegisterProcess]fail to convert jpg to yuv,ret_dvpp = %d",
            ret_dvpp);
        pobj->err_info.err_code = AppErrorCode::kRegister;
        pobj->err_info.err_msg = "fail to convert jpg to yuv";

        // send description information to next engine
        do {
          hiai_ret = SendData(0, "FaceRecognitionInfo",
                              static_pointer_cast<void>(pobj));

          // when queue full, sleep
          if (hiai_ret == HIAI_QUEUE_FULL) {
            HIAI_ENGINE_LOG("[DoRegisterProcess]queue full, sleep 200ms");
            usleep (kSleepInterval);
          }
        } while (hiai_ret == HIAI_QUEUE_FULL);
        if (hiai_ret != HIAI_OK) {
          HIAI_ENGINE_LOG(HIAI_ENGINE_RUN_ARGS_NOT_RIGHT,
                          "[DoRegisterProcess] SendData failed.");
        }
        continue;
      }

      // 1 indicate the image from register
      pobj->frame.image_source = 1;
      pobj->frame.face_id = face_register_req->id();
      pobj->frame.org_img_format = dvpp_out_data.image_format;
      pobj->frame.org_img_rank = ascend::utils::kVpcNv12;
      // true indicate the image is aligned
      pobj->frame.img_aligned = true;

      // set up origin image infomation, change the width and height to odd
      pobj->org_img.width =
          (dvpp_out_data.width % 2 == 0) ?
              dvpp_out_data.width : (dvpp_out_data.width - 1);
      pobj->org_img.height =
          (dvpp_out_data.height % 2 == 0) ?
              dvpp_out_data.height : (dvpp_out_data.height - 1);
      pobj->org_img.height_step = dvpp_out_data.aligned_height;
      pobj->org_img.width_step = dvpp_out_data.aligned_width;
      pobj->org_img.size = dvpp_out_data.buffer_size;
      pobj->org_img.data.reset(dvpp_out_data.buffer,
                               default_delete<u_int8_t[]>());
      pobj->err_info.err_code = AppErrorCode::kNone;

      // send registered infomation to next engine
      do {
        hiai_ret = SendData(0, "FaceRecognitionInfo",
                            static_pointer_cast<void>(pobj));

        // when queue full, sleep
        if (hiai_ret == HIAI_QUEUE_FULL) {
          HIAI_ENGINE_LOG("queue full, sleep 200ms");
          usleep (kSleepInterval);
        }
      } while (hiai_ret == HIAI_QUEUE_FULL);
      if (hiai_ret != HIAI_OK) {
        HIAI_ENGINE_LOG(HIAI_ENGINE_RUN_ARGS_NOT_RIGHT,
                        "[FaceRegister] SendData failed.");
      }
    } else {
      usleep (kSleepInterval);
    }
  }
}

HIAI_IMPL_ENGINE_PROCESS("face_register", FaceRegister, INPUT_SIZE)
{
  if (!DoRegisterProcess()) {
    HIAI_ENGINE_LOG(HIAI_ENGINE_RUN_ARGS_NOT_RIGHT,
        "DoRegisterProcess failed.");
  }
  return HIAI_OK;
}
