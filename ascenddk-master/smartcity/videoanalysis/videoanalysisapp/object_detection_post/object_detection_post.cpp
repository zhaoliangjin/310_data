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
#include "object_detection_post.h"
#include <unistd.h>
#include <memory>
#include <sstream>
#include "ascenddk/ascend_ezdvpp/dvpp_data_type.h"
#include "ascenddk/ascend_ezdvpp/dvpp_process.h"
using namespace std;

namespace {
// input engine port
const uint32_t kInputPort = 0;

// detection object labels
const uint32_t kLabelPerson = 1;
const uint32_t kLabelBus = 6;
const uint32_t kLabelCar = 3;

const uint32_t kSizePerResultset = 7;

// valid confidence value
const float kMinConfidence = 0.0f;
const float kMaxConfidence = 1.0f;

// dvpp minimal crop size
const uint32_t kMinCropPixel = 16;

// dvpp minimal yuv2jpg size
const uint32_t kMinJpegPixel = 32;

// valid bbox coordinate
const float kLowerCoord = 0.0f;
const float kUpperCoord = 1.0f;

// output engine ports
const uint32_t kPortPost = 0;
const uint32_t kPortCarType = 1;
const uint32_t kPortCarColor = 2;
const uint32_t kPortPedestrian = 3;

const int kDvppProcSuccess = 0;
const int kSleepMicroSecs = 20000;
const int kInferenceVectorSize = 2;
const int kInferenceOutputNum = 1;
const int kInferenceOutputBBox = 0;

const string kPrefixCar = "car_";
const string kPrefixBus = "bus_";
const string kPrefixPerson = "person_";

// function of dvpp returns success
const int kDvppOperationOk = 0;

// infernece output data index
enum BBoxDataIndex {
  kAttribute = 1,
  kScore,
  kTopLeftX,
  kTopLeftY,
  kLowerRightX,
  kLowerRightY,
};
}  // namespace

using ascend::utils::DvppCropOrResizePara;
using ascend::utils::DvppOutput;
using ascend::utils::DvppProcess;
using hiai::ImageData;
using namespace std;

// register data type
HIAI_REGISTER_DATA_TYPE("OutputT", OutputT);
HIAI_REGISTER_DATA_TYPE("DetectionEngineTransT", DetectionEngineTransT);
HIAI_REGISTER_DATA_TYPE("VideoImageInfoT", VideoImageInfoT);
HIAI_REGISTER_DATA_TYPE("VideoImageParaT", VideoImageParaT);
HIAI_REGISTER_DATA_TYPE("ObjectImageParaT", ObjectImageParaT);
HIAI_REGISTER_DATA_TYPE("ObjectInfoT", ObjectInfoT);
HIAI_REGISTER_DATA_TYPE("VideoDetectionImageParaT", VideoDetectionImageParaT);
HIAI_REGISTER_DATA_TYPE("BatchCroppedImageParaT", BatchCroppedImageParaT);

HIAI_StatusT ObjectDetectionPostProcess::Init(
    const hiai::AIConfig& config,
    const vector<hiai::AIModelDescription>& model_desc) {
  HIAI_ENGINE_LOG(HIAI_DEBUG_INFO, "[ODPostProcess] start to initialize!");

  for (int index = 0; index < config.items_size(); ++index) {
    const ::hiai::AIConfigItem& item = config.items(index);
    const string& name = item.name();
    const string& value = item.value();
    if (name == "Confidence") {
      if (!InitConfidence(value)) {
        HIAI_ENGINE_LOG(HIAI_ENGINE_RUN_ARGS_NOT_RIGHT,
                        "[ODPostProcess] confidence value %s is invalid!",
                        value.c_str());
        return HIAI_ERROR;
      }
    }
  }
  return HIAI_OK;
}

HIAI_StatusT ObjectDetectionPostProcess::CropObjectFromImage(
    const ImageData<u_int8_t>& src_img, ImageData<u_int8_t>& target_img,
    const BoundingBox& bbox) {
  DvppCropOrResizePara dvpp_crop_param;
  dvpp_crop_param.src_resolution.height = src_img.height;
  dvpp_crop_param.src_resolution.width = src_img.width;

  // the value of horz_max and vert_max must be odd and
  // horz_min and vert_min must be even.
  dvpp_crop_param.horz_min = bbox.lt_x % 2 == 0 ? bbox.lt_x : bbox.lt_x + 1;
  dvpp_crop_param.horz_max = bbox.rb_x % 2 == 0 ? bbox.rb_x - 1 : bbox.rb_x;
  dvpp_crop_param.vert_min = bbox.lt_y % 2 == 0 ? bbox.lt_y : bbox.lt_y + 1;
  dvpp_crop_param.vert_max = bbox.rb_y % 2 == 0 ? bbox.rb_y - 1 : bbox.rb_y;

  // calculate cropped image width and height.
  int dest_width = dvpp_crop_param.horz_max - dvpp_crop_param.horz_min + 1;
  int dest_height = dvpp_crop_param.vert_max - dvpp_crop_param.vert_min + 1;

  if (dest_width < kMinJpegPixel || dest_height < kMinJpegPixel) {
    float short_side = dest_width < dest_height ? dest_width : dest_height;
    dest_width = dest_width * (kMinJpegPixel / short_side);
    dest_height = dest_height * (kMinJpegPixel / short_side);
  }

  dvpp_crop_param.dest_resolution.width =
      dest_width % 2 == 0 ? dest_width : dest_width + 1;
  dvpp_crop_param.dest_resolution.height =
      dest_height % 2 == 0 ? dest_height : dest_height + 1;
  dvpp_crop_param.is_input_align = true;

  DvppProcess dvpp_process(dvpp_crop_param);

  DvppOutput dvpp_out;
  int ret = dvpp_process.DvppOperationProc(
      reinterpret_cast<char*>(src_img.data.get()), src_img.size, &dvpp_out);
  if (ret != kDvppProcSuccess) {
    HIAI_ENGINE_LOG(HIAI_ENGINE_RUN_ARGS_NOT_RIGHT,
                    "[ODPostProcess] crop image failed with code %d !", ret);
    return HIAI_ERROR;
  }
  target_img.channel = src_img.channel;
  target_img.format = src_img.format;
  target_img.data.reset(dvpp_out.buffer, default_delete<uint8_t[]>());
  target_img.width = dvpp_crop_param.dest_resolution.width;
  target_img.height = dvpp_crop_param.dest_resolution.height;
  target_img.size = dvpp_out.size;

  return HIAI_OK;
}

void ObjectDetectionPostProcess::FilterBoundingBox(
    float* bbox_buffer, int32_t bbox_buffer_size,
    shared_ptr<VideoDetectionImageParaT>& detection_image,
    vector<ObjectImageParaT>& car_type_imgs,
    vector<ObjectImageParaT>& car_color_imgs,
    vector<ObjectImageParaT>& person_imgs) {
  float* ptr = bbox_buffer;
  int32_t num_car = 0;
  int32_t num_bus = 0;
  int32_t num_person = 0;

  uint32_t base_width = detection_image->image.img.width;
  uint32_t base_height = detection_image->image.img.height;

  for (int32_t k = 0; k < bbox_buffer_size; k += kSizePerResultset) {
    ptr = bbox_buffer + k;
    int32_t attr = static_cast<int32_t>(ptr[BBoxDataIndex::kAttribute]);
    float score = ptr[BBoxDataIndex::kScore];

    if (score < confidence_ ||
        (attr != kLabelCar && attr != kLabelBus && attr != kLabelPerson)) {
      continue;
    }

    // bbox coordinate should between 0.0f and 1.0f
    uint32_t lt_x =
                 CorrectCoordinate(ptr[BBoxDataIndex::kTopLeftX]) * base_width,
             lt_y =
                 CorrectCoordinate(ptr[BBoxDataIndex::kTopLeftY]) * base_height,
             rb_x = CorrectCoordinate(ptr[BBoxDataIndex::kLowerRightX]) *
                    base_width,
             rb_y = CorrectCoordinate(ptr[BBoxDataIndex::kLowerRightY]) *
                    base_height;

    if (rb_x - lt_x < kMinCropPixel || rb_y - lt_x < kMinCropPixel) {
      continue;
    }
    // crop image
    ObjectImageParaT object_image;
    BoundingBox bbox = {lt_x, lt_y, rb_x, rb_y};
    HIAI_StatusT crop_ret =
        CropObjectFromImage(detection_image->image.img, object_image.img, bbox);
    if (crop_ret != HIAI_OK) {
      continue;
    }

    object_image.object_info.score = score;
    if (attr == kLabelCar) {
      ++num_car;
      stringstream ss;
      ss << kPrefixCar << num_car;
      object_image.object_info.object_id = ss.str();
      car_type_imgs.push_back(object_image);
      car_color_imgs.push_back(object_image);

    } else if (attr == kLabelBus) {
      ++num_bus;
      stringstream ss;
      ss << kPrefixBus << num_bus;
      object_image.object_info.object_id = ss.str();
      car_color_imgs.push_back(object_image);

    } else if (attr == kLabelPerson) {
      ++num_person;
      stringstream ss;
      ss << kPrefixPerson << num_person;
      object_image.object_info.object_id = ss.str();
      person_imgs.push_back(object_image);
    }
    detection_image->obj_imgs.push_back(object_image);
  }
}
HIAI_StatusT ObjectDetectionPostProcess::HandleResults(
    const shared_ptr<DetectionEngineTransT>& inference_result) {
  shared_ptr<VideoDetectionImageParaT> detection_image =
      make_shared<VideoDetectionImageParaT>();

  detection_image->image = inference_result->video_image;

  // send finished datas to all output port.
  if (inference_result->video_image.video_image_info.is_finished) {
    HIAI_ENGINE_LOG(HIAI_DEBUG_INFO, "[ODPostProcess] input video finished");
    SendResults(kPortPost, "VideoDetectionImageParaT",
                static_pointer_cast<void>(detection_image));

    for (uint32_t port = kPortCarType; port < OUTPUT_SIZE; ++port) {
      shared_ptr<BatchCroppedImageParaT> batch_out =
          make_shared<BatchCroppedImageParaT>();
      batch_out->video_image_info =
          inference_result->video_image.video_image_info;
      SendResults(port, "<BatchCroppedImageParaT",
                  static_pointer_cast<void>(batch_out));
    }
    return HIAI_OK;
  }

  if (!inference_result->status ||
      inference_result->output_datas.size() < kInferenceVectorSize) {
    SendDetectImage(detection_image);
    return HIAI_ERROR;
  }

  // output data
  OutputT out_num = inference_result->output_datas[kInferenceOutputNum];
  OutputT out_bbox = inference_result->output_datas[kInferenceOutputBBox];

  if (out_bbox.data == nullptr || out_bbox.data.get() == nullptr ||
      out_num.data == nullptr || out_num.data.get() == nullptr) {
    // error
    HIAI_ENGINE_LOG(HIAI_ENGINE_RUN_ARGS_NOT_RIGHT,
                    "[ODPostProcess] image inference out");
    SendDetectImage(detection_image);
    return HIAI_ERROR;
  }

  vector<ObjectImageParaT> car_type_imgs;
  vector<ObjectImageParaT> car_color_imgs;
  vector<ObjectImageParaT> person_imgs;

  float* bbox_buffer = reinterpret_cast<float*>(out_bbox.data.get());
  float bbox_number = *reinterpret_cast<float*>(out_num.data.get());
  int32_t bbox_buffer_size = bbox_number * kSizePerResultset;
  HIAI_ENGINE_LOG(HIAI_DEBUG_INFO, "[ODPostProcess] number of bbox: %d",
                  bbox_number);

  FilterBoundingBox(bbox_buffer, bbox_buffer_size, detection_image,
                    car_type_imgs, car_color_imgs, person_imgs);

  // send_data
  HIAI_StatusT send_ret = SendDetectImage(detection_image);
  if (send_ret != HIAI_OK) {
    HIAI_ENGINE_LOG(HIAI_ENGINE_RUN_ARGS_NOT_RIGHT,
                    "[ODPostProcess] send image error channel: %d, frame: %d!",
                    detection_image->image.video_image_info.channel_id,
                    detection_image->image.video_image_info.frame_id);
  }

  SendCroppedImages(kPortCarColor, car_color_imgs,
                    inference_result->video_image.video_image_info);
  SendCroppedImages(kPortCarType, car_type_imgs,
                    inference_result->video_image.video_image_info);
  SendCroppedImages(kPortPedestrian, person_imgs,
                    inference_result->video_image.video_image_info);
  return HIAI_OK;
}

void ObjectDetectionPostProcess::SendCroppedImages(
    uint32_t port_id, const vector<ObjectImageParaT>& cropped_images,
    VideoImageInfoT& video_image_info) {
  if (cropped_images.empty()) {
    return;
  }
  shared_ptr<BatchCroppedImageParaT> object_image =
      make_shared<BatchCroppedImageParaT>();
  object_image->video_image_info = video_image_info;
  object_image->obj_imgs = cropped_images;
  HIAI_StatusT ret = SendResults(port_id, "BatchCroppedImageParaT",
                                 static_pointer_cast<void>(object_image));
  if (ret != HIAI_OK) {
    HIAI_ENGINE_LOG(
        HIAI_ENGINE_RUN_ARGS_NOT_RIGHT,
        "[ODPostProcess] send cropped image error channel: %d, frame: %d!",
        video_image_info.channel_id, video_image_info.frame_id);
  }
}

// if input string is a valid number.
bool ObjectDetectionPostProcess::InitConfidence(const string& input) {
  istringstream iss(input);
  float tmp;
  iss >> noskipws >> tmp;
  if (!(iss.eof() && !iss.fail()) ||
      !(tmp >= kMinConfidence && tmp <= kMaxConfidence)) {
    return false;
  }
  confidence_ = tmp;
  return true;
}

HIAI_StatusT ObjectDetectionPostProcess::SendResults(
    uint32_t port_id, string data_type, const shared_ptr<void>& data_ptr) {
  HIAI_StatusT ret;
  do {
    ret = SendData(port_id, data_type, data_ptr);
    if (ret == HIAI_QUEUE_FULL) {
      HIAI_ENGINE_LOG(HIAI_DEBUG_INFO, "[ODPostProcess] port %d queue full.",
                      port_id);
      usleep(kSleepMicroSecs);
    }
  } while (ret == HIAI_QUEUE_FULL);
  if (ret != HIAI_OK) {
    return HIAI_ERROR;
  }
  return HIAI_OK;
}

HIAI_StatusT ObjectDetectionPostProcess::SendDetectImage(
    const shared_ptr<VideoDetectionImageParaT> &image_para) {
  ascend::utils::DvppToJpgPara dvpp_to_jpg_para;
  dvpp_to_jpg_para.format = JPGENC_FORMAT_NV12;

  // use dvpp convert yuv to jpg image, level should set fixed value 100
  dvpp_to_jpg_para.level = 100;
  dvpp_to_jpg_para.resolution.height = image_para->image.img.height;
  dvpp_to_jpg_para.resolution.width = image_para->image.img.width;

  // true indicate the image is aligned
  dvpp_to_jpg_para.is_align_image = true;
  ascend::utils::DvppProcess dvpp_jpg_process(dvpp_to_jpg_para);
  ascend::utils::DvppOutput dvpp_output = { 0 };

  // use dvpp convert yuv image to jpg image
  int ret_dvpp;
  ret_dvpp = dvpp_jpg_process.DvppOperationProc(
      (char*) (image_para->image.img.data.get()), image_para->image.img.size,
      &dvpp_output);
  if (ret_dvpp == kDvppOperationOk) {
    image_para->image.img.data.reset(dvpp_output.buffer,
                                     default_delete<uint8_t[]>());
    image_para->image.img.size = dvpp_output.size;
  } else {
    HIAI_ENGINE_LOG(HIAI_ENGINE_RUN_ARGS_NOT_RIGHT,
                    "fail to convert yuv to jpg,ret_dvpp = %d", ret_dvpp);
    return HIAI_ERROR;
  }

  // get small images after reasoning
  for (vector<ObjectImageParaT>::iterator iter = image_para->obj_imgs.begin();
      iter != image_para->obj_imgs.end(); ++iter) {
    // use dvpp convert yuv image to jpg image
    ascend::utils::DvppToJpgPara dvpp_to_jpg_obj_para;
    dvpp_to_jpg_obj_para.format = JPGENC_FORMAT_NV12;

    // level should set fixed value 100
    dvpp_to_jpg_obj_para.level = 100;

    // true indicate the image is aligned
    dvpp_to_jpg_obj_para.is_align_image = true;
    dvpp_to_jpg_obj_para.resolution.height = iter->img.height;
    dvpp_to_jpg_obj_para.resolution.width = iter->img.width;
    ascend::utils::DvppProcess dvpp_jpg_objprocess(dvpp_to_jpg_obj_para);
    ascend::utils::DvppOutput obj_dvpp_output = { 0 };
    ret_dvpp = dvpp_jpg_objprocess.DvppOperationProc(
        (char*) (iter->img.data.get()), iter->img.size, &obj_dvpp_output);
    if (ret_dvpp == kDvppOperationOk) {
      iter->img.data.reset(obj_dvpp_output.buffer, default_delete<uint8_t[]>());
      iter->img.size = obj_dvpp_output.size;
    } else {
      HIAI_ENGINE_LOG(HIAI_ENGINE_RUN_ARGS_NOT_RIGHT,
                      "fail to convert obj yuv to jpg,ret_dvpp = %d", ret_dvpp);
      return HIAI_ERROR;
    }
  }

  return SendResults(kPortPost, "VideoDetectionImageParaT",
                     static_pointer_cast<void>(image_para));
}

float ObjectDetectionPostProcess::CorrectCoordinate(float value) {
  float tmp = value < kLowerCoord ? kLowerCoord : value;
  return tmp > kUpperCoord ? kUpperCoord : tmp;
}

HIAI_IMPL_ENGINE_PROCESS("object_detection_post", ObjectDetectionPostProcess,
                         INPUT_SIZE) {

  if (arg0 == nullptr) {
    HIAI_ENGINE_LOG(HIAI_ENGINE_RUN_ARGS_NOT_RIGHT,
                    "[ODPostProcess] failed to PopAllData!");
    return HIAI_ERROR;
  }
  shared_ptr<DetectionEngineTransT> detection_trans =
      static_pointer_cast<DetectionEngineTransT>(arg0);

  return HandleResults(detection_trans);
}
