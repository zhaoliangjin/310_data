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

#ifndef VIDEO_ANALYSIS_POST_ENGINE_H_
#define VIDEO_ANALYSIS_POST_ENGINE_H_
#include <iostream>
#include <string>
#include <dirent.h>
#include <memory>
#include <unistd.h>
#include <vector>
#include <stdint.h>
#include "hiaiengine/engine.h"
#include "hiaiengine/multitype_queue.h"
#include "video_analysis_params.h"
#include "ascenddk/presenter/agent/channel.h"
#include "video_analysis_message.pb.h"

#define INPUT_SIZE 4
#define OUTPUT_SIZE 1

enum OperationCode {
  // send data success
  kOperationOk = 0,
  // input param of function is valid
  kInvalidParam,
  // app need to exit when reach final data
  kExitApp,
  // send data failed
  kSendDataFailed,
};

struct RegisterAppParam {
  // ip of presenter server
  string host_ip;
  // port of presenter server
  uint16_t port;
  // name of registered app
  string app_name;
  // type of registered app
  string app_type;
};

// IP regular expression
const std::string kIpRegularExpression =
    "^(1\\d{2}|2[0-4]\\d|25[0-5]|[1-9]\\d|[1-9])\\."
        "(1\\d{2}|2[0-4]\\d|25[0-5]|[1-9]\\d|\\d)\\."
        "(1\\d{2}|2[0-4]\\d|25[0-5]|[1-9]\\d|\\d)\\."
        "(1\\d{2}|2[0-4]\\d|25[0-5]|[1-9]\\d|\\d)$";

// port number range
const int32_t kPortMinNumber = 1024;
const int32_t kPortMaxNumber = 49151;

// app name regular expression
const std::string kAppNameRegularExpression = "[a-zA-Z0-9_]{3,20}";

// presenter server ip
const std::string kPresenterServerIP = "presenter_server_ip";

// presenter server port
const std::string kPresenterServerPort = "presenter_server_port";

// app name
const std::string kAppName = "app_name";

// app type
const std::string kAppType = "video_analysis";

class VideoAnalysisPost : public hiai::Engine {
public:
  /**
   * @brief   constructor
   */
  VideoAnalysisPost()
      : input_que_(INPUT_SIZE),
        app_config_(nullptr),
        agent_channel_(nullptr),
        image_ret_(kOperationOk),
        car_type_ret_(kOperationOk),
        car_color_ret_(kOperationOk),
        pedestrian_ret_(kOperationOk) {
  }

  /**
   * @brief   destructor
   */
  ~VideoAnalysisPost();

  /**
   * @brief  init config of video post by aiConfig
   * @param [in] config:  initialized aiConfig
   * @param [in] model_desc:  modelDesc
   * @return  success --> HIAI_OK ; fail --> HIAI_ERROR
   */
  HIAI_StatusT Init(const hiai::AIConfig& config,
                    const std::vector<hiai::AIModelDescription> &model_desc);

  /**
   * @brief: validate IP address
   * @param [in]: IP address
   * @return: true: invalid
   *          false: valid
   */
  bool IsInvalidIp(const std::string &ip);

  /**
   * @brief: validate port
   * @param [in]: port
   * @return: true: invalid
   *          false: valid
   */
  bool IsInvalidPort(const std::string &port);

  /**
   * @brief: validate app name
   * @param [in]: app name
   * @return: true: invalid
   *          false: valid
   */
  bool IsInvalidAppName(const std::string &app_name);

  /**
   * @brief  send image infomation to presenter server by protobuf
   * @param [in]  image infomation from detected engine
   * @return  OperationCode
   */
  OperationCode SendDetectionImage(
      const std::shared_ptr<VideoDetectionImageParaT> &image_para);

  /**
   * @brief  send car inference to presenter server by protobuf
   * @param [in]  car infomation from car inferential engine
   * @return  OperationCode
   */
  OperationCode SendCarInfo(
      const std::shared_ptr<BatchCarInfoT> &car_info_para);

  /**
   * @brief  send person inference to presenter server by protobuf
   * @param [in]  person infomation from person inferential engine
   * @return  OperationCode
   */
  OperationCode SendPedestrianInfo(
      const std::shared_ptr<BatchPedestrianInfoT> &pedestrian_info_para);

  /**
   * @brief  reload Engine Process
   * @param [in]  define the number of input and output
   */
  HIAI_DEFINE_PROCESS(INPUT_SIZE, OUTPUT_SIZE)

private:
  // Private implementation a member variable,
  // which is used to cache the input queue
  hiai::MultiTypeQueue input_que_;

  // used to save configs of app from engine
  std::shared_ptr<RegisterAppParam> app_config_;

  // agent channel by factory create,used to send Message
  ascend::presenter::Channel* agent_channel_;

  // ret of SendDetectionImage function
  OperationCode image_ret_;

  // ret of SendCarInfo function when infomation about car type
  OperationCode car_type_ret_;

  // ret of SendCarInfo function when infomation about car color
  OperationCode car_color_ret_;

  // ret of SendPedestrianInfo function
  OperationCode pedestrian_ret_;
};

#endif
