TOPDIR      := $(patsubst %,%,$(CURDIR))

ifndef DDK_HOME
$(error "Can not find DDK_HOME env, please set it in environment!.")
endif

LOCAL_MODULE_NAME := libascend_ezdvpp.so

ifeq ($(mode),)
mode=AtlasDK
endif

ifeq ($(mode), AtlasDK)
CC := aarch64-linux-gnu-g++
else ifeq ($(mode), ASIC)
CC := $(DDK_HOME)/uihost/toolchains/aarch64-linux-gcc6.3/bin/aarch64-linux-gnu-g++
else
$(error "Unsupported mode: "$(mode)", please input: AtlasDK or ASIC.")
endif

LOCAL_DIR  := .
OUT_DIR = out
OBJ_DIR = $(OUT_DIR)/obj
DEPS_DIR  = $(OUT_DIR)/deps
LOCAL_LIBRARY=$(OUT_DIR)/$(LOCAL_MODULE_NAME)
OUT_INC_DIR = $(OUT_DIR)/include

INC_DIR = \
	-I$(LOCAL_DIR)/include \
	-I$(DDK_HOME)/include/inc \
	-I$(DDK_HOME)/include/inc/custom \
	-I$(DDK_HOME)/include/third_party/protobuf/include \
	-I$(DDK_HOME)/include/third_party/cereal/include \
	-I$(DDK_HOME)/include/libc_sec/include \
	

CC_FLAGS := $(INC_DIR) -std=c++11 -fPIC -Wall -O2
LNK_FLAGS := \
	-Wl,-rpath-link=$(DDK_HOME)/device/lib/ \
	-L$(DDK_HOME)/device/lib/ \
	-lhiai_common \
	-lDvpp_api \
	-shared

SRCS := $(patsubst $(LOCAL_DIR)/%.cpp, %.cpp, $(shell find $(LOCAL_DIR) -name "*.cpp"))
OBJS := $(addprefix $(OBJ_DIR)/, $(patsubst %.cpp, %.o,$(SRCS)))

ALL_OBJS := $(OBJS)

all: do_pre_build do_build

do_pre_build:
	$(Q)echo - do [$@]
	$(Q)mkdir -p $(OBJ_DIR)
	$(Q)mkdir -p $(OUT_INC_DIR)

do_build: $(LOCAL_LIBRARY) | do_pre_build
	$(Q)echo - do [$@]

$(LOCAL_LIBRARY): $(ALL_OBJS)
	$(Q)echo [LD] $@
	$(Q)$(CC) $(CC_FLAGS) -o $@ $^ -Wl,--whole-archive -Wl,--no-whole-archive -Wl,--start-group -Wl,--end-group $(LNK_FLAGS)
	$(Q)cp -R $(TOPDIR)/include/* $(OUT_INC_DIR)

$(OBJS): $(OBJ_DIR)/%.o : %.cpp | do_pre_build
	$(Q)echo [CC] $@
	$(Q)mkdir -p $(dir $@)
	$(Q)$(CC) $(CC_FLAGS) $(INC_DIR) -c -fstack-protector-all $< -o $@

install: all
	$(Q)echo [INSTALL] $@
	$(Q)mkdir -p $(HOME)/ascend_ddk/include
	$(Q)mkdir -p $(HOME)/ascend_ddk/device/lib
	$(Q)cp -R $(OUT_INC_DIR)/* $(HOME)/ascend_ddk/include/
	$(Q)cp -R $(OUT_DIR)/lib*.so $(HOME)/ascend_ddk/device/lib/

clean:
	rm -rf $(TOPDIR)/out
