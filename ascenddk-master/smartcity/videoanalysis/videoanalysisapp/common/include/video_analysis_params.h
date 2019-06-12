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

#ifndef COMMON_INCLUDE_VIDEO_ANALYSIS_PARAMS_H
#define COMMON_INCLUDE_VIDEO_ANALYSIS_PARAMS_H

#include "hiaiengine/data_type.h"
#include "hiaiengine/data_type_reg.h"

using hiai::ImageData;
using hiai::IMAGEFORMAT;

struct VideoImageInfoT {
  std::string channel_id;
  uint32_t frame_id;
  std::string channel_name;
  bool is_finished;

  VideoImageInfoT& operator=(VideoImageInfoT& value) {
    channel_id = value.channel_id;
    frame_id = value.frame_id;
    channel_name = value.channel_name;
    is_finished = value.is_finished;
    return *this;
  }
};

template <class Archive>
void serialize(Archive& ar, VideoImageInfoT& data) {
  ar(data.channel_id, data.frame_id, data.channel_name, data.is_finished);
}

struct VideoImageParaT {
  VideoImageInfoT video_image_info;
  hiai::ImageData<u_int8_t> img;
};

template <class Archive>
void serialize(Archive& ar, VideoImageParaT& data) {
  ar(data.video_image_info, data.img);
}

struct ObjectInfoT {
  std::string object_id;
  float score;
};

template <class Archive>
void serialize(Archive& ar, ObjectInfoT& data) {
  ar(data.object_id, data.score);
}

struct ObjectImageParaT {
  ObjectInfoT object_info;
  hiai::ImageData<u_int8_t> img;
};

template <class Archive>
void serialize(Archive& ar, ObjectImageParaT& data) {
  ar(data.object_info, data.img);
}

struct VideoDetectionImageParaT {
  VideoImageParaT image;
  std::vector<ObjectImageParaT> obj_imgs;
};

template <class Archive>
void serialize(Archive& ar, VideoDetectionImageParaT& data) {
  ar(data.image, data.obj_imgs);
}

struct BatchCroppedImageParaT {
  VideoImageInfoT video_image_info;
  std::vector<ObjectImageParaT> obj_imgs;
};

template <class Archive>
void serialize(Archive& ar, BatchCroppedImageParaT& data) {
  ar(data.video_image_info, data.obj_imgs);
}

enum CarInferenceType {
  kCarColor = 0,
  kCarType = 1,
};

struct CarInfoT {
  std::string object_id;
  uint32_t label;
  CarInferenceType attribute_name;  // attribute name:cartype or carcolor
  std::string inference_result;
  float confidence;
};

template <class Archive>
void serialize(Archive& ar, CarInfoT& data) {
  ar(data.object_id, data.label, data.attribute_name, data.inference_result,
     data.confidence);
}

struct BatchCarInfoT {
  VideoImageInfoT video_image_info;
  std::vector<CarInfoT> car_infos;
};

template <class Archive>
void serialize(Archive& ar, BatchCarInfoT& data) {
  ar(data.video_image_info, data.car_infos);
}

struct OutputT {
  int32_t size;
  std::string name;
  std::shared_ptr<u_int8_t> data;
};

template <class Archive>
void serialize(Archive& ar, OutputT& data) {
  ar(data.size);
  ar(data.name);
  if (data.size > 0 && data.data.get() == nullptr) {
    data.data.reset(new u_int8_t[data.size]);
  }

  ar(cereal::binary_data(data.data.get(), data.size * sizeof(u_int8_t)));
}

struct DetectionEngineTransT {
  bool status;
  std::string msg;
  std::vector<OutputT> output_datas;
  VideoImageParaT video_image;
};

template <class Archive>
void serialize(Archive& ar, DetectionEngineTransT& data) {
  ar(data.status, data.msg, data.output_datas, data.video_image);
}

struct PedestrianInfoT {
  std::string object_id;
  std::string
      attribute_name;  // property name:cartype or carcolor or pedestrian
  std::map<string, float>
      pedestrian_attribute_map;  // string:label_name,float:confidence
};

template <class Archive>
void serialize(Archive& ar, PedestrianInfoT& data) {
  ar(data.object_id, data.attribute_name, data.pedestrian_attribute_map);
}

struct BatchPedestrianInfoT {
  VideoImageInfoT video_image_info;
  std::vector<PedestrianInfoT> pedestrian_info;
};

template <class Archive>
void serialize(Archive& ar, BatchPedestrianInfoT& data) {
  ar(data.video_image_info, data.pedestrian_info);
}
#endif /* COMMON_INCLUDE_VIDEO_ANALYSIS_PARAMS_H */
