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
#include "hiaiengine/ai_memory.h"
// 注册序列化和反序列化函数
/**
* @ingroup hiaiengine
* @brief GetTransSearPtr,        序列化Trans数据
* @param [in] : data_ptr         结构体指针
* @param [out]：struct_str       结构体buffer
* @param [out]：data_ptr         结构体数据指针buffer
* @param [out]：struct_size      结构体大小
* @param [out]：data_size        结构体数据大小
* @author w00437212
*/
void GetTransSearPtr(void* data_ptr, std::string& struct_str,
    uint8_t*& buffer, uint32_t& buffer_size)
{
    EngineTransNewT* engine_trans = (EngineTransNewT*)data_ptr;
    // 获取结构体buffer和size
    struct_str  = std::string((char*)data_ptr, sizeof(EngineTransNewT));

    // 获取结构体数据buffer和size
    buffer = (uint8_t*)engine_trans->trans_buff.get();
    buffer_size = engine_trans->buffer_size;
}

/**
* @ingroup hiaiengine
* @brief GetTransSearPtr,             反序列化Trans数据
* @param [in] : ctrl_ptr              结构体指针
* @param [in] : data_ptr              结构体数据指针
* @param [out]：std::shared_ptr<void> 传给Engine的指针结构体指针
* @author w00437212
*/
std::shared_ptr<void> GetTransDearPtr(
    const char* ctrlPtr, const uint32_t& ctrlLen,
    const uint8_t* dataPtr, const uint32_t& dataLen)
{
    std::shared_ptr<EngineTransNewT> engine_trans_ptr = std::make_shared<EngineTransNewT>();
    // 给engine_trans_ptr赋值
    engine_trans_ptr->buffer_size = ((EngineTransNewT*)ctrlPtr)->buffer_size;
    engine_trans_ptr->trans_buff.reset((unsigned char*)dataPtr, hiai::HIAIMemory::HIAI_DFree);
    return std::static_pointer_cast<void>(engine_trans_ptr);
}

// 注册EngineTransNewT
HIAI_REGISTER_SERIALIZE_FUNC("EngineTransNewT", EngineTransNewT, GetTransSearPtr, GetTransDearPtr);

