/**
* @file sample_main.cpp
*
* Copyright (C) <2018>  <Huawei Technologies Co., Ltd.>. All Rights Reserved.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
*/
#include "sample_dvpp.h"
#include "sample_status.h"
#include "common.h"
#include "hiaiengine/profile.h"
#include "hiaiengine/ai_memory.h"
#include "mmpa/mmpa_api.h"
/**
* @ingroup hiaiengine
* @brief   AlignImageWith    对齐图片
* @param   [in] imageAddr:   图片buffer
* @param   [in] alingAddr:   图片buffer
* @param   [in] alingWith:   对齐宽
* @param   [in] alingHigh:   对齐高
* @param   [in] imageWith:   图片宽
* @param   [in] imageHight:  图片高
* @return  HIAI_StatusT      HIAI_OK
*/
HIAI_StatusT AlignImageWith(char *alingAddr,
    char *imageAddr, int32_t alingWith,
    int32_t alingHigh, int32_t imageWith,
    int32_t imageHight)
{
    int32_t tmpLen = 0;
    if ((nullptr == alingAddr)||(nullptr == imageAddr)) {
        HIAI_ENGINE_LOG(ALG_DVPP_CROP_PARA_WRONG, "AlignImageWith para error!");
        return ALG_DVPP_CROP_PARA_WRONG;
    }

    if (alingWith == imageWith) {
        if (alingHigh == imageHight) {
            tmpLen = imageWith * imageHight * SAMPLE_PARA_1 / SAMPLE_PARA_2;
            memcpy_s(alingAddr, tmpLen, imageAddr, tmpLen);
        }
        else if (alingHigh > imageHight) {
            tmpLen = imageWith * imageHight;
            memcpy_s(alingAddr, tmpLen, imageAddr, tmpLen);
            memcpy_s(alingAddr+alingHigh*alingWith,
                tmpLen / SAMPLE_PARA_2, imageAddr+tmpLen, tmpLen / SAMPLE_PARA_2);
        }
    }
    else if (alingWith > imageWith) {
        for (int32_t n=0; n < imageHight; n++) {
            memcpy_s(alingAddr+n*alingWith,
                imageWith, imageAddr+n*imageWith, imageWith);
        }

        for (int32_t n=0; n < imageHight / SAMPLE_PARA_2; n++) {
            memcpy_s(alingAddr+n*alingWith+alingHigh*alingWith,
                imageWith,
                imageAddr+n*imageWith+imageHight*imageWith,
                imageWith);  // UV
        }
    }
    else {
        HIAI_ENGINE_LOG(ALG_DVPP_CROP_PARA_WRONG,
            "AlignImageWith para error,alingWith[%d] < imageWith[%d]!",
            alingWith, imageWith);
        return ALG_DVPP_CROP_PARA_WRONG;
    }
    return HIAI_OK;
}

/**
* @ingroup hiaiengine
* @brief   ResizeOnceAgain       重启Resize
* @param   [in] pDvppApi :       DVPP指针
* @param   [in] image_buffer:    图片buffer
* @param   [in] outImage:        对齐宽
* @param   [in] outImageSize:    对齐高
* @param   [in] rectangle:       框
* @param   [in] outWidth:        输出宽
* @param   [in] outHeight:       输出高
* @return  HIAI_StatusT      HIAI_OK
*/
HIAI_StatusT ResizeOnceAgain(IDVPPAPI *pDvppApi,
    uint8_t *image_buffer,
    shared_ptr<AutoBuffer> autoOutBuffer,
    int32_t outImageSize,
    const hiai::Rectangle<hiai::Point2D> & rectangle,
    uint32_t outWidth,
    uint32_t outHeight)
{
    dvppapi_ctl_msg dvppApiCtlMsg;
    vpc_in_msg vpcInMsg;
    uint32_t iWidth =
        (uint32_t)(rectangle.anchor_rb.x - rectangle.anchor_lt.x);
    uint32_t iHeight =
        (uint32_t)(rectangle.anchor_rb.y - rectangle.anchor_lt.y);
    uint32_t hmin = 0;
    uint32_t vmin = 0;
    uint32_t hmax = (uint32_t)(hmin + (iWidth % SAMPLE_PARA_2 ? iWidth : (iWidth - 1)));
    uint32_t vmax = (uint32_t)(vmin + (iHeight % SAMPLE_PARA_2 ? iHeight : (iHeight - 1)));
    vpcInMsg.format = 0;
    vpcInMsg.rank = 0;
    vpcInMsg.width = iWidth;
    vpcInMsg.high = iHeight;
    vpcInMsg.stride = iWidth;

    uint32_t real_rect_Height = 0;
    uint32_t real_rect_iWidth = 0;
    HIAI_ENGINE_LOG("ResizeOnceAgain!");
    // 对齐原图128*16
    int32_t align_image_w =
        (iWidth % DVPP_IMAGE_DATA_ALIGN_W) ?
        ((iWidth/DVPP_IMAGE_DATA_ALIGN_W)* DVPP_IMAGE_DATA_ALIGN_W+DVPP_IMAGE_DATA_ALIGN_W):iWidth;
    int32_t align_image_h =
        (iHeight%DVPP_IMAGE_DATA_ALIGN_H) ?
        ((iHeight/DVPP_IMAGE_DATA_ALIGN_H) * DVPP_IMAGE_DATA_ALIGN_H+DVPP_IMAGE_DATA_ALIGN_H):iHeight;

    struct resize_param_in_msg resize_in_param;
    resize_in_param.src_width = align_image_w;  // 128对齐(image_buffer肯定是128对齐的)
    resize_in_param.src_high =  align_image_h;  // 16对齐
    resize_in_param.hmax = hmax;
    resize_in_param.hmin = hmin;
    resize_in_param.vmax = vmax;
    resize_in_param.vmin = vmin;
    resize_in_param.dest_width = outWidth;
    resize_in_param.dest_high = outHeight;
    struct resize_param_out_msg resize_out_param;
    dvppApiCtlMsg.in  = (void*)(&resize_in_param);
    dvppApiCtlMsg.out = (void*)(&resize_out_param);
    if (0 != DvppCtl(pDvppApi, DVPP_CTL_TOOL_CASE_GET_RESIZE_PARAM, &dvppApiCtlMsg)) {
        HIAI_ENGINE_LOG(ALG_DVPP_CTL_PROCESS_FAIL, "DvppCtl failed");
        return ALG_DVPP_CTL_PROCESS_FAIL;
    }
    double vincTemp = resize_out_param.vinc;
    double hincTemp = resize_out_param.hinc;

    double vincSrc = (outHeight * 1.0f / (vmax - vmin + 1));
    double hincSrc = (outWidth * 1.0f / (hmax - hmin + 1));

    if (!CHECK_INC_VALUE(vincSrc, hincSrc)) {
        real_rect_Height = (uint32_t)(vmax - vmin + 1) * vincTemp;
        real_rect_iWidth = (uint32_t)(hmax - hmin + 1) * hincTemp;
    }

    vpcInMsg.vinc = vincTemp;   // todo height resize, resolution need to make sure no more +-1 missing
    vpcInMsg.hinc = hincTemp;   // width resize

    vpcInMsg.width = align_image_w;
    vpcInMsg.high = align_image_h;
    vpcInMsg.hmax = resize_out_param.hmax;
    vpcInMsg.hmin = resize_out_param.hmin;
    vpcInMsg.vmax = resize_out_param.vmax;
    vpcInMsg.vmin = resize_out_param.vmin;

    vpcInMsg.in_buffer_size = vpcInMsg.width * vpcInMsg.high * 3 / 2;
    vpcInMsg.auto_out_buffer_1 = autoOutBuffer;
    // 图片数据地址对齐
    hiai::AlignBuff tmpAlignBuff(vpcInMsg.in_buffer_size);
    tmpAlignBuff.InitAlignBuff();
    char *alignImageAddr = tmpAlignBuff.GetBuffer();

    // 处理宽对齐拷贝, 因上一级出来的就是128宽对齐的图片，因此不需要处理宽对齐拷贝
    AlignImageWith(alignImageAddr,
        (char*)image_buffer,
        vpcInMsg.width,
        vpcInMsg.high,
        vpcInMsg.width,
        iHeight);
    vpcInMsg.in_buffer = alignImageAddr;

    dvppApiCtlMsg.in = (void*)(&vpcInMsg);
    dvppApiCtlMsg.in_size = sizeof(vpc_in_msg);
    if (0 != DvppCtl(pDvppApi, DVPP_CTL_VPC_PROC, &dvppApiCtlMsg)) {
        HIAI_ENGINE_LOG(ALG_DVPP_CTL_PROCESS_FAIL, "DvppCtl failed");
        return ALG_DVPP_CTL_PROCESS_FAIL;
    }

    uint32_t auto_buffer_size = vpcInMsg.auto_out_buffer_1->getBufferSize();
    if (outImageSize != auto_buffer_size) {
        HIAI_ENGINE_LOG(ALG_DVPP_CTL_PROCESS_FAIL,
            "outImageSize != auto_out_buffer_1->getBufferSize()");
        return ALG_DVPP_CTL_PROCESS_FAIL;
    }

    if (real_rect_Height != 0 && auto_buffer_size) {
        uint8_t *image_buffer =
            new uint8_t[auto_buffer_size];
        memcpy_s(image_buffer,
            auto_buffer_size,
            vpcInMsg.auto_out_buffer_1->getBuffer(),
            auto_buffer_size);

        hiai::Rectangle<hiai::Point2D> real_rect;
        real_rect.anchor_lt.x = 0;
        real_rect.anchor_lt.y = 0;
        real_rect.anchor_rb.x = real_rect_iWidth;
        real_rect.anchor_rb.y = real_rect_Height;
        ResizeOnceAgain(pDvppApi, image_buffer,
            autoOutBuffer, outImageSize,
            real_rect, outWidth,
            outHeight);
        delete[] image_buffer;
    }
    return HIAI_OK;
}

/**
* @ingroup hiaiengine
* @brief   CheckDvppParameter    检查DVPP参数
* @param   [in] inImage :        输入图片
* @param   [in] rectangle:       框
* @return  HIAI_StatusT      HIAI_OK
*/
HIAI_StatusT CheckDvppParameter(const hiai::ImageData<uint8_t> & inImage, const hiai::Rectangle<hiai::Point2D> & rectangle)
{
    if ((rectangle.anchor_lt.x < 0)
        || (rectangle.anchor_lt.y < 0)
        || (rectangle.anchor_rb.x < 0)
        || (rectangle.anchor_rb.y < 0)
        || (rectangle.anchor_rb.x <= rectangle.anchor_lt.x)
        || (rectangle.anchor_rb.y <= rectangle.anchor_lt.y)) {
        HIAI_ENGINE_LOG(ALG_DVPP_CROP_PARA_WRONG, "dvpp input rectangle is negtive!!!!");
        return ALG_DVPP_CROP_PARA_WRONG;
    } else if ((inImage.width < uint32_t(rectangle.anchor_rb.x - rectangle.anchor_lt.x + 1))
        || (inImage.height < uint32_t(rectangle.anchor_rb.y - rectangle.anchor_lt.y + 1))) {
        HIAI_ENGINE_LOG(ALG_DVPP_CROP_PARA_WRONG,
            "dvpp input rectangle is bigger than input img size!!!!");
        return ALG_DVPP_CROP_PARA_WRONG;
    }

    return HIAI_OK;
}

/**
* @ingroup hiaiengine
* @brief   CropAndResizeByDvpp   Crop&&Resize
* @param   [in] inImage :        输入图片
* @param   [in] rectangle:       框
* @return  HIAI_StatusT      HIAI_OK
*/
HIAI_StatusT CropAndResizeByDvpp(IDVPPAPI *pDvppApi,
    const hiai::ImageData<uint8_t> & inImage,
    std::shared_ptr<AutoBuffer> autoOutBuffer,
    int32_t &outImageSize ,
    const hiai::Rectangle<hiai::Point2D> & rectangle,
    uint32_t outWidth,
    uint32_t outHeight)
{
    dvppapi_ctl_msg dvppApiCtlMsg;
    vpc_in_msg vpcInMsg;

    if (nullptr ==pDvppApi)
    {
        HIAI_ENGINE_LOG(ALG_DVPP_CROP_PARA_WRONG,
            "CropAndResizeByDvpp pDvppApi = null");
        return ALG_DVPP_CROP_PARA_WRONG;
    }

    HIAI_StatusT ret = CheckDvppParameter(inImage, rectangle);
    if (ret != HIAI_OK) {
        HIAI_ENGINE_LOG("CheckDvppParameter failed!");
        return ret;
    }

    uint32_t iWidth =
        (uint32_t)(rectangle.anchor_rb.x - rectangle.anchor_lt.x);
    uint32_t iHeight =
        (uint32_t)(rectangle.anchor_rb.y - rectangle.anchor_lt.y);
    uint32_t hmin = (uint32_t)((rectangle.anchor_lt.x >> 1) << 1);
    uint32_t vmin = (uint32_t)((rectangle.anchor_lt.y >> 1) << 1);
    uint32_t hmax = (uint32_t)(hmin + (iWidth % 2 ? iWidth : (iWidth - 1)));
    uint32_t vmax = (uint32_t)(vmin + (iHeight % 2 ? iHeight : (iHeight - 1)));

    uint32_t real_rect_Height = 0;
    uint32_t real_rect_iWidth = 0;

    // 对齐原图128*16
    int32_t alignImageW =
        (inImage.width%128) ? ((inImage.width/128) * 128+128) : inImage.width;
    int32_t alignImageH =
        (inImage.height%16) ? ((inImage.height/16) * 16+16) : inImage.height;

    struct resize_param_in_msg resize_in_param;
    resize_in_param.src_width = alignImageW;  // 128对齐
    resize_in_param.src_high =  alignImageH;  // 16对齐
    resize_in_param.hmax = hmax;
    resize_in_param.hmin = hmin;
    resize_in_param.vmax = vmax;
    resize_in_param.vmin = vmin;
    resize_in_param.dest_width = outWidth;
    resize_in_param.dest_high = outHeight;
    struct resize_param_out_msg resize_out_param;
    dvppApiCtlMsg.in  = (void*)(&resize_in_param);
    dvppApiCtlMsg.out = (void*)(&resize_out_param);
    HIAI_ENGINE_LOG("resize_in_param:%d %d %d %d %d %d %d %d %d\n",
        resize_in_param.src_width,
        resize_in_param.src_high,
        resize_in_param.hmax,
        resize_in_param.hmin,
        resize_in_param.vmax,
        resize_in_param.vmin,
        resize_in_param.dest_width,
        resize_in_param.dest_high,
        DVPP_CTL_TOOL_CASE_GET_RESIZE_PARAM);
    if (0 != DvppCtl(pDvppApi,
        DVPP_CTL_TOOL_CASE_GET_RESIZE_PARAM, &dvppApiCtlMsg)) {
        HIAI_ENGINE_LOG(ALG_DVPP_CTL_PROCESS_FAIL, "DvppCtl failed");
        return ALG_DVPP_CTL_PROCESS_FAIL;
    }
    double vincTemp = resize_out_param.vinc;
    double hincTemp = resize_out_param.hinc;
    double vincSrc = (outHeight * 1.0f / (vmax - vmin + 1));
    double hincSrc = (outWidth * 1.0f / (hmax - hmin + 1));
    if (!CHECK_INC_VALUE(vincSrc, hincSrc)) {
        real_rect_Height = (uint32_t)(vmax - vmin + 1) * vincTemp;
        real_rect_iWidth = (uint32_t)(hmax - hmin + 1) * hincTemp;
    }

    vpcInMsg.format = 0;            // nv12
    vpcInMsg.rank = 0;              // nv12
    vpcInMsg.width = alignImageW;
    vpcInMsg.high = alignImageH;  // inImage.height;
    vpcInMsg.hmax = resize_out_param.hmax;
    vpcInMsg.hmin = resize_out_param.hmin;
    vpcInMsg.vmax = resize_out_param.vmax;
    vpcInMsg.vmin = resize_out_param.vmin;
    vpcInMsg.stride = vpcInMsg.width;
    vpcInMsg.vinc = vincTemp;
    vpcInMsg.hinc = hincTemp;

    // alloc in and out buffer
    vpcInMsg.in_buffer_size = vpcInMsg.width * vpcInMsg.high * 3 / 2;
    // 图片数据地址对齐
    char *alignImageAddr = nullptr;

    hiai::AlignBuff tmpAlignBuff(vpcInMsg.in_buffer_size);
    tmpAlignBuff.InitAlignBuff();
    alignImageAddr = tmpAlignBuff.GetBuffer();
    // 处理宽对齐拷贝
    AlignImageWith(alignImageAddr,
        (char*)inImage.data.get(),
        vpcInMsg.width,
        vpcInMsg.high,
        inImage.width,
        inImage.height);
    vpcInMsg.in_buffer = alignImageAddr;

    vpcInMsg.auto_out_buffer_1 = autoOutBuffer;
    dvppApiCtlMsg.in = (void*)(&vpcInMsg);
    dvppApiCtlMsg.in_size = sizeof(vpc_in_msg);
    if (0 != DvppCtl(pDvppApi, DVPP_CTL_VPC_PROC, &dvppApiCtlMsg)) {
        HIAI_ENGINE_LOG(ALG_DVPP_CTL_PROCESS_FAIL, "DvppCtl failed");
        return ALG_DVPP_CTL_PROCESS_FAIL;
    }
    uint32_t auto_buffer_size = vpcInMsg.auto_out_buffer_1->getBufferSize();
    if (outImageSize != auto_buffer_size) {
        return ALG_DVPP_CTL_PROCESS_FAIL;
    }

    // 如果一次resize不满足要求，需要在处理一次
    if (real_rect_Height != 0 && auto_buffer_size) {
        uint8_t *image_buffer =
            new uint8_t[auto_buffer_size];
        memcpy_s(image_buffer,
            auto_buffer_size,
            vpcInMsg.auto_out_buffer_1->getBuffer(),
            auto_buffer_size);

        hiai::Rectangle<hiai::Point2D> real_rect;
        real_rect.anchor_lt.x = 0;
        real_rect.anchor_lt.y = 0;
        real_rect.anchor_rb.x = real_rect_iWidth;
        real_rect.anchor_rb.y = real_rect_Height;
        HIAI_StatusT ret = ResizeOnceAgain(pDvppApi,
            image_buffer, autoOutBuffer, outImageSize,
            real_rect, outWidth, outHeight);
        if (ret != HIAI_OK) {
            HIAI_ENGINE_LOG(ALG_DVPP_CTL_PROCESS_FAIL,
                "ResizeOnceAgain failed");
            delete[] image_buffer;
            return ALG_DVPP_CTL_PROCESS_FAIL;
        }
        delete[] image_buffer;
    }
    HIAI_ENGINE_LOG("DVPP crop and resize success");
    return HIAI_OK;
}

