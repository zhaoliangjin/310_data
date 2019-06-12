/**
* @file sample_main.cpp
*
* Copyright (C) <2018>  <Huawei Technologies Co., Ltd.>. All Rights Reserved.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
*/
#include <fstream>
#include <iostream>
#include <hiaiengine/data_type_reg.h>
#include "sample_data.h"
#include "sample_device.h"
#include "sample_errorcode.h"
#include "sample_dvpp.h"
#include "common.h"
#include <stdlib.h>
#include <unistd.h>
#include "hiaiengine/data_type.h"
#include "hiaiengine/ai_types.h"
#include "mmpa/mmpa_api.h"
#include <stdint.h>
#include <string>
#include <vector>
#include <sys/mman.h>

using hiai::AIConfig;
using hiai::AIModelDescription;

// 全局变量
static uint32_t gJepgdOutWidth = 0;
static uint32_t gJepgdOutHeight = 0;
static uint32_t gJepgdInWidth = 0;
static uint32_t gJepgdInHeight = 0;

/*
*
* 样例流程ImageData->JpegdEngine(解码)->VpcEngine(CropAndResize)->AIStubEngine(推理)->返回结果给到Host
*
*/
/**
* @ingroup hiaiengine
* @brief   JpegeFreeBuffer      JpegdOutData的释放
* @return  void
*/
void JpegdEngine::JpegdFreeBuffer(void* ptr)
{
    if (jpegdOutData.yuv_data != nullptr) {
        jpegdOutData.cbFree();
    }
}

/**
* @ingroup hiaiengine
* @brief   JpegdEngine          JpegdEngine Process函数
* @return  uint32_t
*/
HIAI_IMPL_ENGINE_PROCESS("JpegdEngine", JpegdEngine, 1)
{
    struct jpegd_raw_data_info jpegdInData;
    IDVPPAPI *pidvppapi = nullptr;

    if ( arg0 == nullptr)
    {
        return HIAI_OK;
    }
    HIAI_ENGINE_LOG("[DEBUG] JpegdEngine Start Process");
    std::shared_ptr<EngineTransNewT> result =
        std::static_pointer_cast<EngineTransNewT>(arg0);

    // 构造输入数据
    jpegdInData.jpeg_data_size = result->buffer_size;
    jpegdInData.jpeg_data = reinterpret_cast<unsigned char*>(result->trans_buff.get());

    // 创建dvpp handle
    uint32_t ret = CreateDvppApi(pidvppapi);
    if (pidvppapi == nullptr || ret != DVPP_SUCCESS) {
        HIAI_ENGINE_LOG("VPC create dvppApi failed");
        return HIAI_CREATE_DVPP_ERROR;        
    }
    // 构造dvpp ctrl message
    dvppapi_ctl_msg dvppApiCtlMsg;
    dvppApiCtlMsg.in = (void*)&jpegdInData;
    dvppApiCtlMsg.in_size = sizeof( jpegdInData );
    dvppApiCtlMsg.out = (void*)&jpegdOutData;
    dvppApiCtlMsg.out_size = sizeof( jpegdOutData );

    // 执行DVPP进行Jpeg解码
    if(pidvppapi != nullptr) {
        if( 0 != DvppCtl( pidvppapi, DVPP_CTL_JPEGD_PROC, &dvppApiCtlMsg ) ) {
            HIAI_ENGINE_LOG("Jpegd Engine dvppctl error ");
            DestroyDvppApi(pidvppapi); 
            return HIAI_JPEGD_CTL_ERROR;
        }
        DestroyDvppApi(pidvppapi);
    } else {
        HIAI_ENGINE_LOG("Jpegd Engine can not open dvppapi");
        return HIAI_JPEGD_CTL_ERROR;
    }

    // 构造DVPP OUt数据并进行发送
    gJepgdOutWidth = jpegdOutData.img_width_aligned;
    gJepgdOutHeight = jpegdOutData.img_height_aligned;
    gJepgdInWidth = jpegdOutData.img_width;
    gJepgdInHeight = jpegdOutData.img_height;

    std::shared_ptr<EngineTransNewT> output = std::make_shared<EngineTransNewT>();
    ReleaseFunction releaseFunction = std::bind(&JpegdEngine::JpegdFreeBuffer, this, std::placeholders::_1);
    output->trans_buff.reset((uint8_t*)jpegdOutData.yuv_data, releaseFunction);
    output->buffer_size = jpegdOutData.yuv_data_size;
    // 发送数据
    if (HIAI_OK != SendData(0, gMsgType, output)) {
        HIAI_ENGINE_LOG("SendData wrong");
    }
    HIAI_ENGINE_LOG("[DEBUG] JpegdEngine End Process");
    return HIAI_OK;
}

/**
* @ingroup hiaiengine
* @brief   VpcEngine        VpcEngine Process，进行Crop/Resize
* @return  HIAI_StatusT     HIAI_OK 为正确，其他为错误
*/
HIAI_IMPL_ENGINE_PROCESS("VpcEngine", VpcEngine, 1)
{
    uint32_t ret = CREATE_DVPP_SUCCESS;
    IDVPPAPI *dvppApi = nullptr;
    if (arg0 == nullptr) {
        return HIAI_OK;
    }
    HIAI_ENGINE_LOG("[DEBUG] VpcEngine Start Process");
    std::shared_ptr<EngineTransNewT> result =
        std::static_pointer_cast<EngineTransNewT>(arg0);
    
    HIAI_ENGINE_LOG(HIAI_OK, "VpcEngine Process");
    ret = CreateDvppApi(dvppApi);
    if (dvppApi == nullptr || ret != DVPP_SUCCESS) {
        HIAI_ENGINE_LOG("VPC create dvppApi failed");
        return HIAI_CREATE_DVPP_ERROR;
    }

    // 填充图片数据
    hiai::ImageData<uint8_t> inImage;
    inImage.width = gJepgdOutWidth;
    inImage.height = gJepgdOutHeight;
    inImage.channel = 1;
    inImage.depth = 0;
    inImage.width_step = gJepgdOutWidth;
    inImage.size = gJepgdOutWidth * gJepgdOutHeight * SAMPLE_PARA_1 / SAMPLE_PARA_2;
    inImage.data = result->trans_buff;
    // Jpegd输出的宽高， 默认是128*16对齐的
    uint32_t outWidth = MODEL_WIDTH;
    uint32_t outHeight = MODEL_WIDTH;    
    int32_t outImageSize = IMG_W * IMG_H * SAMPLE_PARA_1 / SAMPLE_PARA_2; 
    hiai::Rectangle<hiai::Point2D> rectangle;
    rectangle.anchor_lt.x = 0;
    rectangle.anchor_lt.y = 0;
    rectangle.anchor_rb.x = gJepgdInWidth - 1;
    rectangle.anchor_rb.y = gJepgdInHeight - 1;

    if (outBuffer_ == nullptr) {
        outBuffer_ = make_shared<AutoBuffer>();
    }
    // 进行图片Crop和Resize
    ret = CropAndResizeByDvpp(dvppApi, inImage, outBuffer_, outImageSize,
        rectangle, outWidth, outHeight);
    if (HIAI_OK != ret) {
        HIAI_ENGINE_LOG("dvppprocess failed");
        return HIAI_CREATE_DVPP_ERROR;
    }
    // 处理结果下发
    std::shared_ptr<EngineTransNewT> output = std::make_shared<EngineTransNewT>();
    output->trans_buff.reset((uint8_t*)outBuffer_->getBuffer());    // 直接拿OuputBuffer给到推理引擎
    output->buffer_size = outWidth * outHeight * SAMPLE_PARA_1 / SAMPLE_PARA_2;

    // 发送数据数据给到
    if (HIAI_OK != SendData(0, gMsgType, output)) {
        HIAI_ENGINE_LOG("SendData wrong");
    }
    HIAI_ENGINE_LOG("[DEBUG] VpcEngine End Process");
    return HIAI_OK;
}

/**
* @ingroup hiaiengine
* @brief   AIStubEngine::Init  AI计算Engine初始化
* @param   [in] config:        config配置
* @param   [in] modelDesc:    模型描述
* @return  HIAI_StatusT        HIAI_OK 为正确，其他为错误
*/
HIAI_StatusT AIStubEngine::Init(const AIConfig &config,
    const std::vector<AIModelDescription>& modelDescInVec)
{
    hiai::AIStatus ret = hiai::SUCCESS;
    _config.clear();
    for (auto item : config.items()) {
        _config[item.name()] = item.value();
    }

    // AiModelManager Init
    if (nullptr == ai_model_manager_) {
        ai_model_manager_ = std::make_shared<hiai::AIModelManager>();
    }
    const char* modelPath = _config["model_path"].c_str();
    const char* modelName = _config["model_name"].c_str();

    std::vector<AIModelDescription> modelDescVec;
    hiai::AIModelDescription modelDesc;
    modelDesc.set_path(modelPath);
    modelDesc.set_name(modelName);
    modelDesc.set_key("");
    modelDescVec.push_back(modelDesc);
    // 模型初始化，加载
    ret = ai_model_manager_->Init(config, modelDescVec);

    if (hiai::SUCCESS != ret) {
        HIAI_ENGINE_LOG("AiModelManager init error");
        return HIAI_AI_MODEL_MANAGER_INIT_FAIL;
    }

    // 初始化的时候先准备好Output Buffer
    std::vector<hiai::TensorDimension> inputTensor;
    std::vector<hiai::TensorDimension> outputTensor;
    hiai::AIStatus ai_ret = ai_model_manager_->GetModelIOTensorDim(modelName, inputTensor, outputTensor);
    if (ai_ret != hiai::SUCCESS) {
        HIAI_ENGINE_LOG("AiModelManager GetModelIOTensorDim failed");
    }

    uint32_t out_size = outputTensor.size();
    for (uint32_t index = 0; index < out_size; index++) {
        std::shared_ptr<hiai::AINeuralNetworkBuffer> neuralBuffer = 
            std::shared_ptr<hiai::AINeuralNetworkBuffer>(new hiai::AINeuralNetworkBuffer());
        neuralBuffer->SetBuffer(new uint8_t[outputTensor[index].size], outputTensor[index].size);
        outputTensorBuffer_.push_back(neuralBuffer);
    }
    return HIAI_OK;
}

AIStubEngine::~AIStubEngine()
{
    // 删除outputBuffer
    for (auto& output: outputTensorBuffer_) {
        if (nullptr != std::static_pointer_cast<hiai::AINeuralNetworkBuffer>(output)->GetBuffer()) {
            delete[] (uint8_t*)std::static_pointer_cast<hiai::AINeuralNetworkBuffer>(output)->GetBuffer();
        }
    }          
}
/**
* @ingroup hiaiengine
* @brief   AIStubEngine::Process  AIStubEngine执行函数
* @return  HIAI_StatusT        HIAI_OK 为正确，其他为错误
*/
HIAI_IMPL_ENGINE_PROCESS("AIStubEngine", AIStubEngine, 1)
{
    hiai::AIStatus ret = hiai::SUCCESS;
    HIAI_StatusT hiaiRet = HIAI_OK;
    if (arg0 == nullptr) {
        HIAI_ENGINE_LOG(this, HIAI_INVALID_INPUT_MSG, "fail to process invalid message");
        return HIAI_INVALID_INPUT_MSG;
    }
    std::shared_ptr<EngineTransNewT> result =
        std::static_pointer_cast<EngineTransNewT>(arg0);
    
    HIAI_ENGINE_LOG("[DEBUG] AIStubEngine Start");
    // 模型处理
    std::vector<std::shared_ptr<hiai::IAITensor>> inputDataVec;
    uint32_t buffer_size = result->buffer_size;
    // 组织数据Buffer
    std::shared_ptr<hiai::AINeuralNetworkBuffer> neuralBuffer =
        std::shared_ptr<hiai::AINeuralNetworkBuffer>(new hiai::AINeuralNetworkBuffer());
    neuralBuffer->SetBuffer((void*)result->trans_buff.get(),  (uint32_t)(buffer_size*sizeof(char)));
    std::shared_ptr<hiai::IAITensor> inputData = std::static_pointer_cast<hiai::IAITensor>(neuralBuffer);
    inputDataVec.push_back(inputData);

    hiai::AIContext aiContext;
    ret = ai_model_manager_->Process(aiContext, inputDataVec, outputTensorBuffer_, 0);
    if (hiai::SUCCESS != ret) {
        return HIAI_AI_MODEL_MANAGER_PROCESS_FAIL;
    }

    // 根据输出结果发送计算结果给到Dest_Engine
    for (uint32_t index = 0; index < outputTensorBuffer_.size(); index++) {
        std::shared_ptr<hiai::AINeuralNetworkBuffer> outputData =
            std::static_pointer_cast<hiai::AINeuralNetworkBuffer>(outputTensorBuffer_[index]);
        std::shared_ptr<std::string> outputStringPtr =
            std::shared_ptr<std::string>(new std::string((char*)outputData->GetBuffer(), outputData->GetSize()));

        hiaiRet = SendData(0, "string", std::static_pointer_cast<void>(outputStringPtr));
        if (HIAI_OK != hiaiRet) {   
            HIAI_ENGINE_LOG(this, HIAI_SEND_DATA_FAIL, "fail to send data");
        }
    }
    HIAI_ENGINE_LOG("[DEBUG] AIStubEngine Start");
    return HIAI_OK;
}
