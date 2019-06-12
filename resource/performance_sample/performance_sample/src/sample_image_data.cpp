/**
* @file sample_main.cpp
*
* Copyright (C) <2018>  <Huawei Technologies Co., Ltd.>. All Rights Reserved.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
*/
#include "sample_data.h"
#include "hiaiengine/data_type.h"
using hiai::ImageData;

// 注册序列化和反序列化函数
/**
* @ingroup hiaiengine
* @brief GetTransSearPtr,        序列化Trans数据
* @param [in] : dataPtr          结构体指针
* @param [out]：structStr        结构体buffer
* @param [out]：buffer           结构体数据指针buffer
* @param [out]：structSize       结构体大小
* @param [out]：dataSize         结构体数据大小
* @author w00437212
*/
void GetImageDataSearPtr(void* dataPtr, std::string& structStr, uint8_t*& buffer,
    uint32_t& bufferSize)
{
    ImageData<uint8_t>* image_data = (ImageData<uint8_t>*)dataPtr;
    structStr = std::string((char*)dataPtr, sizeof(ImageData<uint8_t>));

    buffer = (uint8_t*)image_data->data.get();
    bufferSize = image_data->size;
    
}


/**
* @ingroup hiaiengine
* @brief GetTransSearPtr,             反序列化Trans数据
* @param [in] : ctrlPtr               结构体指针
* @param [in] : dataPtr               结构体数据指针
* @param [out]：std::shared_ptr<void> 传给Engine的指针结构体指针
* @author w00437212
*/

std::shared_ptr<void> GetImageDearPtr(const char* ctrlPtr, const uint32_t& ctrlLen,
    const uint8_t* dataPtr, const uint32_t& datalen)
{
    ImageData<uint8_t>* imgCtrlPtr = (ImageData<uint8_t>*)ctrlPtr;
    std::shared_ptr<ImageData<uint8_t>> imageDataPtr = std::make_shared<ImageData<uint8_t>>();
    imageDataPtr->width = imgCtrlPtr->width;
    imageDataPtr->size = imgCtrlPtr->size;
    
    imageDataPtr->data.reset((uint8_t*)dataPtr, hiai::Graph::ReleaseDataBuffer);
    return std::static_pointer_cast<void>(imgCtrlPtr);
}
HIAI_REGISTER_HIAI_DATA_FUNC("ImageData_uint8", ImageData, GetImageDataSearPtr, GetImageDearPtr);



