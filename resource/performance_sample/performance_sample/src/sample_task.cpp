/**
* @file sample_main.cpp
*
* Copyright (C) <2018>  <Huawei Technologies Co., Ltd.>. All Rights Reserved.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
*/
#include "sample_task.h"

TaskSignal& TaskSignal::GetSingleton()
{
    static TaskSignal instance;
    return instance;
}

void TaskSignal::Block()
{
    std::unique_lock<std::mutex> lk(_m);
    _processed = false;
    _cv.wait(lk, [&]{return _processed;});
}

void TaskSignal::Unblock()
{
    {
        std::lock_guard<std::mutex> lk(_m);
        _processed = true;
        ++_processed_count;
    }
    _cv.notify_one();
}

TaskSignal::TaskSignal() : _processed(false)
                         , _processed_count(0)
{
}

uint32_t TaskSignal::GetProcessedCount() const
{
    return _processed_count;
}
