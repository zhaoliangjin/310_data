/***************************************************************************************
*                      CopyRight (C) Hisilicon Co., Ltd.
*
*       Filename:  user_def_errorcode.h
*    Description:  User defined Error Code
*
*        Version:  1.0
*        Created:  2018-01-08 10:15:18
*         Author:
*
*       Revision:  initial draft;
**************************************************************************************/
#ifndef __INC_SAMPLE_ERRORCODE_H__
#define __INC_SAMPLE_ERRORCODE_H__
#include "hiaiengine/status.h"

#define MODID_SCAENARIO 0x5001
enum
{
    HIAI_CREATE_DVPP_ERROR_CODE = 0,
    HIAI_JPEGD_CTL_ERROR_CODE,
    HIAI_PNGD_CTL_ERROR_CODE,
    HIAI_JPEGE_CTL_ERROR_CODE,
    HIAI_VDEC_CTL_ERROR_CODE,
    HIAI_VENC_CTL_ERROR_CODE,
    HIAI_INVALID_INPUT_MSG_CODE,
    HIAI_AI_MODEL_MANAGER_INIT_FAIL_CODE,
    HIAI_AI_MODEL_MANAGER_PROCESS_FAIL_CODE,
    HIAI_SEND_DATA_FAIL_CODE
};
HIAI_DEF_ERROR_CODE(MODID_SCAENARIO, HIAI_ERROR, HIAI_CREATE_DVPP_ERROR, \
    "create dvpp error");
HIAI_DEF_ERROR_CODE(MODID_SCAENARIO, HIAI_ERROR, HIAI_JPEGD_CTL_ERROR, \
    "jpegd ctl error");
HIAI_DEF_ERROR_CODE(MODID_SCAENARIO, HIAI_ERROR, HIAI_PNGD_CTL_ERROR, \
    "pngd ctl error");
HIAI_DEF_ERROR_CODE(MODID_SCAENARIO, HIAI_ERROR, HIAI_JPEGE_CTL_ERROR, \
    "jpege ctl error");
HIAI_DEF_ERROR_CODE(MODID_SCAENARIO, HIAI_ERROR, HIAI_VDEC_CTL_ERROR, \
    "vdec ctl error");
HIAI_DEF_ERROR_CODE(MODID_SCAENARIO, HIAI_ERROR, HIAI_VENC_CTL_ERROR, \
    "venc ctl error");
HIAI_DEF_ERROR_CODE(MODID_SCAENARIO, HIAI_ERROR, HIAI_INVALID_INPUT_MSG, \
    "invalid input message");
HIAI_DEF_ERROR_CODE(MODID_SCAENARIO, HIAI_ERROR, HIAI_AI_MODEL_MANAGER_INIT_FAIL, \
    "ai model manager init failed");
HIAI_DEF_ERROR_CODE(MODID_SCAENARIO, HIAI_ERROR, HIAI_AI_MODEL_MANAGER_PROCESS_FAIL, \
    "ai model manager process failed");
HIAI_DEF_ERROR_CODE(MODID_SCAENARIO, HIAI_ERROR, HIAI_SEND_DATA_FAIL, \
    "send data failed");
#endif //__INC_SAMPLE_ERRORCODE_H__
