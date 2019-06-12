/**
* @file sample_data.cpp
*
* Copyright (C) <2018>  <Huawei Technologies Co., Ltd.>. All Rights Reserved.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
*/
#ifndef __INC_SAMPLE_DATA_H__
#define __INC_SAMPLE_DATA_H__
#include "hiaiengine/data_type.h"
#include "hiaiengine/data_type_reg.h"

// 注册Engine将流转的结构体
typedef struct EngineTransNew
{
    std::shared_ptr<uint8_t> trans_buff;    // 传输Buffer
    uint32_t buffer_size;                   // 传输Buffer大小
}EngineTransNewT;
#endif // __INC_SAMPLE_DATA_H__