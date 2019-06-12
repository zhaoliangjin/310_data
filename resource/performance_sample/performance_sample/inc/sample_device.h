#ifndef __INC_SAMPLE_DEVICE_H__
#define __INC_SAMPLE_DEVICE_H__
#include <hiaiengine/engine.h>
#include <hiaiengine/api.h>
#include "hiaiengine/ai_model_manager.h"
#include <dvpp/idvppapi.h>

// Define Source Engine, Dst Engine, Jpegd Engine, Vpc Engine, AiStubEngine
using hiai::Engine;
using ReleaseFunction = std::function<void(void* arg)>;

// Jpegd Engine
class JpegdEngine :public Engine
{
    HIAI_DEFINE_PROCESS(1, 1);
public:
    void JpegdFreeBuffer(void* ptr);
private:
    struct jpegd_yuv_data_info jpegdOutData;     
};

// Vpc Engine
class VpcEngine :public Engine
{
    HIAI_DEFINE_PROCESS(1, 1);
private:
    std::shared_ptr<AutoBuffer> outBuffer_;    
};

// AIStubEngine
class AIStubEngine :public Engine
{
public:
    HIAI_StatusT Init(const hiai::AIConfig &config,
        const std::vector<hiai::AIModelDescription>& model_desc);
    HIAI_DEFINE_PROCESS(1, 1);
    ~AIStubEngine();

private:
    std::vector<std::shared_ptr<hiai::IAITensor>> outputTensorBuffer_;
    std::map<std::string, std::string> _config;
    std::shared_ptr<hiai::AIModelManager> ai_model_manager_;
};

// 注册Engine将流转的结构体
typedef struct EngineTrans
{
    std::string trans_buff;
    uint32_t buffer_size;
    HIAI_SERIALIZE(trans_buff, buffer_size)
}EngineTransT;

static uint32_t MODEL_WIDTH  = 224;
#endif // __INC_SAMPLE_DEVICE_H__