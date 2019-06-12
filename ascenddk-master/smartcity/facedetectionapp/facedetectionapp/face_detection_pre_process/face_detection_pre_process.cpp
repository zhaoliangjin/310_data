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
#include <math.h>
#include "hiaiengine/ai_types.h"
#include "hiaiengine/data_type_reg.h"
#include "hiaiengine/log.h"
#include "face_detection_pre_process.h"

#define MAP_VA32BIT 0x200  // flags for mmap

HIAI_REGISTER_DATA_TYPE("ScaleInfoT", ScaleInfoT);
HIAI_REGISTER_DATA_TYPE("NewImageParaT", NewImageParaT);
HIAI_REGISTER_DATA_TYPE("BatchImageParaWithScaleT", BatchImageParaWithScaleT);
HIAI_REGISTER_DATA_TYPE("BatchPreProcessedImageT", BatchPreProcessedImageT);

const int kRankYuv420sp = 1;    // rank value for yuv420sp.
const int kFormatYuv420sp = 0;  // format value for yuv420sp.

FaceDetectionPreProcess::FaceDetectionPreProcess() {
  dvpp_config_ = nullptr;
  batch_image_out_ = nullptr;
  batch_image_in_ = nullptr;
  dvpp_api_ = nullptr;
}

FaceDetectionPreProcess::~FaceDetectionPreProcess() {
  if (dvpp_api_ != nullptr) {
    DestroyDvppApi(dvpp_api_);
    dvpp_api_ = nullptr;
  }
  HIAI_ENGINE_LOG(HIAI_DEBUG_INFO, "[FDPreProcess] Engine Destory!!!\n");
}

HIAI_StatusT FaceDetectionPreProcess::Init(
    const hiai::AIConfig &config,
    const std::vector<hiai::AIModelDescription> &model_desc) {
  if (dvpp_config_ == nullptr) {
    dvpp_config_ = std::make_shared<DvppConfig>();
  }

  // read configuration parameters.
  for (int index = 0; index < config.items_size(); ++index) {
    const ::hiai::AIConfigItem &item = config.items(index);
    std::string name = item.name();
    std::string value = item.value();
    if (name == "resize_height") {
      // input check.
      if (!IsStrNum(value)) {
        HIAI_ENGINE_LOG(HIAI_GRAPH_INVALID_VALUE,
                        "[FDPreProcess] invalid item value: %s", value.c_str());
        return HIAI_ERROR;
      }
      dvpp_config_->resize_height = atof(value.data());
    } else if (name == "resize_width") {
      if (!IsStrNum(value)) {
        HIAI_ENGINE_LOG(HIAI_GRAPH_INVALID_VALUE,
                        "[FDPreProcess] invalid item value: %s", value.c_str());
        return HIAI_ERROR;
      }
      dvpp_config_->resize_width = atof(value.data());
    }
  }

  // create dvppapi handle.
  if (CreateDvppApi(dvpp_api_) != DVPP_SUCCESS) {
    HIAI_ENGINE_LOG(HIAI_ENGINE_RUN_ARGS_NOT_RIGHT,
                    "[FDPreProcess] create DVPP api failed.");
    return HIAI_ERROR;
  }
  HIAI_ENGINE_LOG(HIAI_DEBUG_INFO,
                  "[FDPreProcess] create DVPP api successfully.");
  return HIAI_OK;
}

HIAI_StatusT FaceDetectionPreProcess::HandleDvpp() {
  if (batch_image_in_ == nullptr || batch_image_in_->v_img.empty()) {
    HIAI_ENGINE_LOG(HIAI_ENGINE_RUN_ARGS_NOT_RIGHT,
                    "[FDPreProcess] input batch is null!");
    return HIAI_ERROR;
  }
  if (batch_image_out_ == nullptr) {
    batch_image_out_ = std::make_shared<BatchPreProcessedImageT>();
  }

  for (std::vector<NewImageParaT>::iterator iter =
           batch_image_in_->v_img.begin();
       iter != batch_image_in_->v_img.end(); ++iter) {
    // images which format is not in the branch judgment will be discarded.
    if (iter->img.format == hiai::IMAGEFORMAT::YUV420SP) {
      batch_image_out_->original_imgs.push_back(*iter);
      if (HandleVpc(iter->img) != HIAI_OK) {
        return HIAI_ERROR;
      }
    }
  }
  return HIAI_OK;
}

HIAI_StatusT FaceDetectionPreProcess::HandleVpc(
    const ImageData<u_int8_t> &img) {
  HIAI_ENGINE_LOG(HIAI_DEBUG_INFO, "[FDPreProcess] call DVPP vpc process! \n");

  // the width and height of the input image stored in memory should
  // be aligned by 128-byte and 16-byte.
  int align_width = ALIGN_UP(img.width, VPC_OUT_WIDTH_STRIDE);
  int align_height = ALIGN_UP(img.height, VPC_OUT_HIGH_STRIDE);

  // the YUV semi-planar looks like this:
  // ----------------------
  // |     Y      | Cb|Cr |
  // ----------------------
  // each chroma value is shared between 4 luma-pixels.
  // Y = width x height pixels.
  // Cb = Y / 4 pixels.
  // Cr = Y / 4 pixels.
  // so,the numer of pixles = width * height * 3 / 2 .
  int align_image_len = align_width * align_height * 3 / 2;

  // allocate memory for input buffer with huge memory table.
  // start from 0 , length:alignImageLength
  // if flags have the value MAP_ANONYMOUS, the fd value will be set to -1.
  // the start and offset value are usually set to 0.
  char *align_buffer = (char *)mmap(
      0, align_image_len, PROT_READ | PROT_WRITE,
      MAP_PRIVATE | MAP_ANONYMOUS | MAP_HUGETLB | MAP_VA32BIT, -1, 0);
  if (align_buffer == nullptr) {
    HIAI_ENGINE_LOG(HIAI_ENGINE_RUN_ARGS_NOT_RIGHT,
                    "[FDPreProcess] vpc can not alloc input buffer.");
    return HIAI_ERROR;
  }

  // copy image data to aligned buffer.
  char *image_buffer = (char *)(img.data.get());

  // copy luma.
  for (uint32_t i = 0; i < img.height; i++) {
    error_t err = memcpy_s(align_buffer + i * align_width, img.width,
                           image_buffer + i * img.width, img.width);
    // if memcpy return error , free up alignBuffer.
    if (err != EOK) {
      HIAI_ENGINE_LOG(HIAI_ENGINE_RUN_ARGS_NOT_RIGHT,
                      "[FDPreProcess] memcpy failed. error:%d", err);
      munmap(align_buffer, (unsigned int)ALIGN_UP(align_image_len, MAP_2M));
      return HIAI_ERROR;
    }
  }

  // copy chroma.
  for (uint32_t i = 0; i < img.height / 2; i++) {
    error_t err = memcpy_s(
        align_buffer + i * align_width + align_width * align_height, img.width,
        image_buffer + i * img.width + img.width * img.height, img.width);

    if (err != EOK) {
      HIAI_ENGINE_LOG(HIAI_ENGINE_RUN_ARGS_NOT_RIGHT,
                      "[FDPreProcess] memcpy failed. error:%d", err);
      munmap(align_buffer, (unsigned int)ALIGN_UP(align_image_len, MAP_2M));
      return HIAI_ERROR;
    }
  }

  // call dvpp to resize images.
  HIAI_StatusT ret =
      HandleVpcWithParam((unsigned char *)align_buffer, align_image_len, img,
                         align_width, align_height);

  // free up memory.
  munmap(align_buffer, (unsigned int)ALIGN_UP(align_image_len, MAP_2M));

  if (ret != HIAI_OK) {
    // dvpp processed error.
    HIAI_ENGINE_LOG(HIAI_ENGINE_RUN_ARGS_NOT_RIGHT,
                    "[FDPreProcess] DVPP_CTL_VPC_PROC process faild!\n");
    return HIAI_ERROR;
  }
  return HIAI_OK;
}

HIAI_StatusT FaceDetectionPreProcess::HandleVpcWithParam(
    unsigned char *buffer, long buffer_size, const ImageData<u_int8_t> &img,
    int align_width, int align_height) {
  int real_width = img.width;
  int real_height = img.height;

  HIAI_ENGINE_LOG(HIAI_DEBUG_INFO,
                  "[FDPreProcess] input image size:width: %d, height: %d",
                  real_width, real_height);

  // create dvppapi handle failed.
  if (dvpp_api_ == nullptr) {
    HIAI_ENGINE_LOG(HIAI_ENGINE_RUN_ARGS_NOT_RIGHT,
                    "[FDPreProcess] dvppApi is null!\n");
    return HIAI_ERROR;
  }

  vpc_in_msg vpc_input_param;  // vpc input parrameter.

  vpc_input_param.format = kFormatYuv420sp;  // yuv420sp
  vpc_input_param.rank = kRankYuv420sp;

  vpc_input_param.width = align_width;
  vpc_input_param.high = align_height;
  vpc_input_param.stride = align_width;
  vpc_input_param.bitwidth = 8;  // the default value is 8.

  // maximum and minimum offset from the origin.
  // the maximum value must be an odd number.
  real_width = (real_width % 2 == 0) ? (real_width) : (real_width - 1);
  real_height = (real_height % 2 == 0) ? (real_height) : (real_height - 1);

  // from 0 to realWidth - 1
  vpc_input_param.hmin = 0;
  vpc_input_param.hmax = real_width - 1;
  // from 0 to realHeight- 1
  vpc_input_param.vmin = 0;
  vpc_input_param.vmax = real_height - 1;

  // caculate the aspect ratio after resizing init with 1.
  vpc_input_param.hinc = 1.0;
  vpc_input_param.vinc = 1.0;
  if (dvpp_config_->resize_width > 0 && dvpp_config_->resize_height > 0) {
    // the original width equals to vpcInMsg.hmax - vpcInMsg.hmin + 1.
    // the aspect ratio equals to resize_width / original width.
    vpc_input_param.hinc = dvpp_config_->resize_width /
                           (vpc_input_param.hmax - vpc_input_param.hmin + 1);
    vpc_input_param.vinc = dvpp_config_->resize_height /
                           (vpc_input_param.vmax - vpc_input_param.vmin + 1);
  }

  // input image buffer and buffer size.
  vpc_input_param.in_buffer = (char *)buffer;
  vpc_input_param.in_buffer_size = buffer_size;

  vpc_input_param.auto_out_buffer_1 = std::make_shared<AutoBuffer>();

  // dvpp input param struct.
  dvppapi_ctl_msg dvppapi_in_msg;
  dvppapi_in_msg.in = (void *)(&vpc_input_param);
  dvppapi_in_msg.in_size = sizeof(vpc_in_msg);

  resize_param_in_msg resize_in_param;
  resize_param_out_msg resize_out_param;

  resize_in_param.src_width = vpc_input_param.width;
  resize_in_param.src_high = vpc_input_param.high;
  resize_in_param.hmax = vpc_input_param.hmax;
  resize_in_param.hmin = vpc_input_param.hmin;
  resize_in_param.vmax = vpc_input_param.vmax;
  resize_in_param.vmin = vpc_input_param.vmin;

  // the width and heigth of output image.
  // vpcInMsg.hmax - vpcInMsg.hmin + 1 equals to the original width.
  // floor( x + 0.5) will return the rounding value of x.
  resize_in_param.dest_width = floor(
      vpc_input_param.hinc * (vpc_input_param.hmax - vpc_input_param.hmin + 1) +
      0.5);
  resize_in_param.dest_high = floor(
      vpc_input_param.vinc * (vpc_input_param.vmax - vpc_input_param.vmin + 1) +
      0.5);

  dvppapi_ctl_msg dvppapi_resize_msg;
  dvppapi_resize_msg.in = (void *)(&resize_in_param);
  dvppapi_resize_msg.out = (void *)(&resize_out_param);

  // step1. call dvpp to caculate the cropping and resizing parameters
  // with command "DVPP_CTL_TOOL_CASE_GET_RESIZE_PARAM".
  // return DVPP_SUCCESS if success.
  if (DvppCtl(dvpp_api_, DVPP_CTL_TOOL_CASE_GET_RESIZE_PARAM,
              &dvppapi_resize_msg) != DVPP_SUCCESS) {
    HIAI_ENGINE_LOG(HIAI_ENGINE_RUN_ARGS_NOT_RIGHT,
                    "[FDPreProcess] call dvppctl process:RESIZE faild!\n");
    return HIAI_ERROR;
  }

  // step2. call dvpp once again to generate the new image with command
  // "DVPP_CTL_VPC_PROC" and output params according to the previous step.
  vpc_input_param.hmax = resize_out_param.hmax;
  vpc_input_param.hmin = resize_out_param.hmin;
  vpc_input_param.vmax = resize_out_param.vmax;
  vpc_input_param.vmin = resize_out_param.vmin;
  vpc_input_param.vinc = resize_out_param.vinc;
  vpc_input_param.hinc = resize_out_param.hinc;

  // return DVPP_SUCCESS if sucess.
  if (DvppCtl(dvpp_api_, DVPP_CTL_VPC_PROC, &dvppapi_in_msg) != DVPP_SUCCESS) {
    HIAI_ENGINE_LOG(HIAI_ENGINE_RUN_ARGS_NOT_RIGHT,
                    "[FDPreProcess] call dvppctl process:VPC faild!\n");
    return HIAI_ERROR;
  }

  // create NewImageParaT pointer for new image.
  std::shared_ptr<NewImageParaT> image = std::make_shared<NewImageParaT>();

  // vpcInMsg.hmax - vpcInMsg.hmin + 1 equals to the original width.
  // original width * aspect ratio equals the new width after resizing.
  image->img.width =
      (uint32_t)(1.0 * (vpc_input_param.hmax - vpc_input_param.hmin + 1) *
                 vpc_input_param.hinc);
  image->img.height =
      (uint32_t)(1.0 * (vpc_input_param.vmax - vpc_input_param.vmin + 1) *
                 vpc_input_param.vinc);
  image->img.size = vpc_input_param.auto_out_buffer_1->getBufferSize();
  image->img.channel = img.channel;
  image->img.format = img.format;

  image->scale_info.scale_width = (1.0 * image->img.width) / img.width;
  image->scale_info.scale_height = (1.0 * image->img.height) / img.height;

  std::shared_ptr<u_int8_t> out_buffer = std::shared_ptr<u_int8_t>(
      new u_int8_t[image->img.size], std::default_delete<u_int8_t[]>());

  error_t err =
      memcpy_s(out_buffer.get(), image->img.size,
               vpc_input_param.auto_out_buffer_1->getBuffer(), image->img.size);

  if (err != EOK) {
    HIAI_ENGINE_LOG(HIAI_ENGINE_RUN_ARGS_NOT_RIGHT,
                    "[FDPreProcess] luma memcpy failed. error:%d", err);
    return HIAI_ERROR;
  }

  image->img.data = out_buffer;
  HIAI_ENGINE_LOG(HIAI_DEBUG_INFO,
                  "[FDPreProcess] output image size:width: %d , height: %d",
                  image->img.width, image->img.height);
  // add the new image to output parameters.
  batch_image_out_->processed_imgs.push_back(*image);

  return HIAI_OK;
}

HIAI_StatusT FaceDetectionPreProcess::SendImage() {
  if (batch_image_out_ == nullptr || batch_image_out_->original_imgs.empty()) {
    HIAI_ENGINE_LOG(HIAI_ENGINE_RUN_ARGS_NOT_RIGHT,
                    "[FDPreProcess] Out put data is empty.");
    return HIAI_ERROR;
  }

  batch_image_out_->batch_info = batch_image_in_->b_info;

  // send the output parameters to the next engine.
  HIAI_StatusT ret = SendData(0, "BatchPreProcessedImageT",
                              std::static_pointer_cast<void>(batch_image_out_));

  if (ret != HIAI_OK) {
    HIAI_ENGINE_LOG(HIAI_ENGINE_RUN_ARGS_NOT_RIGHT,
                    "[FDPreProcess] SendData failed! code:%d", ret);
    return HIAI_ERROR;
  }
  return HIAI_OK;
}

void FaceDetectionPreProcess::ClearData() {
  if (batch_image_out_ != nullptr) {
    batch_image_out_ = nullptr;
  }
}

/**
 * register a HIAI engine named "FaceDetectionPreProcess" and implement its
 * process.
 */
HIAI_IMPL_ENGINE_PROCESS("face_detection_pre_process", FaceDetectionPreProcess,
                         INPUT_SIZE) {
  if (arg0 == nullptr) {
    HIAI_ENGINE_LOG(HIAI_ENGINE_RUN_ARGS_NOT_RIGHT,
                    "[FDPreProcess] fail to process invalid message");
    return HIAI_ERROR;
  }

  // get input parameters.
  batch_image_in_ = std::static_pointer_cast<BatchImageParaWithScaleT>(arg0);

  // clear output parameters before processing.
  ClearData();

  // the main preprocessing function
  // 1.check the format of the image or video.
  // 2.unzipping,cropping and resizing if needed
  // 3.generate a new picture
  HIAI_StatusT ret = HandleDvpp();

  if (ret != HIAI_OK) {
    HIAI_ENGINE_LOG(HIAI_ENGINE_RUN_ARGS_NOT_RIGHT,
                    "[FDPreProcess] dvpp process error!");
    return HIAI_ERROR;
  }

  // send the image or video to the next engine after processing.
  return SendImage();
}

bool FaceDetectionPreProcess::IsStrNum(const string &str) {
  if (str == "") {
    return false;
  }
  for (auto &c : str) {
    if (c >= '0' && c <= '9') {
      continue;
    } else {
      return false;
    }
  }
  return true;
}
