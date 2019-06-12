#ifndef __INC_SAMPLE_TASK_H__
#define __INC_SAMPLE_TASK_H__

#include <mutex>
#include <condition_variable>

class TaskSignal
{
public:
    static TaskSignal& GetSingleton();
    void Block();
    void Unblock();
    uint32_t GetProcessedCount() const;

private:
    TaskSignal();

    bool _processed;
    uint32_t _processed_count;
    std::condition_variable _cv;
    std::mutex _m;
};

#endif  // __INC_SAMPLE_TASK_H__
