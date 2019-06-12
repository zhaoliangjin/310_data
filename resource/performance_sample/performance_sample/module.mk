LOCAL_PATH := $(call my-dir)
FILE_PATH := $(LOCAL_PATH)/../..

PRODUCT = MINI

# Ingeration test cases.
include $(CLEAR_VARS)
LOCAL_C_INCLUDES := \
    $(TOPDIR)third_party/protobuf/include \
    $(TOPDIR)third_party/cereal/include \
    $(TOPDIR)framework/domi \
    $(TOPDIR)libc_sec/include \
    $(TOPDIR)inc \
    proto/ai_types.proto \
    $(LOCAL_PATH)/inc \

LOCAL_MODULE := sample_demo

LOCAL_SRC_FILES :=  \
        src/sample_main.cpp \
        src/sample_data.cpp \
        src/sample_task.cpp \

LOCAL_LDFLAGS := -lrt -g

LOCAL_SHARED_LIBRARIES := libmatrix libprotobuf libdrvhdc_host  libcrypto

include $(BUILD_HOST_EXECUTABLE)

include $(CLEAR_VARS)

LOCAL_MODULE := sample_device
LOCAL_C_INCLUDES := \
    $(TOPDIR)third_party/protobuf/include \
    $(TOPDIR)third_party/cereal/include \
    $(TOPDIR)framework/domi \
    $(TOPDIR)libc_sec/include \
    $(TOPDIR)inc \
    proto/ai_types.proto \
    $(LOCAL_PATH)/inc \

LOCAL_SRC_FILES :=  \
        src/sample_dvpp.cpp \
        src/sample_device.cpp \
        src/sample_data.cpp \

LOCAL_LDFLAGS := -lrt

LOCAL_SHARED_LIBRARIES := libdrvdevdrv libhiai_common \
    libhiai_client libhiai_server	\
    libdrvhdc libmatrixdaemon libmmpa \
    libome libcrypto libfmk_common libslog  \
    libprotobuf libc_sec libcce libcce_aicore \
    libruntime libcce_aicpudev \
    libDvpp_api  libDvpp_jpeg_decoder \
    libDvpp_jpeg_encoder libDvpp_png_decoder \
    libDvpp_vpc libOMX_common libOMX_hisi_vdec_core \
    libOMX_hisi_video_decoder libOMX_hisi_video_encoder \
    libOMX_Core libOMX_Core_VENC

include $(BUILD_SHARED_LIBRARY)