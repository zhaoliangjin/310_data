/**
 * ============================================================================
 *
 * Copyright (C) 2018, Hisilicon Technologies Co., Ltd. All Rights Reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 *   1 Redistributions of source code must retain the above copyright notice,
 *     this list of conditions and the following disclaimer.
 *
 *   2 Redistributions in binary form must reproduce the above copyright notice,
 *     this list of conditions and the following disclaimer in the documentation
 *     and/or other materials provided with the distribution.
 *
 *   3 Neither the names of the copyright holders nor the names of the
 *   contributors may be used to endorse or promote products derived from this
 *   software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 * ============================================================================
 */

#ifndef ASCENDDK_ASCENDCAMERA_THREAD_SAFE_QUEUE_H_
#define ASCENDDK_ASCENDCAMERA_THREAD_SAFE_QUEUE_H_

#include <condition_variable>
#include <memory>
#include <mutex>
#include <queue>

template<typename T>
class ThreadSafeQueue {
 private:
  mutable std::mutex mut_;
  std::queue<T> data_queue_;
  std::condition_variable data_cond_;

 public:
  ThreadSafeQueue() {
  }
  ThreadSafeQueue(ThreadSafeQueue const& other) {
    std::lock_guard < std::mutex > lk(other.mut_);
    data_queue_ = other.data_queue_;
  }

  /**
   * @brief push a element to queue
   * @param [out] T new_value: a element
   */
  void Push(T new_value) {
    std::lock_guard<std::mutex> lk(mut_);
    data_queue_.push(new_value);
    data_cond_.notify_one();
  }

  /**
   * @brief if the queue is empty ,it will block the caller thread.
   *        if the queue is not empty,it will pop a element from queue.
   * @param [out] T& value: a element
   */
  void WaitAndPop(T& value) {
    std::unique_lock<std::mutex> lk(mut_);
    data_cond_.wait(lk, [this] {return !data_queue_.empty();});
    value = data_queue_.front();
    data_queue_.pop();
  }

  /**
   * @brief if the queue is empty ,it will block the caller thread.
   *        if the queue is not empty,it will pop a element from queue.
   * @return a shared_ptr point to a element
   */
  std::shared_ptr<T> WaitAndPop() {
    std::unique_lock<std::mutex> lk(mut_);
    data_cond_.wait(lk, [this] {return !data_queue_.empty();});
    std::shared_ptr<T> res(std::make_shared<T>(data_queue_.front()));
    data_queue_.pop();
    return res;
  }

  /**
   * @brief if the queue is empty ,it will return false.
   *        if the queue is not empty,it will pop a element from queue.
   * @param [out] T& value: a element from queue
   * @return true:success to pop a element from queue.
   *         false:the queue is empty.
   */
  bool TryPop(T& value) {
    std::lock_guard<std::mutex> lk(mut_);
    if (data_queue_.empty()) {
      return false;
    }
    value = data_queue_.front();
    data_queue_.pop();
    return true;
  }

  /**
   * @brief if the queue is empty ,it will return a null shared_ptr.
   *        if the queue is not empty,it will pop a element from queue.
   * @return std::shared_ptr<T> a element from the queue
   */
  std::shared_ptr<T> TryPop() {
    std::lock_guard<std::mutex> lk(mut_);
    if (data_queue_.empty()) {
      return std::shared_ptr<T>();
    }
    std::shared_ptr<T> res(std::make_shared<T>(data_queue_.front()));
    data_queue_.pop();
    return res;
  }

  /**
   * @brief if the queue is empty ,it will return true.
   * @return true:the queue is empty.
   */
  bool Empty() const {
    std::lock_guard<std::mutex> lk(mut_);
    return data_queue_.empty();
  }

  /**
   * @brief get size of the queue.
   * @return size of queue.
   */
  int Size() const {
    std::lock_guard<std::mutex> lk(mut_);
    return data_queue_.size();
  }
};

#endif /* ASCENDDK_ASCENDCAMERA_THREAD_SAFE_QUEUE_H_ */
