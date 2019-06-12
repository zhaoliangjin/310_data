/******************************************************************************
* Copyright (C), 2010-2020, Huawei Tech. Co., Ltd.
* File name      : hw_ivs_dvpp.h
* Version        : 1.00
* Date           : 2018-7=18
* Description    : 封装DVPP接口 
*******************************************************************************/
 
#ifndef __INC_SAMPLE_DVPP_H__
#define __INC_SAMPLE_DVPP_H__

#include "hiaiengine/profile.h"
#include "dvpp/idvppapi.h"
#include "hiaiengine/api.h"
#include "hiaiengine/data_type.h"
#include "hiaiengine/ai_memory.h"
// 宏定义
#define MIN_RESIZE (0.03125)
#define MAX_RESIZE  (4)
#define  CHECK_INC_VALUE(vincTemp, hincTemp)   (((vincTemp >= MIN_RESIZE) && (vincTemp <= MAX_RESIZE)) && \
	        										((hincTemp >= MIN_RESIZE) && (hincTemp <= MAX_RESIZE)))
#define DVPP_IMAGE_DATA_ALIGN_W			128
#define DVPP_IMAGE_DATA_ALIGN_H			16
#define DVPP_IMAGE_ADDR_ALIGN			128
#define DVPP_OUT_IMAGE_ALIGN_W_SIZE 	128
#define DVPP_OUT_IMAGE_ALIGN_H_SIZE 	2

HIAI_StatusT CropAndResizeByDvpp(IDVPPAPI *pDvppApi,
        const hiai::ImageData<uint8_t> & inImage,
        shared_ptr<AutoBuffer> autoOutBuffer,
        int32_t &outImageSize, \
        const hiai::Rectangle<hiai::Point2D> & rectangle,
        uint32_t outWidth, uint32_t outHeight);

namespace hiai {
// AlignBuffer
class AlignBuff{
  public:
    /**
    * @ingroup IvsAlignBuff
    * @brief: 构造函数
    * @param [in] buffSize:buffer大小
    * @return :
    */
    AlignBuff(const uint32_t& buffSize) :
        pBufferSize(buffSize),
        pBuffer(nullptr) {
            InitAlignBuff();
    }

    /**
    * @ingroup IvsAlignBuff
    * @brief GetBuffer: 获取对齐后的Buffer地址
    * @return char*:对齐后的Buffer地址
    */
    char* GetBuffer() {
        return pBuffer;
    }

    /**
    * @ingroup AlignBuff
    * @brief GetBufferSize: 获取对齐后的Buffer大小
    * @return char*:对齐后的Buffer大小
    */
    HIAI_StatusT InitAlignBuff() {
        HIAI_StatusT ret = HIAI_OK;
        if (pBufferSize == 0) {
            HIAI_ENGINE_LOG(HIAI_GRAPH_HDC_MEMORY_ADDR_NOT_ALIGN,
                "Init alignBuffer failed due to bufferSize is 0");
            return HIAI_GRAPH_HDC_MEMORY_ADDR_NOT_ALIGN;
        }

        if (pBuffer != nullptr) {
            ret = hiai::HIAIMemory::HIAI_DFree(pBuffer);
            if (ret != HIAI_OK) {
                HIAI_ENGINE_LOG(HIAI_GRAPH_HDC_MEMORY_ADDR_NOT_ALIGN,
                    "Init alignBuffer failed due to pBuffer is not nullptr and free failed");
                return HIAI_GRAPH_HDC_MEMORY_ADDR_NOT_ALIGN;
            }
            pBuffer = nullptr;
        }

        ret = hiai::HIAIMemory::HIAI_DMalloc(pBufferSize,
            (void*&)pBuffer, 0, hiai::HIAI_MEMORY_HUGE_PAGE);

        if (ret != HIAI_OK || pBuffer == nullptr) {
            HIAI_ENGINE_LOG(HIAI_GRAPH_HDC_MEMORY_ADDR_NOT_ALIGN,
                "Init alignBuffer failed due to HIAI_DMalloc failed");
            return HIAI_GRAPH_HDC_MEMORY_ADDR_NOT_ALIGN;
        }

        return ret;
    }

    /**
    * @ingroup IvsAlignBuff
    * @brief: 析构函数
    * @return :
    */
    ~AlignBuff() {
        if (pBuffer != nullptr) {
            hiai::HIAIMemory::HIAI_DFree(pBuffer);
            pBuffer = nullptr;
        }
    }
      private:
        uint32_t pBufferSize;
        char *pBuffer;
};
}

#endif // #ifndef __INC_SAMPLE_DVPP_H__

#pragma once

