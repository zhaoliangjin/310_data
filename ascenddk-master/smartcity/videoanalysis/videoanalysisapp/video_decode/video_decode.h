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

#ifndef VIDEO_DECODE_H_
#define VIDEO_DECODE_H_

#include <dirent.h>
#include <stdint.h>
#include <unistd.h>

#include <iostream>
#include <string>
#include <memory>
#include <vector>

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
}

#include "thread_safe_queue.h"
#include "hiaiengine/engine.h"
#include "hiaiengine/multitype_queue.h"
#include "dvpp/idvppapi.h"
#include "video_analysis_params.h"

// input size used for engine
#define INPUT_SIZE 1

// output size used for engine
#define OUTPUT_SIZE 1

// video type
enum VideoType {
  kH264,
  kH265,
  kInvalidTpye
};

/**
 * @brief check current is key frame, key frame: 1,6,11,16...
 * @param [in] frame_id: frame id
 * @return true: is key frame; false: is not key frame
 */
bool IsKeyFrame(uint32_t frame_id);

/**
 * @brief get frame id by channel id
 * @param [in] channel_id: channel id
 * @return frame id
 */
uint32_t GetFrameId(const std::string &channel_id);

/**
 * @brief send key frame data to next engine
 * @param [in] vpcInMsg: input message used for vpc
 * @param [in] hiai_data: used for transmit channel id , channel name and frame id
 * @param [in] frame: image frame data
 */
void SendKeyFrameData(const vpc_in_msg &vpcInMsg, void* hiai_data,
                      FRAME* frame);

/**
 * @brief call vpc to get yuv42sp image
 * @param [in] frame: image frame data
 * @param [in] hiai_data: used for transmit channel and frame info
 */
void CallVpcGetYuvImage(FRAME* frame, void* hiai_data);

/**
 * @brief add image data to queue by channel id
 * @param [in] video_image_para: the image data from video
 * @param [out] current_queue: the queue used for current channel
 */
void AddImage2QueueByChannel(
    const shared_ptr<VideoImageParaT> &video_image_para,
    ThreadSafeQueue<shared_ptr<VideoImageParaT>> &current_queue);

// yuv420sp image frame info
struct YuvImageFrameInfo {
  std::string channel_name;
  std::string channel_id;
};

class VideoDecode : public hiai::Engine {
 public:

  /**
   * @brief VideoDecode constructor
   */
  VideoDecode();

  /**
   * @brief VideoDecode destructor
   */
  ~VideoDecode();

  /**
   * @brief current engine initialize function
   * @param [in] config: hiai engine config
   * @param [in] model_desc: hiai AI model description
   * @return HIAI_OK: initialize success; HIAI_ERROR:initialize failed
   */
  HIAI_StatusT Init(const hiai::AIConfig &config,
                    const std::vector<hiai::AIModelDescription> &model_desc);

  /**
   * @brief HIAI_DEFINE_PROCESS : reload Engine Process
   * @param [in] INPUT_SIZE: the input size of engine
   * @param [in] OUTPUT_SIZE: the output size of engine
   */
HIAI_DEFINE_PROCESS(INPUT_SIZE, OUTPUT_SIZE)

 private:

  std::string channel1_; // channel1 content, mp4 file path or rtsp address

  std::string channel2_; // channel2 content, mp4 file path or rtsp address

  /**
   * @brief unpack video to image
   * @param [in] channel_id: the input channel id
   */
  void UnpackVideo2Image(const std::string &channel_id);

  /**
   * @brief verify the  video type of channel1 and channle2
   */
  bool VerifyVideoType();

  /**
   * @brief unpack the video of input channel, and verify video type
   * @param [in] channel_value: the input channel value
   */
  bool VerifyVideoWithUnpack(const std::string &channel_value);

  /**
   * @brief handle video with multithread
   */
  void MultithreadHandleVideo();

  /**
   * @brief send image data to next engine
   * @param [in] channel_id: the input channel id
   */
  void SendImageDate(const std::string &channel_id);

  /**
   * @brief get video index form video format context
   * @param [in] av_format_context: video format context
   * @return video index
   */
  int GetVideoIndex(AVFormatContext* av_format_context);

  /**
   * @brief verify channel value is valid or not
   * @return true: verify passed; false: verify failed
   */
  bool VerifyChannelValues();

  /**
   * @brief verify video type
   * @param [in] videoindex: video index
   * @param [in] av_format_context: the video format context
   * @param [in] video_type: video type
   * @return true: the vide type is valid; false: the video type is invalid
   */
  bool CheckVideoType(int videoindex, AVFormatContext* av_format_context,
                      VideoType &video_type);

  /**
   * @brief check the input string is empty or not
   * @param [in] input_str: the input string
   * @param [in] channel_id: channel id
   * @return true: input string is empty; false: input string is not empty
   */
  bool IsEmpty(const std::string &input_str, const std::string &channel_id);

  /**
   * @brief verify the video source name is valid or not
   * @param [in] input_str: the input string
   * @return true: input string is valid; false: input string is invalid
   */
  bool VerifyVideoSourceName(const std::string &input_str);

  /**
   * @brief verify the input rtsp is valid or not
   * @param [in] input_str: the input string
   * @return true: input rtsp is valid; false: input rtsp is invalid
   */
  bool IsValidRtsp(const std::string &input_str);

  /**
   * @brief verify the input rtsp is valid or not
   * @param [in] input_str: the input string
   * @return true: input rtsp is valid; false: input rtsp is invalid
   */
  bool IsValidMp4File(const std::string &input_str);

  /**
   * @brief initialize video stream filter
   * @param [in] video_type: video type
   * @param [in] video_filter: video filter
   */
  void InitVideoStreamFilter(VideoType video_type,
                             const AVBitStreamFilter* &video_filter);

  /**
   * @brief sned finished data to next engine
   */
  void SendFinishedData();

  /**
   * @brief get channel id(integer value)
   * @param [in] channel_id: the input channel id
   * @return channel id(integer value)
   */
  int GetIntChannelId(const std::string channel_id);

  /**
   * @brief get channel value
   * @param [in] channel_id: the input channel id
   * @return channel value
   */
  const std::string &GetChannelValue(const std::string &channel_id);

  /**
   * @brief set dictionary for rtsp
   * @param [in] channel_value: the input channel value
   * @param [in] avdic: video dictionary
   */
  void SetDictForRtsp(const std::string &channel_value, AVDictionary* &avdic);

  /**
   * @brief open video format from input channel
   * @param [in] channel_value: the input channel value
   * @param [out] av_format_context: the video format context
   * @return true: success to open video; false: fail to open video
   */
  bool OpenVideoFromInputChannel(const std::string &channel_value,
                                 AVFormatContext* &av_format_context);

  /**
   * @brief open video format from input channel
   * @param [in] videoindex: the video index
   * @param [out] video_type: the input channel value
   * @param [in] av_format_context: the video format context
   * @param [out] bsf_ctx: video bsf context
   * @return true: success to initialize; false: fail to initialize
   */
  bool InitVideoParams(int videoindex, VideoType &video_type,
                       AVFormatContext* av_format_context,
                       AVBSFContext* &bsf_ctx);

  /**
   * @brief send image data by channel
   * @param [out] current_queue:current channel queue
   */
  void SendImageDataByChannel(
      ThreadSafeQueue<shared_ptr<VideoImageParaT>> &current_queue);
};

#endif /* VIDEO_DECODE_H_ */
