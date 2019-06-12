LOCAL_DIR      := $(patsubst %,%,$(CURDIR))

# set DDK_HOME default value
ifndef DDK_HOME
$(error "Can not find DDK_HOME env, please set it in environment!.")
endif

CC := aarch64-linux-gnu-g++
CPP := aarch64-linux-gnu-g++

LOCAL_MODULE_NAME := ascendcamera
CC_FLAGS := -std=c++11 -Wall

local_src_files := \
	$(LOCAL_DIR)/src/ascenddk/ascendcamera/main.cpp \
	$(LOCAL_DIR)/src/ascenddk/ascendcamera/camera.cpp \
	$(LOCAL_DIR)/src/ascenddk/ascendcamera/output_info_process.cpp \
	$(LOCAL_DIR)/src/ascenddk/ascendcamera/main_process.cpp \
	$(LOCAL_DIR)/src/ascenddk/ascendcamera/parameter_utils.cpp \
	$(LOCAL_DIR)/src/ascenddk/ascendcamera/ascend_camera_parameter.cpp

local_inc_dirs := \
	$(LOCAL_DIR)/include \
	$(DDK_HOME)/include/inc \
	$(DDK_HOME)/include/third_party/protobuf/include \
	$(DDK_HOME)/include/third_party/cereal/include \
	$(DDK_HOME)/include/libc_sec/include \
	$(DDK_HOME)/include/inc/custom \
	$(HOME)/ascend_ddk/include

local_shared_libs := \
	c_sec \
	pthread \
	protobuf \
	slog \
	media_mini \
	Dvpp_api \
	Dvpp_jpeg_decoder \
	Dvpp_jpeg_encoder \
	Dvpp_vpc \
	OMX_hisi_video_decoder \
	OMX_hisi_video_encoder \
	Dvpp_png_decoder \
	OMX_Core \
	OMX_hisi_vdec_core \
	OMX_Core_VENC \
	OMX_common \
	ascend_ezdvpp \
	presenteragent

#Q := @
FULL_SRC_FILES        := $(local_src_files)
FULL_INC_DIRS         := $(foreach inc_dir, $(local_inc_dirs), -I$(inc_dir))
SHARED_LIBRARIES      := $(foreach shared_lib, $(local_shared_libs), -l$(shared_lib))

LOCAL_OBJ_PATH        := $(LOCAL_DIR)/out
LOCAL_LIBRARY         := $(LOCAL_OBJ_PATH)/$(LOCAL_MODULE_NAME)
FULL_CPP_SRCS         := $(filter %.cpp,$(FULL_SRC_FILES))
FULL_CPP_OBJS         := $(patsubst $(LOCAL_DIR)/%.cpp,$(LOCAL_OBJ_PATH)/%.o, $(FULL_CPP_SRCS))

LNK_FLAGS := \
	-Wl,-rpath-link=$(DDK_HOME)/device/lib/ \
	-L$(DDK_HOME)/device/lib \
	-L$(HOME)/ascend_ddk/device/lib \
	$(SHARED_LIBRARIES)

all: do_pre_build do_build

do_pre_build:
	$(Q)echo - do [$@]
	$(Q)mkdir -p $(LOCAL_OBJ_PATH)

do_build: $(LOCAL_LIBRARY) | do_pre_build
	$(Q)echo - do [$@]

$(LOCAL_LIBRARY): $(FULL_CPP_OBJS) | do_pre_build
	$(Q)echo [LD] $@
	$(Q)$(CPP) $(CC_FLAGS) -o $(LOCAL_LIBRARY) $(FULL_CPP_OBJS)   -Wl,--whole-archive  -Wl,--no-whole-archive -Wl,--start-group  -Wl,--end-group $(LNK_FLAGS)

$(FULL_CPP_OBJS): $(LOCAL_OBJ_PATH)/%.o : $(LOCAL_DIR)/%.cpp  | do_pre_build
	$(Q)echo [CC] $@
	$(Q)mkdir -p $(dir $@)
	$(Q)$(CPP) $(CC_FLAGS) $(FULL_INC_DIRS) -c  -fstack-protector-all $< -o $@

clean:
	rm -rf $(LOCAL_DIR)/out
