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

#include "video_analysis_post.h"
#include <memory>
#include <fstream>
#include <unistd.h>
#include <sstream>
#include <iostream>
#include <stdio.h>
#include <string.h>
#include <cmath>
#include <regex>
#include "hiaiengine/log.h"
#include "ascenddk/ascend_ezdvpp/dvpp_process.h"

using hiai::Engine;
using namespace hiai;
using namespace std;
using namespace ascend::presenter;
using namespace ascend::presenter::video_analysis;

// register data type
HIAI_REGISTER_DATA_TYPE("VideoImageInfoT", VideoImageInfoT);
HIAI_REGISTER_DATA_TYPE("VideoImageParaT", VideoImageParaT);
HIAI_REGISTER_DATA_TYPE("ObjectInfoT", ObjectInfoT);
HIAI_REGISTER_DATA_TYPE("ObjectImageParaT", ObjectImageParaT);
HIAI_REGISTER_DATA_TYPE("VideoDetectionImageParaT", VideoDetectionImageParaT);
HIAI_REGISTER_DATA_TYPE("BatchCroppedImageParaT", BatchCroppedImageParaT);
HIAI_REGISTER_DATA_TYPE("CarInfoT", CarInfoT);
HIAI_REGISTER_DATA_TYPE("BatchCarInfoT", BatchCarInfoT);
HIAI_REGISTER_DATA_TYPE("OutputT", OutputT);
HIAI_REGISTER_DATA_TYPE("DetectionEngineTransT", DetectionEngineTransT);
HIAI_REGISTER_DATA_TYPE("PedestrianInfoT", PedestrianInfoT);
HIAI_REGISTER_DATA_TYPE("BatchPedestrianInfoT", BatchPedestrianInfoT);

VideoAnalysisPost::~VideoAnalysisPost() {
  if (agent_channel_ != nullptr) {
    delete agent_channel_;
  }
}

bool VideoAnalysisPost::IsInvalidIp(const std::string &ip) {
  regex re(kIpRegularExpression);
  smatch sm;
  return !regex_match(ip, sm, re);
}

bool VideoAnalysisPost::IsInvalidPort(const string &port) {
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

bool VideoAnalysisPost::IsInvalidAppName(const std::string &app_name) {
  regex re(kAppNameRegularExpression);
  smatch sm;
  return !regex_match(app_name, sm, re);
}

HIAI_StatusT VideoAnalysisPost::Init(
    const hiai::AIConfig &config,
    const std::vector<hiai::AIModelDescription> &model_desc) {
  if (app_config_ == nullptr) {
    app_config_ = make_shared<RegisterAppParam>();
  }

  // get engine config and save to app_config_
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
      app_config_->host_ip = value;
    } else if (name == kPresenterServerPort) {
      // validate presenter server port
      if (IsInvalidPort(value)) {
        HIAI_ENGINE_LOG(HIAI_GRAPH_INVALID_VALUE,
                        "port = %s which configured is invalid.",
                        value.c_str());
        return HIAI_ERROR;
      }
      app_config_->port = atoi(value.data());
    } else if (name == kAppName) {
      // validate app name
      if (IsInvalidAppName(value)) {
        HIAI_ENGINE_LOG(HIAI_GRAPH_INVALID_VALUE,
                        "app_name = %s which configured is invalid.",
                        value.c_str());
        return HIAI_ERROR;
      }
      app_config_->app_name = value;
    } else {
      HIAI_ENGINE_LOG("unused config name: %s", name.c_str());
    }
  }
  app_config_->app_type = kAppType;
  HIAI_ENGINE_LOG("host_ip = %s,port = %d,app_name = %s",
                  app_config_->host_ip.c_str(), app_config_->port,
                  app_config_->app_name.c_str());

  // create agent channel by host_ip and port
  ChannelFactory channel_factory;
  agent_channel_ = channel_factory.NewChannel(app_config_->host_ip,
                                              app_config_->port);

  //open present channel
  PresenterErrorCode present_open_err = agent_channel_->Open();
  if (present_open_err != PresenterErrorCode::kNone) {
    HIAI_ENGINE_LOG(HIAI_ENGINE_RUN_ARGS_NOT_RIGHT,
                    "open presenter channel failed, error code=%d",
                    present_open_err);
    return HIAI_ERROR;
  }

  // register app to presenter server
  // construct registered messages
  RegisterApp app_register;
  app_register.set_id(app_config_->app_name);
  app_register.set_type(app_config_->app_type);

  // construct responded protobuf Message
  unique_ptr < google::protobuf::Message > response;

  // send registered request to server
  PresenterErrorCode present_register_err = agent_channel_->SendMessage(
      app_register, response);
  if (present_register_err != PresenterErrorCode::kNone) {
    HIAI_ENGINE_LOG(HIAI_ENGINE_RUN_ARGS_NOT_RIGHT,
                    "send agent failed, error code=%d", present_register_err);
    return HIAI_ERROR;
  }

  // get responded Message and judge result
  CommonResponse* register_response =
      dynamic_cast<CommonResponse*>(response.get());
  if (register_response == nullptr) {
    HIAI_ENGINE_LOG(HIAI_ENGINE_RUN_ARGS_NOT_RIGHT,
                    "register_response is nullptr");
    return HIAI_ERROR;
  }

  ErrorCode register_err = register_response->ret();
  if (register_err != kErrorNone) {
    HIAI_ENGINE_LOG(HIAI_ENGINE_RUN_ARGS_NOT_RIGHT,
                    "register app failed, error code=%d", register_err);
    return HIAI_ERROR;
  }

  return HIAI_OK;
}

OperationCode VideoAnalysisPost::SendDetectionImage(
    const shared_ptr<VideoDetectionImageParaT> &image_para) {
  if (image_para == nullptr) {
    return kInvalidParam;
  }

  // exit app when data has been transferred
  if (image_para->image.video_image_info.is_finished) {
    return kExitApp;
  }

  // Construct Message ImageSet,which has FrameIndex,image and Object
  ImageSet image_set;
  FrameIndex* frame_image = new FrameIndex();

  // fill content of FrameIndex
  frame_image->set_app_id(app_config_->app_name);
  frame_image->set_channel_id(image_para->image.video_image_info.channel_id);
  frame_image->set_channel_name(
      image_para->image.video_image_info.channel_name);
  frame_image->set_frame_id(
      to_string(image_para->image.video_image_info.frame_id));
  image_set.set_allocated_frame_index(frame_image);

  // set up origin image buff in ImageSet Message
  image_set.set_frame_image(
      string(reinterpret_cast<char*>(image_para->image.img.data.get()),
             image_para->image.img.size));

  // get small images after reasoning
  for (vector<ObjectImageParaT>::iterator iter = image_para->obj_imgs.begin();
      iter != image_para->obj_imgs.end(); ++iter) {
    // set up id and confidence of small images
    Object* object_img = image_set.add_object();
    object_img->set_id(iter->object_info.object_id);
    object_img->set_confidence(iter->object_info.score);

    // set up small image buff in ImageSet Message
    object_img->set_image(
        string(reinterpret_cast<char*>(iter->img.data.get()), iter->img.size));
  }

  // construct callback Messages
  unique_ptr < google::protobuf::Message > response_detection;

  // send infomation images to server
  PresenterErrorCode detection_image_err = agent_channel_->SendMessage(
      image_set, response_detection);
  if (detection_image_err != PresenterErrorCode::kNone) {
    HIAI_ENGINE_LOG(HIAI_ENGINE_RUN_ARGS_NOT_RIGHT,
                    "send detection image failed, error code=%d",
                    detection_image_err);
    return kSendDataFailed;
  }

  // get responded Message and judge result
  CommonResponse* detection_image_response =
      dynamic_cast<CommonResponse*>(response_detection.get());
  if (detection_image_response == nullptr) {
    HIAI_ENGINE_LOG(HIAI_ENGINE_RUN_ARGS_NOT_RIGHT,
                    "[SendDetectionImage]detection_image_response is nullptr");
    return kSendDataFailed;
  }

  ErrorCode response_code = detection_image_response->ret();
  if (response_code != kErrorNone) {
    HIAI_ENGINE_LOG(HIAI_ENGINE_RUN_ARGS_NOT_RIGHT,
                    "[SendDetectionImage]server response failed, error code=%d",
                    response_code);
    return kSendDataFailed;
  }

  return kOperationOk;
}

OperationCode VideoAnalysisPost::SendCarInfo(
    const shared_ptr<BatchCarInfoT> &car_info_para) {
  if (car_info_para == nullptr) {
    return kInvalidParam;
  }

  // exit app when data has been transferred
  if (car_info_para->video_image_info.is_finished) {
    return kExitApp;
  }

  for (vector<CarInfoT>::iterator iter = car_info_para->car_infos.begin();
      iter != car_info_para->car_infos.end(); ++iter) {
    // Construct Message CarInferenceResult,which has FrameIndex,object_id,
    // CarInferenceType,confidence and value
    CarInferenceResult car_result;
    FrameIndex* frame_car = new FrameIndex();

    // set up relevant infomations of origin image
    frame_car->set_app_id(app_config_->app_name);
    frame_car->set_channel_id(car_info_para->video_image_info.channel_id);
    frame_car->set_channel_name(car_info_para->video_image_info.channel_name);
    frame_car->set_frame_id(
        to_string(car_info_para->video_image_info.frame_id));
    car_result.set_allocated_frame_index(frame_car);

    // set up relevant infomations of small image
    car_result.set_object_id(iter->object_id);
    if (iter->attribute_name == kCarType) {
      car_result.set_type(ascend::presenter::video_analysis::kCarBrand);
    } else {
      car_result.set_type(ascend::presenter::video_analysis::kCarColor);
    }
    car_result.set_confidence(iter->confidence);
    car_result.set_value(iter->inference_result);

    // construct callback Messages,send to presenter server
    unique_ptr < google::protobuf::Message > response_objcar;
    PresenterErrorCode car_err = agent_channel_->SendMessage(car_result,
                                                             response_objcar);
    if (car_err != PresenterErrorCode::kNone) {
      HIAI_ENGINE_LOG(HIAI_ENGINE_RUN_ARGS_NOT_RIGHT,
                      "send car result failed, error code=%d,object_id = %s",
                      car_err, iter->object_id.c_str());
      return kSendDataFailed;
    }

    // get responded Message and judge result
    CommonResponse* car_response = dynamic_cast<CommonResponse*>(response_objcar
        .get());
    if (car_response == nullptr) {
      HIAI_ENGINE_LOG(HIAI_ENGINE_RUN_ARGS_NOT_RIGHT,
                      "car_response is nullptr");
      return kSendDataFailed;
    }

    ErrorCode response_code = car_response->ret();
    if (response_code != kErrorNone) {
      HIAI_ENGINE_LOG(
          HIAI_ENGINE_RUN_ARGS_NOT_RIGHT,
          "[SendCarInfo]server response failed, error code=%d,object_id = %s",
          response_code, iter->object_id.c_str());
      return kSendDataFailed;
    }
  }

  return kOperationOk;
}

OperationCode VideoAnalysisPost::SendPedestrianInfo(
    const shared_ptr<BatchPedestrianInfoT> &pedestrian_info_para) {
  if (pedestrian_info_para == nullptr) {
    return kInvalidParam;
  }

  // exit app when data has been transferred
  if (pedestrian_info_para->video_image_info.is_finished) {
    return kExitApp;
  }

  for (vector<PedestrianInfoT>::iterator iter = pedestrian_info_para
      ->pedestrian_info.begin();
      iter != pedestrian_info_para->pedestrian_info.end(); ++iter) {
    // Construct Message HumanInferenceResult,which has FrameIndex,object_id,
    // and human_property
    HumanInferenceResult person_result;
    FrameIndex* frame_person = new FrameIndex();

    // set up relevant infomations of origin image
    frame_person->set_app_id(app_config_->app_name);
    frame_person->set_channel_id(
        pedestrian_info_para->video_image_info.channel_id);
    frame_person->set_channel_name(
        pedestrian_info_para->video_image_info.channel_name);
    frame_person->set_frame_id(
        to_string(pedestrian_info_para->video_image_info.frame_id));
    person_result.set_allocated_frame_index(frame_person);

    // set up relevant infomations of small image
    person_result.set_object_id(iter->object_id);
    map<string, float>::iterator iter_map;
    for (iter_map = iter->pedestrian_attribute_map.begin();
        iter_map != iter->pedestrian_attribute_map.end(); ++iter_map) {
      // set up human_property
      MapType* property_map = person_result.add_human_property();
      property_map->set_key(iter_map->first);
      property_map->set_value(iter_map->second);
    }

    // construct callback Messages,send to presenter server
    unique_ptr < google::protobuf::Message > response_objper;
    PresenterErrorCode person_err = agent_channel_->SendMessage(
        person_result, response_objper);
    if (person_err != PresenterErrorCode::kNone) {
      HIAI_ENGINE_LOG(HIAI_ENGINE_RUN_ARGS_NOT_RIGHT,
                      "send person result failed, error code=%d,object_id = %s",
                      person_err, iter->object_id.c_str());
      return kSendDataFailed;
    }

    // get responded Message and judge result
    CommonResponse* person_response =
        dynamic_cast<CommonResponse*>(response_objper.get());
    if (person_response == nullptr) {
      HIAI_ENGINE_LOG(HIAI_ENGINE_RUN_ARGS_NOT_RIGHT,
                      "[SendPersonInfo]person_response is nullptr");
      return kSendDataFailed;
    }

    ErrorCode response_code = person_response->ret();
    if (response_code != kErrorNone) {
      HIAI_ENGINE_LOG(HIAI_ENGINE_RUN_ARGS_NOT_RIGHT, "[SendPersonInfo]server "
                      "response failed, error code=%d,object_id = %s",
                      response_code, iter->object_id.c_str());
      return kSendDataFailed;
    }
  }

  return kOperationOk;
}

HIAI_IMPL_ENGINE_PROCESS("video_analysis_post", VideoAnalysisPost, INPUT_SIZE) {
  //arg0:image detection; arg1:car type; arg2:car color; arg3:person info
  input_que_.PushData(0, arg0);
  input_que_.PushData(1, arg1);
  input_que_.PushData(2, arg2);
  input_que_.PushData(3, arg3);

  //input_arg0:image detection; input_arg1:car type;
  //input_arg2:car color; input_arg3:PedestrianInfo
  shared_ptr<void> input_arg0;
  shared_ptr<void> input_arg1;
  shared_ptr<void> input_arg2;
  shared_ptr<void> input_arg3;

  // if the image detection channel has data,then send to presenter server
  if (input_que_.FrontData(0, input_arg0)) {
    input_que_.PopData(0, input_arg0);
    image_ret_ = SendDetectionImage(
        static_pointer_cast<VideoDetectionImageParaT>(input_arg0));
    if ((image_ret_ != kOperationOk) && (image_ret_ != kExitApp)) {
      HIAI_ENGINE_LOG(HIAI_ENGINE_RUN_ARGS_NOT_RIGHT,"[VideoAnalysePost]"
          "SendDetectionImage failed,error code: %d", image_ret_);
    }
  }

  // if the car type channel has data,then send to presenter server
  if (input_que_.FrontData(1, input_arg1)) {
    input_que_.PopData(1, input_arg1);
    car_type_ret_ = SendCarInfo(
        static_pointer_cast<BatchCarInfoT>(input_arg1));
    if ((car_type_ret_ != kOperationOk) && (car_type_ret_ != kExitApp)) {
      HIAI_ENGINE_LOG(HIAI_ENGINE_RUN_ARGS_NOT_RIGHT,"[VideoAnalysePost]"
          "SendCarType failed,error code: %d",car_type_ret_);
    }
  }

  // if the car color channel has data,then send to presenter server
  if (input_que_.FrontData(2, input_arg2)) {
    input_que_.PopData(2, input_arg2);
    car_color_ret_ = SendCarInfo(
        static_pointer_cast<BatchCarInfoT>(input_arg2));
    if ((car_color_ret_ != kOperationOk) && (car_color_ret_ != kExitApp)) {
      HIAI_ENGINE_LOG(HIAI_ENGINE_RUN_ARGS_NOT_RIGHT,"[VideoAnalysePost]"
          "SendCarColor failed,error code: %d",car_color_ret_);
    }
  }

  // if the person info channel has data,then send to presenter server
  if (input_que_.FrontData(3, input_arg3)) {
    input_que_.PopData(3, input_arg3);
    pedestrian_ret_ = SendPedestrianInfo(
        static_pointer_cast<BatchPedestrianInfoT>(input_arg3));
    if ((pedestrian_ret_ != kOperationOk) && (pedestrian_ret_ != kExitApp)) {
      HIAI_ENGINE_LOG(HIAI_ENGINE_RUN_ARGS_NOT_RIGHT,"[VideoAnalysePost]"
          "SendPedestrianInfo failed,error code: %d",pedestrian_ret_);
    }
  }

  // if all channel transmissions are completed,then exit app
  if ((image_ret_ == kExitApp) && (car_type_ret_ == kExitApp) &&
      (car_color_ret_ == kExitApp) && (pedestrian_ret_ == kExitApp)) {
    HIAI_ENGINE_LOG(HIAI_ENGINE_RUN_ARGS_NOT_RIGHT,"[VideoAnalysePost]"
        "app will exit...");
    shared_ptr<string> result_data(new string);
    SendData(0, "string", static_pointer_cast<void>(result_data));
  }

  return HIAI_OK;
}
