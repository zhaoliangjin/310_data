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

#include "pedestrian_attr_inference.h"

#include <unistd.h>
#include <time.h>

#include <vector>
#include <thread>
#include <fstream>
#include <algorithm>
#include <iostream>

#include <hiaiengine/log.h>
#include <hiaiengine/ai_types.h>
#include "hiaiengine/ai_model_parser.h"

namespace {
const int kDestImageWidth = 226; // the image width for model

const int kDestImageHeight = 226; // the image height for model

const int kPedestrianAttrNumber = 35; // pedestrian attributes number

const int kAgeAttrNumber = 4; // age attributes number

// pedestrian attributes confidence threshold value
const float kAttrConfidenceThreshold = 0.5;

const int kHairAttrOrder = 15; // short or long hair attributes order

const int kGenderAttrOrder = 16; // gender attributes order

const string kModelPathItemName = "model_path"; // model path string

const string kPasscodeItemName = "passcode"; // passcode string

// the name of batch_size in the config file
const string kBatchSizeItemName = "batch_size";

const string kAttrShortHair = "Short hair"; // the attribute: short hair

const string kAttrLongHair = "Long hair"; // the attribute: long hair

const string kAttrFemale = "Female"; // the attribute: female

const string kAttrMale = "Male"; // the attribute: male

// pedestrian each attribute, total 35
const string kPedestrianAttr[kPedestrianAttrNumber] = { "Age16-30", "Age31-45",
    "Age46-60", "AgeAbove61", "Backpack", "CarryingOther", "Casual lower",
    "Casual upper", "Formal lower", "Formal upper", "Hat", "Jacket", "Jeans",
    "Leather Shoes", "Logo", "Long hair", "Male", "Messenger Bag", "Muffler",
    "No accessory", "No carrying", "Plaid", "PlasticBags", "Sandals", "Shoes",
    "Shorts", "Short Sleeve", "Skirt", "Sneaker", "Stripes", "Sunglasses",
    "Trousers", "Tshirt", "UpperOther", "V-Neck" };

const int kWaitTime = 10000; // wait 10 ms when send data to Cnext engine
}

HIAI_REGISTER_DATA_TYPE("PedestrianInfoT", PedestrianInfoT);
HIAI_REGISTER_DATA_TYPE("BatchPedestrianInfoT", BatchPedestrianInfoT);
HIAI_REGISTER_DATA_TYPE("VideoImageInfoT", VideoImageInfoT);
HIAI_REGISTER_DATA_TYPE("BatchCroppedImageParaT", BatchCroppedImageParaT);

HIAI_StatusT PedestrianAttrInference::Init(
    const hiai::AIConfig &config,
    const std::vector<hiai::AIModelDescription> &model_desc) {
  HIAI_ENGINE_LOG("Start init!");

  if (ai_model_manager_ == nullptr) { // check ai model manager is nullptr
    ai_model_manager_ = std::make_shared<hiai::AIModelManager>();
  }

  std::vector<hiai::AIModelDescription> model_desc_vec;
  hiai::AIModelDescription model_description;

  // loop for each config items
  for (int index = 0; index < config.items_size(); ++index) {
    const ::hiai::AIConfigItem &item = config.items(index);

    if (item.name() == kModelPathItemName) { // get mode path
      const char* model_path = item.value().data();
      model_description.set_path(model_path);
    } else if (item.name() == kPasscodeItemName) { // get passcode
      const char* passcode = item.value().data();
      model_description.set_key(passcode);
    } else if (item.name() == kBatchSizeItemName) { // get batch size
      std::stringstream ss(item.value());
      ss >> batch_size_;
    }
  }

  model_desc_vec.push_back(model_description);

  // check ai model manager initialize result
  if (ai_model_manager_->Init(config, model_desc_vec) != hiai::SUCCESS) {
    return HIAI_ERROR;
  }

  HIAI_ENGINE_LOG("End init!");
  return HIAI_OK;
}

HIAI_StatusT PedestrianAttrInference::SendResultData(
    const std::shared_ptr<BatchPedestrianInfoT> &tran_data) {

  HIAI_StatusT hiai_ret = HIAI_OK;

  // send result data to next engine
  do {
    HIAI_ENGINE_LOG("All data has send, process success!");

    // send data to output port 0
    hiai_ret = SendData(0, "BatchPedestrianInfoT",
                        std::static_pointer_cast<void>(tran_data));
    if (hiai_ret == HIAI_QUEUE_FULL) { // check queue is full
      HIAI_ENGINE_LOG("The queue full, sleep 10ms");
      usleep(kWaitTime);
    }
  } while (hiai_ret == HIAI_QUEUE_FULL); // loop while queue is full

  if (hiai_ret != HIAI_OK) { // check send data is failed
    HIAI_ENGINE_LOG(HIAI_ENGINE_RUN_ARGS_NOT_RIGHT, "Fail to send data!");
    return HIAI_ERROR;
  }

  return HIAI_OK;
}

//resize YUV420SP image
void PedestrianAttrInference::BatchImageResize(
    std::shared_ptr<BatchCroppedImageParaT> &batch_image_input,
    std::shared_ptr<BatchCroppedImageParaT> &batch_image_output) {
  batch_image_output->video_image_info = batch_image_input->video_image_info;

  // resize each image
  for (std::vector<ObjectImageParaT>::iterator iter = batch_image_input
      ->obj_imgs.begin(); iter != batch_image_input->obj_imgs.end(); ++iter) {
    ascend::utils::DvppCropOrResizePara dvpp_resize_param;

    // DVPP limits horz_max should be Odd number
    dvpp_resize_param.horz_max =
        iter->img.width % 2 == 0 ? iter->img.width - 1 : iter->img.width;
    dvpp_resize_param.horz_min = 0;

    // DVPP limits vert_max should be Odd number
    dvpp_resize_param.vert_max =
        iter->img.height % 2 == 0 ? iter->img.height - 1 : iter->img.height;
    dvpp_resize_param.vert_min = 0;
    dvpp_resize_param.is_input_align = true;
    dvpp_resize_param.is_output_align = true;
    dvpp_resize_param.src_resolution.width = iter->img.width;
    dvpp_resize_param.src_resolution.height = iter->img.height;
    dvpp_resize_param.dest_resolution.width = kDestImageWidth;
    dvpp_resize_param.dest_resolution.height = kDestImageHeight;

    ascend::utils::DvppProcess dvpp_process(dvpp_resize_param);

    char* image_buffer = (char*) (iter->img.data.get());
    ascend::utils::DvppOutput dvpp_out;
    int ret = dvpp_process.DvppOperationProc(image_buffer, iter->img.size,
                                             &dvpp_out);
    if (ret != ascend::utils::kDvppOperationOk) { // check call dvpp result
      HIAI_ENGINE_LOG(HIAI_ENGINE_RUN_ARGS_NOT_RIGHT,
                      "Fail to resize image, result code:%d !", ret);
      continue;
    }

    std::shared_ptr<ObjectImageParaT> obj_image = std::make_shared<
        ObjectImageParaT>();
    if (obj_image == nullptr) { // check make_shared result
      obj_image = std::make_shared<ObjectImageParaT>();
    }

    obj_image->object_info.object_id = iter->object_info.object_id;
    obj_image->object_info.score = iter->object_info.score;
    obj_image->img.width = kDestImageWidth;
    obj_image->img.height = kDestImageHeight;
    obj_image->img.channel = iter->img.channel;
    obj_image->img.depth = iter->img.depth;
    obj_image->img.size = dvpp_out.size;
    obj_image->img.data.reset(dvpp_out.buffer,
                              std::default_delete<uint8_t[]>());

    batch_image_output->obj_imgs.push_back(*obj_image);
  }
}

void PedestrianAttrInference::ExtractResults(int batch_result_index,
                                             PedestrianInfoT &out_data,
                                             float* &result) {
  string age_str = ""; // initialize to empty string
  float age_confidence = 0; // initialize to 0

  // add age confidence to map
  for (int i = 0; i < kAgeAttrNumber; i++) { // age has 4 values
    float current_confidence = *(result
        + batch_result_index * kPedestrianAttrNumber + i);

    if (age_confidence < current_confidence) { // get the max confidence
      age_str = kPedestrianAttr[i];
      age_confidence = current_confidence;
    }
  }

  out_data.pedestrian_attribute_map.insert(
      pair<string, float>(age_str, age_confidence));

  // add hair confidence to map
  float hair_confidence = *(result + batch_result_index * kPedestrianAttrNumber
      + kHairAttrOrder);
  if (hair_confidence < kAttrConfidenceThreshold) { // is short hair
    // if gender is short hair, the confidence is the remainder of 1.0
    out_data.pedestrian_attribute_map.insert(
        pair<string, float>(kAttrShortHair, 1.0 - hair_confidence));
  } else { // is long hair
    out_data.pedestrian_attribute_map.insert(
        pair<string, float>(kAttrLongHair, hair_confidence));
  }

  // add gender confidence to map
  float gender_confidence = *(result
      + batch_result_index * kPedestrianAttrNumber + kGenderAttrOrder);
  if (gender_confidence < kAttrConfidenceThreshold) { // is female
    // if gender is female, the confidence is the remainder of 1.0
    out_data.pedestrian_attribute_map.insert(
        pair<string, float>(kAttrFemale, 1.0 - gender_confidence));
  } else { // is male
    out_data.pedestrian_attribute_map.insert(
        pair<string, float>(kAttrMale, gender_confidence));
  }

  // loop for each pedestrian attribute
  for (int i = kAgeAttrNumber; i < kPedestrianAttrNumber; i++) {
    float current_confidence = *(result
        + batch_result_index * kPedestrianAttrNumber + i);

    // check current confidence is valid
    if (current_confidence > kAttrConfidenceThreshold && i != kHairAttrOrder
        && i != kGenderAttrOrder) {
      out_data.pedestrian_attribute_map.insert(
          pair<string, float>(kPedestrianAttr[i], current_confidence));
    }
  }
}

bool PedestrianAttrInference::ConstructInferenceResult(
    const std::vector<std::shared_ptr<hiai::IAITensor>> &output_data_vec,
    int batch_index,
    const std::shared_ptr<BatchCroppedImageParaT> &image_handle,
    const std::shared_ptr<BatchPedestrianInfoT> &tran_data) {
  bool is_successed = true;
  int image_number = image_handle->obj_imgs.size();

  // loop for each data in output_data_vec
  for (int n = 0; n < output_data_vec.size(); ++n) {
    std::shared_ptr<hiai::AINeuralNetworkBuffer> result_tensor =
        std::static_pointer_cast<hiai::AINeuralNetworkBuffer>(
            output_data_vec[n]);
    //get confidence result
    int size = result_tensor->GetSize() / sizeof(float);
    float* result = (float*) result_tensor->GetBuffer();

    // analyze each batch result
    for (int batch_result_index = 0; batch_result_index < size;
        batch_result_index += kPedestrianAttrNumber) {
      if (batch_index + batch_result_index / kPedestrianAttrNumber
          < image_number) { // check current data is valid image data
        PedestrianInfoT out_data;
        out_data.object_id = image_handle->obj_imgs[batch_index
            + batch_result_index / kPedestrianAttrNumber].object_info.object_id;
        out_data.attribute_name = "pedestrian";

        // extract appropriate results
        ExtractResults(batch_result_index, out_data, result);

        tran_data->pedestrian_info.push_back(out_data);
      }
    }
  }

  return is_successed;
}

bool PedestrianAttrInference::ConstructBatchBuffer(
    int batch_index,
    const std::shared_ptr<BatchCroppedImageParaT> &image_handle,
    uint8_t* batch_buffer) {
  bool is_successed = true;
  int image_number = image_handle->obj_imgs.size();
  int image_size = image_handle->obj_imgs[0].img.size * sizeof(uint8_t);

  //the loop for each image
  for (int j = 0; j < batch_size_; j++) {
    if (batch_index + j < image_number) { // check current iamge is valid
      errno_t ret_memcpy = memcpy_s(
          batch_buffer + j * image_size, image_size,
          image_handle->obj_imgs[batch_index + j].img.data.get(), image_size);
      if (ret_memcpy != EOK) { // check memcpy_s result
        HIAI_ENGINE_LOG(HIAI_ENGINE_RUN_ARGS_NOT_RIGHT,
                        "Fail to copy image buffer!");
        is_successed = false;

        break;
      }
    } else { // invalid iamge data, append 0
      errno_t ret_memset = memset_s(batch_buffer + j * image_size, image_size,
                                    static_cast<char>(0), image_size);
      if (ret_memset != EOK) { // check memset_s result
        HIAI_ENGINE_LOG(HIAI_ENGINE_RUN_ARGS_NOT_RIGHT,
                        "Fail to padding batch image data!");
        is_successed = false;

        break;
      }
    }
  }

  return is_successed;
}

HIAI_StatusT PedestrianAttrInference::BatchInferenceProcess(
    const std::shared_ptr<BatchCroppedImageParaT> &image_handle,
    std::shared_ptr<BatchPedestrianInfoT> tran_data) {
  HIAI_ENGINE_LOG("Start process!");

  int image_number = image_handle->obj_imgs.size();
  int image_size = image_handle->obj_imgs[0].img.size * sizeof(uint8_t);
  int batch_buffer_size = image_size * batch_size_;

  // the loop for each batch ,maybe image_number greater than batch_size_
  for (int i = 0; i < image_number; i += batch_size_) {
    std::vector<std::shared_ptr<hiai::IAITensor>> input_data_vec;
    std::vector<std::shared_ptr<hiai::IAITensor>> output_data_vec;

    // prepare input buffer for each batch
    if (tran_data == nullptr) {
      tran_data = std::make_shared<BatchPedestrianInfoT>();
    }

    // apply buffer for each batch
    uint8_t* batch_buffer = new uint8_t[batch_buffer_size];
    // origin image information is transmitted to next Engine directly
    tran_data->video_image_info = image_handle->video_image_info;

    if (!ConstructBatchBuffer(i, image_handle, batch_buffer)) {
      HIAI_ENGINE_LOG(HIAI_ENGINE_RUN_ARGS_NOT_RIGHT,
                      "Fail to construct input buffer!");

      delete[] batch_buffer;
      return HIAI_ERROR;
    }

    std::shared_ptr<hiai::AINeuralNetworkBuffer> neural_buffer =
        std::shared_ptr<hiai::AINeuralNetworkBuffer>(
            new hiai::AINeuralNetworkBuffer());
    neural_buffer->SetBuffer((void*) (batch_buffer), batch_buffer_size);
    std::shared_ptr<hiai::IAITensor> input_data = std::static_pointer_cast<
        hiai::IAITensor>(neural_buffer);
    input_data_vec.push_back(input_data);

    // Call Process, Predict
    if (ai_model_manager_->CreateOutputTensor(input_data_vec, output_data_vec)
        != hiai::SUCCESS) {
      HIAI_ENGINE_RUN_ARGS_NOT_RIGHT, HIAI_ENGINE_LOG(
          "CreateOutputTensor failed");

      delete[] batch_buffer;
      return HIAI_ERROR;
    }

    hiai::AIContext ai_context;
    HIAI_ENGINE_LOG("Start ai_model_manager_->Process!");
    hiai::AIStatus ret_process = ai_model_manager_->Process(ai_context,
                                                            input_data_vec,
                                                            output_data_vec, 0);
    if (ret_process != hiai::SUCCESS) {
      HIAI_ENGINE_LOG(HIAI_ENGINE_RUN_ARGS_NOT_RIGHT,
                      "ai_model_manager Process failed");

      delete[] batch_buffer;
      return HIAI_ERROR;
    }

    delete[] batch_buffer;

    // set the tran_data with the result of this batch
    if (!ConstructInferenceResult(output_data_vec, i, image_handle,
                                  tran_data)) {
      HIAI_ENGINE_LOG(HIAI_ENGINE_RUN_ARGS_NOT_RIGHT,
                      "Fail to copy batch output buffer!");
      return HIAI_ERROR;
    }

    // send the result data
    HIAI_StatusT ret_send_data = SendResultData(tran_data);
    if (ret_send_data != HIAI_OK) { // check send data result
      HIAI_ENGINE_LOG(HIAI_ENGINE_RUN_ARGS_NOT_RIGHT,
                      "SendData failed! error code: %d", ret_send_data);
    }

    // release sources
    tran_data = nullptr;
  }

  HIAI_ENGINE_LOG("End process!");
  return HIAI_OK;
}

/**
 * @ingroup hiaiengine
 * @brief HIAI_DEFINE_PROCESS : Realize the port input/output processing
 * @[in]: Define an input port, an output port,
 *        And the Engine is registered, its called "HIAIMultiEngineExample"
 */
HIAI_IMPL_ENGINE_PROCESS("pedestrian_attr_inference",
    PedestrianAttrInference, INPUT_SIZE) {
  HIAI_ENGINE_LOG("Start process!");

  std::shared_ptr<BatchPedestrianInfoT> tran_data = std::make_shared<
      BatchPedestrianInfoT>();

  // PreProcess:Framework input data
  std::shared_ptr<BatchCroppedImageParaT> image_input = std::make_shared<
      BatchCroppedImageParaT>();
  std::shared_ptr<BatchCroppedImageParaT> image_handle = std::make_shared<
      BatchCroppedImageParaT>();

  input_que_.PushData(0, arg0); // push arg0 data to the input queue
  if (!input_que_.PopAllData(image_input)) { // get all input data
    HIAI_ENGINE_LOG(HIAI_ENGINE_RUN_ARGS_NOT_RIGHT, "Fail to PopAllData!");
    return HIAI_ERROR;
  }

  if (image_input == nullptr) { // check input image is nullptr
    HIAI_ENGINE_LOG(HIAI_ENGINE_RUN_ARGS_NOT_RIGHT,
                    "The input image is nullptr!");
    return HIAI_ERROR;
  }

  // check current data is contains is_finished
  if (image_input->video_image_info.is_finished == true) {
    tran_data->video_image_info = image_input->video_image_info;
    return SendResultData(tran_data);
  }

  // resize input image
  BatchImageResize(image_input, image_handle);
  if (image_handle->obj_imgs.empty() == true) { // check resize result
    HIAI_ENGINE_LOG(HIAI_ENGINE_RUN_ARGS_NOT_RIGHT,
                    "Fail to resize input image!");
    return HIAI_ERROR;
  }

  // inference and send inference result
  return BatchInferenceProcess(image_handle, tran_data);
}
