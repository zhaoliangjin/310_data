/**
* @file sample_main.cpp
*
* Copyright (C) <2018>  <Huawei Technologies Co., Ltd.>. All Rights Reserved.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
*/
#include <fstream>
#include <iostream>
#include <thread>
#include <unistd.h>
#include <vector>
#include "sample_host.h"
#include "sample_errorcode.h"
#include "sample_data.h"
#include "common.h"
#include "hiaiengine/ai_memory.h"
#include "hiaiengine/data_type.h"

// Graph and Engine ID
static const uint32_t gGraphId = 1001;
static const uint32_t gSourceEngineId = 101;
static const uint32_t gDstEngineId = 102;

// 输出文件路径
static const std::string gTestDestFileName = "./hiaiengine/result.t";
// 传输消息类型

// 锁与条件变量
std::mutex gDataMutex;
std::condition_variable gDataConv;
// 结果变量，判断是否收到结果
static bool gDataReady = false;

/**
* @ingroup hiaiengine
* @brief   ReleaseDataBuffer    智能指针释放函数，因为框架负责释放，所以这里无需进行释放
* @param   [in] ptr:            传入的智能指针
* @return  void
*/
void ReleaseDataBuffer(void* ptr)
{
    // do nothing
}

/**
* @ingroup hiaiengine
* @brief   RecvData，            接收处理结果毁掉
* @param   [in]message:          处理结果
* @return  bool                  文件存在正确与否  
*/
static bool FileExist(const std::string& file_name)
{
    std::ifstream f(file_name.c_str());
    return f.good();
}

/**
* @ingroup hiaiengine
* @brief   checkDestFileExist    检查输出结果是否存在
* @return  void
*/
void checkDestFileExist()
{
    uint32_t j = 0;
    uint32_t i = 0;

    while (!FileExist(gTestDestFileName) && j < MAX_LOOP_TIMES) {
        sleep(10);
        j++;
    }
    if (j == MAX_LOOP_TIMES) {
        HIAI_ENGINE_LOG("Wait result failed accord to exceed to max times");
    }

    // 打开结果文件句柄
    FILE *fp = fopen("./hiaiengine/result.t","rb");

    if (fp != nullptr) {
        // 获取结果
        std::vector<float> vecProc;
        vecProc.clear();

        float prop = 0.0;
        while (feof(fp) == 0) {
            fread(&prop,sizeof(float),1,fp);
            if (prop > 0.9) { // 将置信度大于0.9的push back进prop vector;
            
                vecProc.push_back(i);
            }
            i++;
        }
        fclose(fp);

        if (vecProc.empty()) {
            HIAI_ENGINE_LOG("Inference result is wrong, ");
        }

        if (vecProc[0] == RIGHT_RESULT) {
            HIAI_ENGINE_LOG("Inference result is aright");
        } else {
            HIAI_ENGINE_LOG("Inference result is wrong, has result but result value is not %u", RIGHT_RESULT );
        }        
    }


    std::unique_lock <std::mutex> lck(gDataMutex);
    gDataReady = true;
    gDataConv.notify_all();
}

/**
* @ingroup hiaiengine
* @brief RecvData，              接收处理结果毁掉
* @param [in]message:            处理结果
* @return HIAI_StatusT错误码，    HIAI_OK为正确
*/
HIAI_StatusT DstEngineDataRecvInterface::RecvData(const std::shared_ptr<void>& message)
{
    std::shared_ptr<std::string> data =
        std::static_pointer_cast<std::string>(message);
    if (nullptr == data) {
        HIAI_ENGINE_LOG("Fail to receive data");
        return HIAI_INVALID_INPUT_MSG;
    }
    // 将处理结果保存文件
    std::ofstream  file_stream(file_name_, std::ios::app);
    if (file_stream.is_open()) {
        file_stream << *data;
    }
    file_stream.close();
    return HIAI_OK;
}

/**
* @ingroup hiaiengine
* @brief SourceEngine::Init      SourceEngine初始化函数
* @param [in]config:             Config配置
* @param [in]model_desc:         模型说明
* @return HIAI_StatusT错误码，    HIAI_OK为正确
*/
HIAI_StatusT SourceEngine::Init(const AIConfig &config,
    const std::vector<AIModelDescription>& model_desc)
{
    return HIAI_OK;
}
 
/**
* @ingroup hiaiengine
* @brief SourceEngine::Process   SourceEngine处理函数，主要负责根据主程序发送来的文件路径读取数据并发送
* @param [in] arg0:              端口数据，根据端口数来决定参数数量
* @return HIAI_StatusT错误码，    HIAI_OK为正确
*/
HIAI_IMPL_ENGINE_PROCESS("SourceEngine", SourceEngine, 1)
{
    HIAI_StatusT ret = HIAI_OK;
    if (nullptr == arg0) {
        return HIAI_OK;
    }

    std::shared_ptr<std::string> input_arg 
        = std::static_pointer_cast<std::string>(arg0);

    // 获取
    std::string filePath = *input_arg;
    // 去读测试文件
    FILE *fpIn = fopen(filePath.data(), "rb");
    if ( NULL == fpIn ) {
        printf("can not open file dvpp_jpeg_decode_001\n");
        return -1;
    }   
    fseek(fpIn, 0, SEEK_END);
    uint32_t fileLen = ftell(fpIn);
    fseek(fpIn, 0, SEEK_SET);

    // 使用DMalloc分配内存
    unsigned char* inbuf = nullptr;
    bool memoryMalloc = true;
    uint32_t bufferLen = fileLen + 8;  // 该内存会直接给到DVPP Jpeg使用，Jpeg需要+8
    HIAI_StatusT getRet = hiai::HIAIMemory::HIAI_DMalloc(bufferLen, (void*&)inbuf, 10000);

    // 如果DMalloc无法获取内存，则通过malloc方式分配
    if (HIAI_OK != getRet || nullptr == inbuf) {
        inbuf = (unsigned char*)malloc(bufferLen);
        memoryMalloc= false;
        if (nullptr == inbuf) {
            HIAI_ENGINE_LOG("Can't alloc buffer");
            fclose( fpIn);
            return -1;
        }
    }
    // 获取图片或关闭文件句柄
    fread( inbuf, 1, fileLen, fpIn );
    fclose( fpIn );
    // 组织发送数据，讲文件buffer填入发送数据结构体
    std::shared_ptr<EngineTransNewT> output =
        std::make_shared<EngineTransNewT>();
    output->trans_buff.reset(inbuf, ReleaseDataBuffer);
    output->buffer_size = bufferLen;
    // 发送数据到制定Engine的端口
    ret = SendData(0, gMsgType, output);
    if (HIAI_OK != ret) {
        HIAI_ENGINE_LOG(ret, "SourceEngine send data error");
    }

    // 如果使用malloc进行内存分配，则需要调用free释放
    if (false == memoryMalloc) {
        free(inbuf);
        inbuf = nullptr;
    }
    return HIAI_OK;
}

/**
* @ingroup hiaiengine
* @brief SourceEngine::Process   DstEngine处理函数，主要负责接收处理结果并下发保存成文件
* @param [in] arg0:              端口数据，根据端口数来决定参数数量
* @return HIAI_StatusT错误码，    HIAI_OK为正确
*/
HIAI_IMPL_ENGINE_PROCESS("DstEngine", DstEngine, 1)
{
    HIAI_StatusT ret = HIAI_OK;
    if (nullptr != arg0) {
        std::shared_ptr<std::string> result =
            std::static_pointer_cast<std::string>(arg0);
        ret = SendData(0, "string", result);
        if (HIAI_OK != ret) {
            HIAI_ENGINE_LOG(ret, "DstEngine SendData to recv failed");
        }
    }
    return HIAI_OK;
}


/**
* @ingroup hiaiengine
* @brief   main                  主函数
* @param   [in] argc:            主函数输入
* @return  返回0/-1, 正确为0，错误为-1
*/
int32_t main(int argc, char **argv)
{
    // Init HIAI
    HIAI_Init(0);
    // Create Graph
    HIAI_StatusT ret =  hiai::Graph::CreateGraph("./hiaiengine/sample.config");
    if (HIAI_OK != ret) {
        HIAI_ENGINE_LOG(ret, "CreateGraph failed");
        hiai::Graph::DestroyGraph(1001);
        return -1;
    }

    std::shared_ptr<hiai::Graph> graph = hiai::Graph::GetInstance(gGraphId);
    // 设置Dst Engine的读取数据回调
    hiai::EnginePortID dstEnginePort;
    dstEnginePort.graph_id = gGraphId;
    dstEnginePort.engine_id = gDstEngineId;
    dstEnginePort.port_id = 0;

    // 设置接受回调函数
    graph->SetDataRecvFunctor(dstEnginePort,
        std::shared_ptr<DstEngineDataRecvInterface>(
        new DstEngineDataRecvInterface(gTestDestFileName)));

    // Send Data, 将数据输入到首个Engine
    hiai::EnginePortID enginePortId;
    enginePortId.graph_id = gGraphId;
    enginePortId.engine_id = gSourceEngineId;
    enginePortId.port_id = 0;

    std::shared_ptr<std::string> srcData(new std::string("./hiaiengine/test.jpg"));
    // 发送数据路径给到source节点；
    graph->SendData(enginePortId,
        "string", std::static_pointer_cast<void>(srcData));

    // 添加阻塞，等待处理结果
    std::thread check_thread(checkDestFileExist);
    check_thread.join();
    std::unique_lock <std::mutex> lck(gDataMutex);
    gDataConv.wait_for(lck,
        std::chrono::seconds(MAX_SLEEP_TIMER),
        [] { return gDataReady; });
    hiai::Graph::DestroyGraph(1001);
}