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

#ifndef ASCENDDK_ASCENDCAMERA_MAIN_PROCESS_H_
#define ASCENDDK_ASCENDCAMERA_MAIN_PROCESS_H_

#include <pthread.h>
#include <atomic>

#include "securec.h"

#include "ascenddk/ascend_ezdvpp/dvpp_process.h"
#include "ascenddk/ascendcamera/camera.h"
#include "ascenddk/ascendcamera/output_info_process.h"
#include "ascenddk/ascendcamera/main_process.h"
#include "ascenddk/ascendcamera/ascend_camera_parameter.h"
#include "ascenddk/ascendcamera/thread_safe_queue.h"

namespace ascend {
namespace ascendcamera {

enum MainProcessErrorCode {
  kMainProcessOk,
  kMainProcessMallocFail,
  kMainProcessInvalidParameter,
  kMainProcessMultiframeInvalidParameter,
  kMainProcessMultiframeDvppProcError,
  kMainProcessMultiframeOutputError,
  kMainProcessMultiframeMallocFail,
  kMainProcessMultiframeCpyFail,
};

enum LoopFlag {
  kNoNeedLoop,
  kNeedLoop,
};

#define CHECK_MEMCPY_S_RESULT(ret) \
if(ret != EOK){ \
  ASC_LOG_ERROR("Failed to copy memory."); \
  return kMainProcessMultiframeCpyFail; \
}

//used for thread control flag
const int kThreadNoStartup = 0;
const int kThreadStartup = 1;
const int kThreadStop = 2;

//used for thread status
const int kThreadInvalidStatus = 0;
const int kThreadRunStatus = 1;
const int kThreadErrorStatus = 2;

// fps >10 and in video
const int kStartupThreadThresHold = 10;

const int kH264BufferQueueMaxLength = 10;

const int kH264BufferMaxFrame = 10;

// maximum size of yuv file
const int kYuvImageMaxSize = (1920 * 1080 * 3 / 2);

// maximum number of retrying to push queue.
const int kMaxRetryNum = 20;

// thread need exit
const int kThreadNeedExit = 1;

const int kThreadNoNeedExit = 0;

// time of waitting push element
const int kWaitTime = (100 * 1000);

// 1 second = 1000 millsecond
const int kSecToMillisec = 1000;

// 1 millisecond = 1000000 nanosecond
const int kMillSecToNanoSec = 1000000;

const int kDebugRecordFrameThresHold = 100;

const int kParaCheckError = -1;

// the quality of yuv convert to jpg
const int kDvppToJpgQualityParameter = 100;

const char KPortChannelSeparator = '/';
const char kPortIpSeparator = ':';

const int kFirstIndex = 0;

struct H264Buf {
  // h264 data buffer
  char *buf;

  // size of one h264 frame
  long single_frame_size;

  // frame number
  int index;

  // whether data package is last
  int exit_flag;
};

struct MultiFrameProc {
  // thread run flag
  std::atomic_int open_thread_flag;

  // thread safe queue
  ThreadSafeQueue<H264Buf *> *safe_queue;

  // current frame buffer
  H264Buf *current_buf;

  // thread id
  pthread_t thread_id;

  // thread status
  std::atomic_int thread_status;

  // thread ErrorCode
  std::atomic_int thread_error_code;
};

struct ControlObject {
  // used for user's input parameter
  ascend::ascendcamera::AscendCameraParameter *ascend_camera_paramter;

  // used for camera
  Camera *camera;

  // dvpp process
  ascend::utils::DvppProcess *dvpp_process;

  // output process
  OutputInfoProcess *output_process;

  // whether need loop
  LoopFlag loop_flag;

  struct MultiFrameProc multi_frame_process;
};

typedef struct MainProcessDebugInfo {
  // frame number
  long total_frame;
  // the maximum length of queue
  int queue_max_length;
} DebugInfo;

class MainProcess {
 public:
  // class constructor
  MainProcess();

  // class destructor
  virtual ~MainProcess();

  /**
   * @brief initialization.
   * @param [in] int argc: .
   * @param [in] char *argv[].
   * @return 0:success -1:fail
   */
  int Init(int argc, char *argv[]);

  /**
   * @brief initialization.
   * @param [in] int argc: .
   * @param [in] char *argv[].
   * @return 0:success -1:fail
   */
  int Run();

 private:
  // the attributes for mainproecss instance
  struct ControlObject control_object_;

  DebugInfo debug_info_;

  /**
   * @brief ready for creating a thread.
   * @param [in] int width: resolution
   * @param [in] int height: resolution
   * @return 0:success -1:fail
   */
  int CreateThreadInit(int width, int height);

  /**
   * @brief create a instance for camera.
   * @param [in] int width: resolution
   * @param [in] int height: resolution
   * @return MAINPROCESS_OK:success
   */
  void CameraInstanceInit(int width, int height);

  /**
   * @brief create a instance for dvpp.
   * @param [in] int width: resolution
   * @param [in] int height: resolution
   * @return MAINPROCESS_OK:success
   */
  void DvppInstanceInit(int width, int height);

  /**
   * @brief create a instance for output process.
   * @param [in] int width: resolution
   * @param [in] int height: resolution
   * @return
   */
  int OutputInstanceInit(int width, int height);

  /**
   * @brief Dvpp process and output the data
   * @param [in] CameraOutputPara *output_para: data buffer from camera
   * @param [in] DvppProcess *dvpp_process: point to dvpp instance
   * @param [in] OutputInfoProcess *output_info_process: output instance
   * @return 0:success -1:fail
   */
  int DvppAndOuputProc(CameraOutputPara *output_para,
                       ascend::utils::DvppProcess *dvpp_process,
                       OutputInfoProcess *output_info_process);

  /**
   * @brief create buffer for multi-frame
   * @param [in] struct h264Buf **buf: h264 multi-frame buffer.
   * @param [in] int size: size of h264 multi-frame buffer.
   * @param [in] int exit_flag: multi-frame buffer whether is last
   * @return enum MainProcessErrorCode
   */
  int CreateMultiFrameBuffer(H264Buf **buf, int size, int exit_flag);

  /**
   * @brief release buffer for multi-frame
   * @param [in] struct H264Buf *buf: h264 multi-frame buffer.
   * @return MAINPROCESS_OK:success
   */
  static int FreeMultiFrameBuffer(H264Buf *buf);

  /**
   * @brief multi-frame thread
   * @param [in] void *startup_arg: point to MainProcess instance.
   */
  static void *MultiFrameProcThread(void *startup_arg);

  /**
   * @brief create a multi-frame thread
   * @return -1:fail 0:success
   */
  int CreateMultiFrameThread(void);

  /**
   * @brief deal with multi-frame.
   * @param [in] CameraOutputPara *output_para buffer from camera.
   * @return enum MainProcessErrorCode
   */
  int MultiFrameBufferProc(CameraOutputPara *output_para);

  /**
   * @brief if timeout,we need to deal with the unprocessed data from camera.
   */
  void MultiFrameBufferTimeoutProc();

  /**
   * @brief deal with a frame data from camera.
   * @return
   */
  int DoOnce();

  /**
   * @brief deal witch multi-frame to dvpp and output process
   * @param [in] struct H264Buf *h264_buf: h264 multi-frame buffer.
   * @param [in] ,struct ControlObject *control_object:
   *               control process instance.
   * @return enum MainProcessErrorCode
   */
  static int DealMultiFrame(H264Buf *h264_buf,
                            struct ControlObject *control_object);

  /**
   * @brief process before exiting
   * @param [in] int ret: result in upper level.
   */
  void ExitProcess(int ret);

  /**
   * @brief get size of queue element
   * @return queue size
   */
  int GetQueueSize();
};
}
}
#endif /* ASCENDDK_ASCENDCAMERA_MAIN_PROCESS_H_ */

