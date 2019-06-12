TOPDIR      := $(patsubst %,%,$(CURDIR))
CC := aarch64-linux-gnu-g++
CPP := aarch64-linux-gnu-g++


LOCAL_MODULE_NAME := main
CC_FLAGS := -std=c++11

local_src_files := \
	$(TOPDIR)/main.cpp 
	
local_inc_dirs := \
	$(TOPDIR) \
	$(DDK_HOME)/include/inc \
	$(DDK_HOME)/include/third_party/protobuf/include \
	$(DDK_HOME)/include/third_party/cereal/include \
	$(DDK_HOME)/include/libc_sec/include

local_shared_libs_dirs := \
	$(DDK_HOME)/host/lib/ \
	
local_shared_libs := \
	hiai_server \
	hiai_common \
	matrixdaemon \
	ome \
	fmk_common \
	drvdevdrv \
	drvaicpu \
	drvhdc \
	drvhdc_host \
	drvtcp \
	mmpa \
	crypto \
	c_sec \
	runtime \
	devmm \
	teec \
	pthread \
	cce \
	cce_aicpudev \
	cce_aicore \
	aicpu_engine  \
	protobuf \
	rt  \
	profilerclient \
	slog \
	dl \
	media_mini

Q := @
		
FULL_SRC_FILES        := $(local_src_files)
FULL_INC_DIRS         := $(foreach inc_dir, $(local_inc_dirs), -I$(inc_dir))
SHARED_LIBRARIES      := $(foreach shared_lib, $(local_shared_libs), -l$(shared_lib))
SHARED_LIBRARIES_DIRS := $(foreach shared_lib_dir, $(local_shared_libs_dirs), -L$(shared_lib_dir))

LOCAL_OBJ_PATH        := $(TOPDIR)/out
LOCAL_LIBRARY         := $(LOCAL_OBJ_PATH)/$(LOCAL_MODULE_NAME)
FULL_C_SRCS           := $(filter %.c,$(FULL_SRC_FILES))
FULL_C_OBJS           := $(patsubst $(TOPDIR)/%.c,$(LOCAL_OBJ_PATH)/%.o, $(FULL_C_SRCS))
FULL_CPP_SRCS         := $(filter %.cpp,$(FULL_SRC_FILES))
FULL_CPP_OBJS         := $(patsubst $(TOPDIR)/%.cpp,$(LOCAL_OBJ_PATH)/%.o, $(FULL_CPP_SRCS))

all: do_pre_build do_build

do_pre_build:
	$(Q)echo - do [$@]
	$(Q)mkdir -p $(LOCAL_OBJ_PATH)
		
do_build: $(LOCAL_LIBRARY) | do_pre_build
	$(Q)echo - do [$@]

$(LOCAL_LIBRARY): $(FULL_C_OBJS) $(FULL_CPP_OBJS) | do_pre_build
	$(Q)echo [LD] $@
	$(Q)$(CPP) $(CC_FLAGS) -o $(LOCAL_LIBRARY) $(FULL_C_OBJS) $(FULL_CPP_OBJS) -Wl,--whole-archive  -Wl,--no-whole-archive -Wl,--start-group  -Wl,--end-group $(SHARED_LIBRARIES_DIRS) $(SHARED_LIBRARIES) -Wl,-rpath-link=$(DDK_HOME)/host/lib

$(FULL_C_OBJS): $(LOCAL_OBJ_PATH)/%.o : $(TOPDIR)/%.c $(TOPDIR)/%.h  | do_pre_build
	$(Q)echo [CC] $@
	$(Q)mkdir -p $(dir $@)
	$(Q)$(CPP) $(CC_FLAGS)  $(FULL_INC_DIRS) -c $< -o $@

$(FULL_CPP_OBJS): $(LOCAL_OBJ_PATH)/%.o : $(TOPDIR)/%.cpp $(TOPDIR)/%.h  | do_pre_build
	$(Q)echo [CC] $@
	$(Q)mkdir -p $(dir $@)
	$(Q)$(CPP) $(CC_FLAGS) $(FULL_INC_DIRS) -c $< -o $@
