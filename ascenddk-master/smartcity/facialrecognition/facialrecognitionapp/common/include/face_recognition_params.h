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

#ifndef FACE_RECOGNITION_PARAMS_H_
#define FACE_RECOGNITION_PARAMS_H_

#include "hiaiengine/data_type.h"
#include "ascenddk/ascend_ezdvpp/dvpp_data_type.h"

#define CHECK_MEM_OPERATOR_RESULTS(ret) \
if (ret != EOK) { \
  HIAI_ENGINE_LOG(HIAI_ENGINE_RUN_ARGS_NOT_RIGHT, \
                  "memory operation failed, error=%d", ret); \
  return false; \
}

// NV12 image's transformation param
// The memory size of the NV12 image is 1.5 times that of width*height.
const int32_t kNv12SizeMolecule = 3;
const int32_t kNv12SizeDenominator = 2;

// model path parameter key in graph.config
const string kModelPathParamKey = "model_path";

// batch size parameter key in graph.config
const string kBatchSizeParamKey = "batch_size";

/**
 * @brief: face recognition APP error code definition
 */
enum class AppErrorCode {
  // Success, no error
  kNone = 0,

  // register engine failed
  kRegister,

  // detection engine failed
  kDetection,

  // feature mask engine failed
  kFeatureMask
};

/**
 * @brief: frame information
 */
struct FrameInfo {
  uint32_t frame_id = 0;  // frame id
  uint32_t channel_id = 0;  // channel id for current frame
  uint32_t timestamp = 0;  // timestamp for current frame
  uint32_t image_source = 0;  // 0:Camera 1:Register
  std::string face_id = "";  // registered face id
  // original image format and rank using for org_img addition
  // IMAGEFORMAT defined by HIAI engine does not satisfy the dvpp condition
  ascend::utils::DvppVpcImageType org_img_format =
      ascend::utils::kVpcYuv420SemiPlannar;
  ascend::utils::DvppVpcImageRankType org_img_rank = ascend::utils::kVpcNv21;
  bool img_aligned = false; // original image already aligned or not
};

/**
 * @brief: serialize for FrameInfo
 *         engine uses it to transfer data between host and device
 */
template<class Archive>
void serialize(Archive& ar, FrameInfo& data) {
  ar(data.frame_id, data.channel_id, data.timestamp, data.image_source,
     data.face_id, data.org_img_format, data.org_img_rank);
}

/**
 * @brief: Error information
 */
struct ErrorInfo {
  AppErrorCode err_code = AppErrorCode::kNone;
  std::string err_msg = "";
};

/**
 * @brief: serialize for ErrorInfo
 *         engine uses it to transfer data between host and device
 */
template<class Archive>
void serialize(Archive& ar, ErrorInfo& data) {
  ar(data.err_code, data.err_msg);
}

/**
 * @brief: face rectangle
 */
struct FaceRectangle {
  hiai::Point2D lt;  // left top
  hiai::Point2D rb;  // right bottom
};

/**
 * @brief: serialize for FacePoint
 *         engine uses it to transfer data between host and device
 */
template<class Archive>
void serialize(Archive& ar, FaceRectangle& data) {
  ar(data.lt, data.rb);
}

/**
 * @brief: face feature
 */
struct FaceFeature {
  bool flag;
  hiai::Point2D left_eye;  // left eye
  hiai::Point2D right_eye;  // right eye
  hiai::Point2D nose;  // nose
  hiai::Point2D left_mouth;  // left mouth
  hiai::Point2D right_mouth;  // right mouth
};

/**
 * @brief: serialize for FaceFeature
 *         engine uses it to transfer data between host and device
 */
template<class Archive>
void serialize(Archive& ar, FaceFeature& data) {
  ar(data.left_eye, data.right_eye, data.nose, data.left_mouth,
     data.right_mouth);
}

/**
 * @brief: face image
 */
struct FaceImage {
  hiai::ImageData<u_int8_t> image;  // cropped image from original image
  FaceRectangle rectangle;  // face rectangle
  FaceFeature feature_mask;  // face feature mask
  std::vector<float> feature_vector;  // face feature vector
};

/**
 * @brief: serialize for FaceImage
 *         engine uses it to transfer data between host and device
 */
template<class Archive>
void serialize(Archive& ar, FaceImage& data) {
  ar(data.image, data.rectangle, data.feature_mask, data.feature_vector);
}

/**
 * @brief: information for face recognition
 */
struct FaceRecognitionInfo {
  FrameInfo frame;  // frame information
  ErrorInfo err_info;  // error information
  hiai::ImageData<u_int8_t> org_img;  // original image
  std::vector<FaceImage> face_imgs;  // cropped image
};

/**
 * @brief: serialize for FaceRecognitionInfo
 *         engine uses it to transfer data between host and device
 */
template<class Archive>
void serialize(Archive& ar, FaceRecognitionInfo& data) {
  ar(data.frame, data.err_info, data.org_img, data.face_imgs);
}

#endif /* FACE_RECOGNITION_PARAMS_H_ */
