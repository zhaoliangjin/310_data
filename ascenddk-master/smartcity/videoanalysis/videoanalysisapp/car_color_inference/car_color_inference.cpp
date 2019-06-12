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
#include "car_color_inference.h"

#include <thread>
#include <fstream>
#include <algorithm>
#include <iostream>
#include <vector>

#include <unistd.h>

#include <hiaiengine/log.h>
#include <hiaiengine/ai_types.h>
#include "hiaiengine/ai_model_parser.h"

namespace {
// car color kind
const string kCarColorClass[12] = { "black", "blue", "brown", "gold", "green",
    "grey", "maroon", "orange", "red", "silver", "white", "yellow" };
// time for waitting when send queue is full.
const int kWaitTime = 20000;
// the image width for model.
const int kDestImageWidth = 224;
// the image height for model.
const int kDestImageHeight = 224;
// the name of model_path in the config file
const string kModelPathItemName = "model_path";
// the name of passcode in the config file
const string kPasscodeItemName = "passcode";
// the name of batch_size in the config file
const string kBatchSizeItemName = "batch_size";
}

HIAI_REGISTER_DATA_TYPE("BatchCarInfoT", BatchCarInfoT);
HIAI_REGISTER_DATA_TYPE("VideoImageInfoT", VideoImageInfoT);
HIAI_REGISTER_DATA_TYPE("CarInfoT", CarInfoT);
HIAI_REGISTER_DATA_TYPE("BatchCroppedImageParaT", BatchCroppedImageParaT);

HIAI_StatusT CarColorInferenceEngine::Init(
    const hiai::AIConfig& config,
    const std::vector<hiai::AIModelDescription>& model_desc) {
  HIAI_ENGINE_LOG("[CarColorInferenceEngine] start init!");
  hiai::AIStatus ret = hiai::SUCCESS;

  if (ai_model_manager_ == nullptr) {
    ai_model_manager_ = std::make_shared<hiai::AIModelManager>();
  }

  std::vector<hiai::AIModelDescription> model_desc_vec;
  hiai::AIModelDescription model_description;

  for (int index = 0; index < config.items_size(); ++index) {
    const ::hiai::AIConfigItem& item = config.items(index);
    if (item.name() == kModelPathItemName) {
      const char* model_path = item.value().data();
      model_description.set_path(model_path);

    } else if (item.name() == kPasscodeItemName) {
      const char* passcode = item.value().data();
      model_description.set_key(passcode);
    } else if (item.name() == kBatchSizeItemName) {
      std::stringstream ss(item.value());
      ss >> batch_size_;
    }
  }

  model_desc_vec.push_back(model_description);
  ret = ai_model_manager_->Init(config, model_desc_vec);
  if (ret != hiai::SUCCESS) {
    return HIAI_ERROR;
  }

  HIAI_ENGINE_LOG("[CarColorInferenceEngine] end init!");
  return HIAI_OK;
}

HIAI_StatusT CarColorInferenceEngine::SendResultData(
    const std::shared_ptr<BatchCarInfoT>& tran_data) {
  HIAI_StatusT hiai_ret = HIAI_OK;

  do {
    // this engine only have one outport, this port parameter be set to zero.
    hiai_ret = SendData(0, "BatchCarInfoT",
                        std::static_pointer_cast<void>(tran_data));
    if (hiai_ret == HIAI_QUEUE_FULL) {
      HIAI_ENGINE_LOG("[CarColorInferenceEngine] queue full");
      usleep(kWaitTime);
    }
  } while (hiai_ret == HIAI_QUEUE_FULL);

  if (hiai_ret != HIAI_OK) {
    HIAI_ENGINE_LOG("[CarColorInferenceEngine] send finished data failed!");
    return HIAI_ERROR;
  }

  return HIAI_OK;
}

void CarColorInferenceEngine::BatchImageResize(
    std::shared_ptr<BatchCroppedImageParaT>& batch_image_input,
    std::shared_ptr<BatchCroppedImageParaT>& batch_image_output) {
  batch_image_output->video_image_info = batch_image_input->video_image_info;
  // resize for each image
  for (std::vector<ObjectImageParaT>::iterator iter = batch_image_input
      ->obj_imgs.begin(); iter != batch_image_input->obj_imgs.end(); ++iter) {
    ascend::utils::DvppCropOrResizePara dvpp_resize_param;

    /**
     * when use dvpp_process only for resize function:
     *
     * 1.DVPP limits horz_max and vert_max should be Odd number,
     * if it is even number, subtract 1, otherwise Equal to origin width
     * or height.
     *
     * 2.horz_min and vert_min should be set to zero.
     */
    dvpp_resize_param.horz_max =
        iter->img.width % 2 == 0 ? iter->img.width - 1 : iter->img.width;
    dvpp_resize_param.horz_min = 0;
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
    if (ret != ascend::utils::kDvppOperationOk) {
      HIAI_ENGINE_LOG(
          "[CarColorInferenceEngine] resize image failed with code %d !", ret);
      continue;
    }

    std::shared_ptr<ObjectImageParaT> obj_image = std::make_shared<
        ObjectImageParaT>();

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

bool CarColorInferenceEngine::ConstructBatchBuffer(
    int batch_index,
    const std::shared_ptr<BatchCroppedImageParaT>& image_handle,
    uint8_t* temp) {
  bool is_successed = true;
  int image_number = image_handle->obj_imgs.size();
  int image_size = image_handle->obj_imgs[0].img.size * sizeof(uint8_t);

  //the loop for each image
  for (int j = 0; j < batch_size_; j++) {
    if (batch_index + j < image_number) {
      errno_t err = memcpy_s(
          temp + j * image_size, image_size,
          image_handle->obj_imgs[batch_index + j].img.data.get(), image_size);
      if (err != EOK) {
        HIAI_ENGINE_LOG(
            "[CarTypeInferenceEngine] ERROR, copy image buffer failed");
        is_successed = false;
        break;
      }
    } else {
      errno_t err = memset_s(temp + j * image_size, image_size,
                             static_cast<char>(0), image_size);
      if (err != EOK) {
        HIAI_ENGINE_LOG(
            "[CarTypeInferenceEngine] batch padding for image data failed");
        is_successed = false;
        break;
      }
    }
  }

  return is_successed;
}

bool CarColorInferenceEngine::ConstructInferenceResult(
    const std::vector<std::shared_ptr<hiai::IAITensor> >& output_data_vec,
    int batch_index,
    const std::shared_ptr<BatchCroppedImageParaT>& image_handle,
    const std::shared_ptr<BatchCarInfoT>& tran_data) {

  if (output_data_vec.size() == 0) {
    HIAI_ENGINE_LOG("[CarColorInferenceEngine] output_data_vec is null!");
    return false;
  }
  int image_number = image_handle->obj_imgs.size();

  for (int n = 0; n < output_data_vec.size(); ++n) {
    std::shared_ptr<hiai::AINeuralNetworkBuffer> result_tensor =
        std::static_pointer_cast<hiai::AINeuralNetworkBuffer>(
            output_data_vec[n]);
    //get confidence result
    int size = result_tensor->GetSize() / sizeof(float);
    float* result = (float*) result_tensor->GetBuffer();
    // analyze each batch result
    for (int batch_result_index = 0; batch_result_index < size;
        batch_result_index += size / batch_size_) {
      int max_confidence_index = batch_result_index;
      //find max confidence for each image
      for (int index = 0; index < size / batch_size_; index++) {
        if (*(result + batch_result_index + index)
            > *(result + max_confidence_index)) {
          max_confidence_index = batch_result_index + index;
        }
      }
      // creat out struct for each batch
      CarInfoT out;
      if (batch_index + batch_result_index / (size / batch_size_)
          < image_number) {
        out.object_id = image_handle->obj_imgs[batch_index
            + batch_result_index / (size / batch_size_)].object_info.object_id;
        out.label = max_confidence_index - batch_result_index;
        out.attribute_name = kCarColor;
        out.inference_result = kCarColorClass[max_confidence_index
            - batch_result_index];
        out.confidence = *(result + max_confidence_index);
        tran_data->car_infos.push_back(out);

      }
    }
  }
  return true;
}

HIAI_StatusT CarColorInferenceEngine::BatchInferenceProcess(
    const std::shared_ptr<BatchCroppedImageParaT>& image_handle,
    std::shared_ptr<BatchCarInfoT> tran_data) {
  HIAI_ENGINE_LOG("[CarColorInferenceEngine] start process!");

  hiai::AIStatus ret = hiai::SUCCESS;
  HIAI_StatusT hiai_ret = HIAI_OK;
  int image_number = image_handle->obj_imgs.size();
  int image_size = image_handle->obj_imgs[0].img.size * sizeof(uint8_t);
  int batch_buffer_size = image_size * batch_size_;

  // the loop for each batch ,maybe image_number greater than batch_size_
  for (int i = 0; i < image_number; i += batch_size_) {

    std::vector<std::shared_ptr<hiai::IAITensor> > input_data_vec;
    std::vector<std::shared_ptr<hiai::IAITensor> > output_data_vec;
    //1.prepare input buffer for each batch
    if (tran_data == nullptr) {
      tran_data = std::make_shared<BatchCarInfoT>();
    }

    // apply buffer for each batch
    uint8_t* temp = new uint8_t[batch_buffer_size];
    // Origin image information is transmitted to next Engine directly
    tran_data->video_image_info = image_handle->video_image_info;
    bool is_successed = true;

    is_successed = ConstructBatchBuffer(i, image_handle, temp);
    if (!is_successed) {
      HIAI_ENGINE_LOG(
          "[CarColorInferenceEngine] batch input buffer construct failed!");
      delete[] temp;
      return HIAI_ERROR;
    }

    std::shared_ptr<hiai::AINeuralNetworkBuffer> neural_buffer =
        std::shared_ptr<hiai::AINeuralNetworkBuffer>(
            new hiai::AINeuralNetworkBuffer());
    neural_buffer->SetBuffer((void*) (temp), batch_buffer_size);
    std::shared_ptr<hiai::IAITensor> input_data = std::static_pointer_cast<
        hiai::IAITensor>(neural_buffer);
    input_data_vec.push_back(input_data);

    // 2.Call Process, Predict
    ret = ai_model_manager_->CreateOutputTensor(input_data_vec,
                                                output_data_vec);
    if (ret != hiai::SUCCESS) {
      HIAI_ENGINE_LOG("[CarColorInferenceEngine] CreateOutputTensor failed");
      delete[] temp;
      return HIAI_ERROR;
    }
    hiai::AIContext ai_context;
    HIAI_ENGINE_LOG(
        "[CarColorInferenceEngine] ai_model_manager_->Process start!");
    ret = ai_model_manager_->Process(ai_context, input_data_vec,
                                     output_data_vec, 0);
    if (ret != hiai::SUCCESS) {
      HIAI_ENGINE_LOG(
          "[CarColorInferenceEngine] ai_model_manager Process failed");
      delete[] temp;
      return HIAI_ERROR;
    }
    delete[] temp;
    input_data_vec.clear();

    //3.set the tran_data with the result of this batch

    is_successed = ConstructInferenceResult(output_data_vec, i, image_handle,
                                            tran_data);

    if (!is_successed) {
      HIAI_ENGINE_LOG(
          "[CarColorInferenceEngine] batch copy output buffer failed!");
      return HIAI_ERROR;
    }
    //4. send the result
    hiai_ret = SendResultData(tran_data);
    if (hiai_ret != HIAI_OK) {
      HIAI_ENGINE_LOG(
          "[CarColorInferenceEngine] SendData failed! error code: %d",
          hiai_ret);
    }

    //5. release sources
    tran_data = nullptr;
  }

  HIAI_ENGINE_LOG("[CarColorInferenceEngine] end process!");
  return HIAI_OK;
}

HIAI_IMPL_ENGINE_PROCESS("car_color_inference", CarColorInferenceEngine,
                         INPUT_SIZE) {
  HIAI_StatusT hiai_ret = HIAI_OK;
  std::shared_ptr<BatchCarInfoT> tran_data = std::make_shared<BatchCarInfoT>();
  std::shared_ptr<BatchCroppedImageParaT> image_input = std::make_shared<
      BatchCroppedImageParaT>();

  std::shared_ptr<BatchCroppedImageParaT> image_handle = std::make_shared<
      BatchCroppedImageParaT>();

  if (arg0 == nullptr) {
    HIAI_ENGINE_LOG("[CarColorInferenceEngine] input data is null!");
    return HIAI_ERROR;
  }
  // this engine only need one queue, so the port should be set to zero.
  input_que_.PushData(0, arg0);
  if (!input_que_.PopAllData(image_input)) {
    HIAI_ENGINE_LOG("[CarColorInferenceEngine] fail to PopAllData");
    return HIAI_ERROR;
  }

  if (image_input == nullptr) {
    HIAI_ENGINE_LOG("[CarColorInferenceEngine] image_input is nullptr");
    return HIAI_ERROR;
  }

  // add is_finished for showing this data in dataset are all sended.
  if (image_input->video_image_info.is_finished == true) {
    tran_data->video_image_info = image_input->video_image_info;
    return SendResultData(tran_data);
  }

  // resize input image;
  BatchImageResize(image_input, image_handle);
  if (image_handle->obj_imgs.empty() == true) {
    HIAI_ENGINE_LOG("[CarColorInferenceEngine] image_input resize failed");
    return HIAI_ERROR;
  }
  // inference and send inference result;
  return BatchInferenceProcess(image_handle, tran_data);
}
