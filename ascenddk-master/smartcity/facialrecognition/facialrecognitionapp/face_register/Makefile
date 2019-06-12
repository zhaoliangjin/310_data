all : libface_register.so
#DEVICE COMPILER		
CC := aarch64-linux-gnu-g++
#注意每行后面不要有空格，否则会算到目录名里面，导致问题
LOCAL_DIR  := ./

SRC_DIR = $(LOCAL_DIR)
BUILD_DIR = tmp
OUT_DIR = ../out
OBJ_DIR = $(BUILD_DIR)/obj
DEPS_DIR  = $(BUILD_DIR)/deps

#这里添加其他头文件路径
INC_DIR = \
	-I$(SRC_DIR) \
	-I$(DDK_HOME)/include/inc \
	-I$(DDK_HOME)/include/inc/custom \
	-I$(DDK_HOME)/include/third_party/opencv/include \
	-I$(DDK_HOME)/include/third_party/protobuf/include \
	-I$(DDK_HOME)/include/third_party/cereal/include \
	-I$(DDK_HOME)/include/libc_sec/include \
	-I../common/include \
	-I$(HOME)/ascend_ddk/include \

#这里添加编译参数
CC_FLAGS := $(INC_DIR) -g -std=c++11 -fPIC -DCPU_ONLY
LNK_FLAGS := \
	-L$(HOME)/ascend_ddk/device/lib/ -L$(DDK_HOME)/device/lib/ \
	-lDvpp_api \
	-lDvpp_jpeg_decoder \
	-lDvpp_jpeg_encoder \
	-lDvpp_vpc \
	-lmedia_mini \
	-lhiai_server \
	-lidedaemon \
	-lhiai_common \
	-lopencv_world \
	-lascend_ezdvpp \
	-lprotobuf \
	-lpresenteragent \
	-shared


#这里递归遍历3级子目录
DIRS := $(shell find $(SRC_DIR) -maxdepth 3 -type d)
CUSTOM_DIRS := $(shell find $(SRC_DIR) -maxdepth 3 -type d)

#将每个子目录添加到搜索路径
VPATH = $(DIRS)

#查找src_dir下面包含子目录的所有cpp文件
SOURCES  = $(foreach dir, $(DIRS), $(wildcard $(dir)/*.cpp))
CUSTOM_SOURCES  = $(foreach dir, $(CUSTOM_DIRS), $(wildcard $(dir)/*.cpp))
OBJS   = $(addprefix $(OBJ_DIR)/,$(patsubst %.cpp,%.o,$(notdir $(SOURCES))))
OBJS_customop = $(addprefix $(OBJ_DIR)/,$(patsubst %.cpp,%.o,$(notdir $(CUSTOM_SOURCES))))
OBJS_no_customop := $(filter-out $(OBJS_customop), $(OBJS))
DEPS  = $(addprefix $(DEPS_DIR)/, $(patsubst %.cpp,%.d,$(notdir $(SOURCES))))

SOURCES_cc  = $(foreach dir, $(DIRS), $(wildcard $(dir)/*.cc))
CUSTOM_SOURCES_cc = $(foreach dir, $(CUSTOM_DIRS), $(wildcard $(dir)/*.cc))
OBJS_customop_cc = $(addprefix $(OBJ_DIR)/,$(patsubst %.cc,%.o,$(notdir $(CUSTOM_SOURCES_cc))))
DEPS_cc  = $(addprefix $(DEPS_DIR)/, $(patsubst %.cc,%.d,$(notdir $(SOURCES_cc))))

# 编译源码链接成目标so

libface_register.so: $(OBJS_customop) $(OBJS_customop_cc)
	$(CC) $^ $(LNK_FLAGS) -o $@
	rm -rf $(BUILD_DIR)

#编译之前要创建OBJ目录，确保目录存在
$(OBJ_DIR)/%.o:%.cpp
	@if [ ! -d $(OBJ_DIR) ]; then mkdir -p $(OBJ_DIR); fi;
	$(CC) -c $(CC_FLAGS) -o $@ $<


$(OBJ_DIR)/%.o:%.cc
	@if [ ! -d $(OBJ_DIR) ]; then mkdir -p $(OBJ_DIR); fi;
	$(CC) -c $(CC_FLAGS) -o $@ $<

#编译之前要创建DEPS目录，确保目录存在
#前面加@表示隐藏命令的执行打印
$(DEPS_DIR)/%.d:%.cpp
	@if [ ! -d $(DEPS_DIR) ]; then mkdir -p $(DEPS_DIR); fi;
	set -e; rm -f $@;
	$(CC) -MM $(CC_FLAGS) $< > $@.$$$$;
	sed 's,\($*\)\.o[ :]*,$(OBJ_DIR)/\1.o $@ : ,g' < $@.$$$$ > $@;
	rm -f $@.$$$$

#前面加-表示忽略错误
ifneq ($(MAKECMDGOALS), clean)
	-include $(DEPS)
endif

.PHONY : clean install
clean:
	rm -rf $(BUILD_DIR) lib*.so *.o
install: libface_register.so
	mv *.so $(OUT_DIR)
