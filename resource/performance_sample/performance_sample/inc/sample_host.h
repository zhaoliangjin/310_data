#ifndef __INC_SAMPLE_HOST_H__
#define __INC_SAMPLE_HOST_H__
#include <hiaiengine/engine.h>
#include <hiaiengine/api.h>
#include <hiaiengine/data_type_reg.h>
// Define Source Engine, Dst Engine, Jpegd Engine, Vpc Engine, AiStubEngine
using hiai::Engine;
using hiai::AIConfig;
using hiai::AIModelDescription;
#define MAX_BUF_LEN  1024 * 1024 * 10
static const int MAX_SLEEP_TIMER = 30 * 60;
// Souce Engine
class SourceEngine : public Engine
{
public:
    HIAI_StatusT Init(const AIConfig &config,
        const std::vector<AIModelDescription>& model_desc);

    HIAI_DEFINE_PROCESS(1, 1);
private:
    std::list<std::string> file_list_;
};

// Dst Engine
class DstEngine :public Engine
{
    HIAI_DEFINE_PROCESS(1, 1);
private:
    std::string dst_file_pat_;
};

// DataRecive 
class DstEngineDataRecvInterface :public hiai::DataRecvInterface
{
public:
    DstEngineDataRecvInterface(const std::string& filename) :
        file_name_(filename)
    {
    }

    HIAI_StatusT RecvData(const std::shared_ptr<void>& message);
private:
    std::string file_name_;
};

// 注册Engine将流转的结构体
typedef struct EngineTrans
{
    std::string trans_buff;
    uint32_t buffer_size;
    HIAI_SERIALIZE(trans_buff, buffer_size)
}EngineTransT;

#endif // __INC_SAMPLE_HOST_H__