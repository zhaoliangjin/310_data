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

#include "ascenddk/ascendcamera/main_process.h"

#include <time.h>
#include <unistd.h>
#include <stdio.h>

#include "ascenddk/ascendcamera/camera.h"
#include "ascenddk/ascendcamera/output_info_process.h"
#include "ascenddk/ascendcamera/ascend_camera_common.h"

using namespace std;

namespace ascend {
namespace ascendcamera {
MainProcess::MainProcess() {
  // constuct a instance used to control the whole process.
  control_object_.ascend_camera_paramter = nullptr;
  control_object_.camera = nullptr;
  control_object_.dvpp_process = nullptr;
  control_object_.output_process = nullptr;
  control_object_.loop_flag = kNoNeedLoop;
  control_object_.multi_frame_process.thread_id = 0;
  control_object_.multi_frame_process.safe_queue = nullptr;
  control_object_.multi_frame_process.current_buf = nullptr;
  control_object_.multi_frame_process.open_thread_flag = kThreadStop;
  control_object_.multi_frame_process.thread_status = kThreadInvalidStatus;

  // initialization debugging info
  debug_info_.total_frame = 0;
  debug_info_.queue_max_length = 0;
}

MainProcess::~MainProcess() {
  // release all
  if (control_object_.ascend_camera_paramter != nullptr) {
    delete control_object_.ascend_camera_paramter;
    control_object_.ascend_camera_paramter = nullptr;
  }

  if (control_object_.camera != nullptr) {
    delete control_object_.camera;
    control_object_.camera = nullptr;
  }

  if (control_object_.dvpp_process != nullptr) {
    delete control_object_.dvpp_process;
    control_object_.dvpp_process = nullptr;
  }

  if (control_object_.output_process != nullptr) {
    delete control_object_.output_process;
    control_object_.dvpp_process = nullptr;
  }

  if (control_object_.multi_frame_process.safe_queue != nullptr) {
    delete control_object_.multi_frame_process.safe_queue;
    control_object_.multi_frame_process.safe_queue = nullptr;
  }

}

int MainProcess::OutputInstanceInit(int width, int height) {
  int ret = kMainProcessOk;

  OutputInfoPara output_para;
  if (control_object_.ascend_camera_paramter->GetMediaType() == kImage) {
    output_para.presenter_para.content_type =
        ascend::presenter::ContentType::kImage;
  } else if (control_object_.ascend_camera_paramter->GetMediaType() == kVideo) {
    output_para.presenter_para.content_type =
        ascend::presenter::ContentType::kVideo;
  }

  string str = control_object_.ascend_camera_paramter->GetOutputFile();
  // if user input "-o",it means output to stdout or save file to local
  if (!str.empty()) {
    // ouput to stdout or save file to local.
    output_para.mode = (str == "-") ? kOutputToStdout : kOutputToLocal;
    output_para.path = (str == "-") ? "" : str;
    output_para.width = width;
    output_para.height = height;
    control_object_.output_process = new OutputInfoProcess(output_para);

    // open output channel
    ret = control_object_.output_process->OpenOutputChannel();
  } else {  // if user input "-s",it means ouput to presenter.
    string str = control_object_.ascend_camera_paramter->GetOutputPresenter();
    if (!str.empty()) {
      // x.x.x.x:yyy/zzz/zz
      // x.x.x.x--ip   yyy--port   zzz/zz--channelName
      output_para.presenter_para.host_ip = str.substr(
          kFirstIndex, str.find(kPortIpSeparator, kFirstIndex));

      output_para.presenter_para.port = stoi(
          str.substr(
              str.find(kPortIpSeparator, kFirstIndex) + 1,
              str.find_first_of(KPortChannelSeparator, kFirstIndex)
                  - str.find(kPortIpSeparator, kFirstIndex) - 1));

      output_para.presenter_para.channel_name = str.substr(
          str.find_first_of(KPortChannelSeparator, kFirstIndex) + 1,
          str.length() - str.find_first_of(KPortChannelSeparator, kFirstIndex));

      output_para.mode = kOutputToPresenter;
      output_para.path = "";
      output_para.width = width;
      output_para.height = height;
      control_object_.output_process = new OutputInfoProcess(output_para);

      // open output channel
      ret = control_object_.output_process->OpenOutputChannel();
    }
  }

  if (ret != kMainProcessOk) {
    control_object_.output_process->PrintErrorInfo(ret);
  }
  return ret;
}

void MainProcess::DvppInstanceInit(int width, int height) {
  // 1. In mode of video to presenter server, it need picture mode.
  // 2. The user points to picture.
  string str = control_object_.ascend_camera_paramter->GetOutputPresenter();
  bool is_need_convert_to_jpg = (!str.empty())
      && (control_object_.ascend_camera_paramter->GetMediaType() == kVideo);

  // instance(convert to jpg)
  if (is_need_convert_to_jpg
      || (control_object_.ascend_camera_paramter->GetMediaType() == kImage)) {
    ascend::utils::DvppToJpgPara dvpp_to_jpg_para;

    dvpp_to_jpg_para.format = JPGENC_FORMAT_NV12;
    dvpp_to_jpg_para.level = kDvppToJpgQualityParameter;
    dvpp_to_jpg_para.resolution.height = height;
    dvpp_to_jpg_para.resolution.width = width;
    control_object_.dvpp_process = new ascend::utils::DvppProcess(
        dvpp_to_jpg_para);
    if (is_need_convert_to_jpg) {
      // if output object is presenter and the mode is video ,
      // dvpp chose picture mode and need Continuous convert yuv to jpg
      control_object_.loop_flag = kNeedLoop;
    }
  } else if (control_object_.ascend_camera_paramter->GetMediaType() == kVideo) {
    ascend::utils::DvppToH264Para dvpp_to_h264_para;

    // instance(convert to h264)
    dvpp_to_h264_para.coding_type = ascend::utils::kH264High;
    dvpp_to_h264_para.yuv_store_type = ascend::utils::kYuv420sp;
    dvpp_to_h264_para.resolution.width = width;
    dvpp_to_h264_para.resolution.height = height;
    control_object_.dvpp_process = new ascend::utils::DvppProcess(
        dvpp_to_h264_para);
    control_object_.loop_flag = kNeedLoop;
  }

}

void MainProcess::CameraInstanceInit(int width, int height) {
  CameraPara camera_para;

  // camera instance paramter
  camera_para.fps = control_object_.ascend_camera_paramter->GetFps();
  camera_para.capture_obj_flag = control_object_.ascend_camera_paramter
      ->GetMediaType();
  camera_para.channel_id = control_object_.ascend_camera_paramter
      ->GetCameraChannel();
  camera_para.image_format = CAMERA_IMAGE_YUV420_SP;
  camera_para.timeout = control_object_.ascend_camera_paramter->GetTimeout();
  camera_para.resolution.width = width;
  camera_para.resolution.height = height;

  // camera instance
  control_object_.camera = new Camera(camera_para);
}

int MainProcess::CreateThreadInit(int width, int height) {
  int ret = kMainProcessOk;

  // init thread flag
  control_object_.multi_frame_process.open_thread_flag = kThreadStop;
  control_object_.multi_frame_process.thread_status = kThreadInvalidStatus;
  control_object_.multi_frame_process.thread_error_code = 0;  //no error

  //if fps > 10 ,it need multi-thread
  if (control_object_.ascend_camera_paramter->GetFps()
      > kStartupThreadThresHold) {
    int size = width * height * kYuv420spSizeNumerator
        / kYuv420spSizeDenominator;
    H264Buf *buffer = nullptr;

    // set thread running flag
    control_object_.multi_frame_process.open_thread_flag = kThreadStartup;

    // create thread-safe queue
    control_object_.multi_frame_process.safe_queue = new ThreadSafeQueue<
        H264Buf *>();

    // create a queue element
    ret = CreateMultiFrameBuffer(&buffer, size, kThreadNoNeedExit);
    if (ret != kMainProcessOk) {
      return ret;
    }

    control_object_.multi_frame_process.current_buf = buffer;

    // clear thread status
    control_object_.multi_frame_process.thread_status = kThreadInvalidStatus;

    // create a thread to deal with multi-frame
    ret = this->CreateMultiFrameThread();
    if (ret != kMainProcessOk) {
      FreeMultiFrameBuffer(buffer);
      return ret;
    }

    // set thread status
    control_object_.multi_frame_process.thread_status = kThreadRunStatus;

    ASC_LOG_INFO("The multi-frame mode of ascendcamera is start.");
  }
  return ret;
}

int MainProcess::Init(int argc, char *argv[]) {

  control_object_.ascend_camera_paramter = new AscendCameraParameter();
  if (!(control_object_.ascend_camera_paramter->Init(argc, argv))) {
    return kParaCheckError;
  }

  // verify ascendcamera_parameter parameters
  if (!(control_object_.ascend_camera_paramter->Verify())) {
    return kParaCheckError;
  }

  int width = control_object_.ascend_camera_paramter->GetImageWidth();
  int height = control_object_.ascend_camera_paramter->GetImageHeight();

  // camera instance
  CameraInstanceInit(width, height);

  // dvpp controller instance
  DvppInstanceInit(width, height);

  // output instance
  int ret = OutputInstanceInit(width, height);
  if (ret != kMainProcessOk) {
    return ret;
  }

  if (control_object_.dvpp_process->GetMode() == ascend::utils::kH264) {
    ret = CreateThreadInit(width, height);
  }

  return ret;
}

int MainProcess::GetQueueSize() {
  return control_object_.multi_frame_process.safe_queue->Size();
}

int MainProcess::CreateMultiFrameBuffer(H264Buf **buffer, int size,
                                        int exit_flag) {
  // create struct h264Buf and a ten-frame buffer
  if ((buffer == nullptr) || (size > kYuvImageMaxSize)) {
    return kMainProcessInvalidParameter;
  }

  *buffer = new H264Buf;

  (*buffer)->single_frame_size = size;
  (*buffer)->index = 0;
  (*buffer)->buf = new char[kH264BufferMaxFrame * size];
  (*buffer)->exit_flag = exit_flag;

  return kMainProcessOk;
}

int MainProcess::FreeMultiFrameBuffer(H264Buf *h264_buf) {
  // relase buffer
  if (h264_buf != nullptr) {
    if (h264_buf->buf != nullptr) {
      delete[] h264_buf->buf;
    }

    delete h264_buf;
  }

  return kMainProcessOk;
}
int MainProcess::DealMultiFrame(H264Buf *h264_buf,
                                ControlObject *control_object) {
  if ((h264_buf == nullptr) || (control_object == nullptr)) {
    return kMainProcessMultiframeInvalidParameter;
  }

  // DVPP conversion
  ascend::utils::DvppOutput dvpp_output = { nullptr, 0 };
  int ret = control_object->dvpp_process->DvppOperationProc(
      h264_buf->buf, h264_buf->single_frame_size * h264_buf->index,
      &dvpp_output);

  if (ret != kMainProcessOk) {
    control_object->dvpp_process->PrintErrorInfo(ret);
    FreeMultiFrameBuffer(h264_buf);

    return ret;
  }

  // send to channel
  ret = control_object->output_process->SendToChannel(dvpp_output.buffer,
                                                      dvpp_output.size);
  if (ret != kMainProcessOk) {
    control_object->output_process->PrintErrorInfo(ret);
  }

  // release buffer
  FreeMultiFrameBuffer(h264_buf);
  if (dvpp_output.buffer != nullptr) {
    delete[] dvpp_output.buffer;
  }

  return ret;
}

void *ascend::ascendcamera::MainProcess::MultiFrameProcThread(
    void *startup_arg) {
  H264Buf *h264_buf = nullptr;
  ControlObject *control_obj = (struct ControlObject *) startup_arg;

  while (control_obj->multi_frame_process.open_thread_flag == kThreadStartup) {
    // If not data,thread will block.
    control_obj->multi_frame_process.safe_queue->WaitAndPop(h264_buf);

    // The exit flag is seted ,we will ready for exit.
    if (h264_buf->exit_flag == kThreadNeedExit) {
      control_obj->multi_frame_process.open_thread_flag = kThreadStop;
      // if index is equal 0,we will immediately exit.
      // if index is not equal 0,we will deal with the remaining part.
      if (h264_buf->index == 0) {
        FreeMultiFrameBuffer(h264_buf);
        continue;
      }
    }

    int ret = DealMultiFrame(h264_buf, control_obj);
    // deal with the multi-frame data.
    if (ret != kMainProcessOk) {
      control_obj->multi_frame_process.thread_status = kThreadErrorStatus;
      control_obj->multi_frame_process.thread_error_code = ret;
      return nullptr;
    }
  }

  return nullptr;
}

int MainProcess::CreateMultiFrameThread(void) {
  pthread_t tid = 0;  // used to get new thread id.

  // create a thread
  int ret = pthread_create(&tid, nullptr, MultiFrameProcThread,
                           &control_object_);
  if (ret != kMainProcessOk) {
    cerr << "[ERROR] Failed to create thread." << endl;
    ASC_LOG_ERROR("Failed to create thread.return value = %d", ret);
    return kSystemCallReturnError;
  }

  control_object_.multi_frame_process.thread_id = tid;
  return kMainProcessOk;
}

int MainProcess::MultiFrameBufferProc(CameraOutputPara *output_para) {
  int ret = kMainProcessOk;
  H264Buf *buf_h264 = nullptr;

  // if  startup thread,we collect ten frames to multi-frame process thread.
  buf_h264 = control_object_.multi_frame_process.current_buf;
  ret = memcpy_s(
      buf_h264->buf + (buf_h264->index) * (buf_h264->single_frame_size),
      buf_h264->single_frame_size, output_para->data.get(), output_para->size);
  CHECK_MEMCPY_S_RESULT(ret);
  buf_h264->index += 1;

  // ten frames
  if (buf_h264->index < kH264BufferMaxFrame) {
    return kMainProcessOk;
  } else {
    do {
      // if queue is not full ,the ten-frame push to queue.
      if (GetQueueSize() < kH264BufferQueueMaxLength) {
        int single_frame_size = buf_h264->single_frame_size;

        H264Buf *buff_h264_temp = nullptr;
        // create a new buffer
        ret = CreateMultiFrameBuffer(&buff_h264_temp, single_frame_size,
                                     kThreadNoNeedExit);
        if (ret != kMainProcessOk) {
          control_object_.multi_frame_process.current_buf = nullptr;
          cerr << "[ERROR] Failed to create multi-frame buffer." << endl;
          ASC_LOG_ERROR("Failed to create multi-frame buffer.");
          buf_h264->exit_flag = kThreadNeedExit;
          control_object_.multi_frame_process.safe_queue->Push(buf_h264);
          pthread_join(control_object_.multi_frame_process.thread_id, nullptr);
          control_object_.multi_frame_process.thread_status =
              kThreadInvalidStatus;
          return kMainProcessMultiframeMallocFail;
        }

        control_object_.multi_frame_process.current_buf = buff_h264_temp;
        control_object_.multi_frame_process.safe_queue->Push(buf_h264);

        // record debug info
        if (debug_info_.queue_max_length < GetQueueSize()) {
          debug_info_.queue_max_length = GetQueueSize();
        }

        break;
      } else {  //wait some time
        usleep(kWaitTime);
        continue;
      }
    } while (1);
  }

  return kMainProcessOk;
}

void MainProcess::MultiFrameBufferTimeoutProc() {
  H264Buf *buf_h264 = nullptr;
  int max_retry_num = 0;

  // if time is up,it will put the last frame to thread.
  if ((control_object_.multi_frame_process.open_thread_flag == kThreadStartup)
      && (control_object_.multi_frame_process.thread_status
          != kThreadInvalidStatus)) {

    // if  startup thread,we collect ten frames to
    // multi-frame process thread.
    buf_h264 = control_object_.multi_frame_process.current_buf;

    do {
      if (GetQueueSize() < kH264BufferQueueMaxLength) {
        //set exit flag
        buf_h264->exit_flag = kThreadNeedExit;
        control_object_.multi_frame_process.safe_queue->Push(buf_h264);
        control_object_.multi_frame_process.current_buf = nullptr;
        break;
      } else {  //wait some time and retry
        usleep(kWaitTime);
        max_retry_num++;
        continue;
      }
    } while (max_retry_num < kMaxRetryNum);

    // wait children thread exit
    pthread_join(control_object_.multi_frame_process.thread_id, nullptr);
    control_object_.multi_frame_process.thread_status = kThreadInvalidStatus;
  }
}

int MainProcess::DvppAndOuputProc(CameraOutputPara *output_para,
                                  ascend::utils::DvppProcess *dvpp_process,
                                  OutputInfoProcess *output_info_process) {
  int ret = 0;
  // use multi-thread to deal with multi-frame
  if (dvpp_process->GetMode() == ascend::utils::kH264) {
    if ((control_object_.multi_frame_process.open_thread_flag == kThreadStartup)
        && (control_object_.multi_frame_process.thread_status
            == kThreadRunStatus)) {
      ret = MultiFrameBufferProc(output_para);
      return ret;
    }

    if ((control_object_.multi_frame_process.open_thread_flag == kThreadStartup)
        && (control_object_.multi_frame_process.thread_status
            == kThreadErrorStatus)) {
      return control_object_.multi_frame_process.thread_error_code;
    }
  }

  ascend::utils::DvppOutput dvpp_output = { nullptr, 0 };
  // DVPP convert to jpg or h264
  ret = dvpp_process->DvppOperationProc(output_para->data.get(),
                                        output_para->size, &dvpp_output);
  if (ret != kMainProcessOk) {
    dvpp_process->PrintErrorInfo(ret);
    return ret;
  }

  // output to channel
  ret = output_info_process->SendToChannel(dvpp_output.buffer,
                                           dvpp_output.size);
  if (ret != kMainProcessOk) {
    output_info_process->PrintErrorInfo(ret);
  }

  delete[] dvpp_output.buffer;
  return ret;
}

int MainProcess::DoOnce() {
  CameraOutputPara output_para;

  // get a frame from camera
  int ret = control_object_.camera->CaptureCameraInfo(&output_para);
  if (ret != kMainProcessOk) {
    control_object_.camera->PrintErrorInfo(ret);
    return ret;
  }

  ret = DvppAndOuputProc(&output_para, control_object_.dvpp_process,
                         control_object_.output_process);
  if (ret != kMainProcessOk) {
    return ret;
  }

  return ret;
}

int MainProcess::Run() {

  if ((control_object_.camera == nullptr)
      || (control_object_.ascend_camera_paramter == nullptr)
      || (control_object_.dvpp_process == nullptr)
      || (control_object_.output_process == nullptr)) {
    return kParaCheckError;
  }

  // init driver of camera and open camera.
  int ret = control_object_.camera->InitCamera();
  if (ret != kCameraInitOk) {
    control_object_.camera->PrintErrorInfo(ret);
    MultiFrameBufferTimeoutProc();
    return ret;
  } else {  // print to terminal
    cerr << "[INFO] Success to open camera["
         << control_object_.camera->GetChannelId() << "],and start working."
         << endl;
    ASC_LOG_INFO("Success to open camera[%d],and start working.",
                 control_object_.camera->GetChannelId());
  }

  int timeout = control_object_.camera->GetUserTimeout();

  // get begin time
  struct timespec begin_time = { 0, 0 };
  struct timespec current_time = { 0, 0 };
  long long running_time = 0;
  clock_gettime(CLOCK_MONOTONIC, &begin_time);

  do {
    // get current time
    clock_gettime(CLOCK_MONOTONIC, &current_time);

    // get running time
    running_time = (current_time.tv_sec - begin_time.tv_sec) * kSecToMillisec
        + current_time.tv_nsec / kMillSecToNanoSec
        - begin_time.tv_nsec / kMillSecToNanoSec;

    // get and deal with a frame from camera.
    ret = DoOnce();
    if (ret != kMainProcessOk) {
      cerr << "[ERROR] Failed to complete the task." << endl;
      ASC_LOG_ERROR("Failed to complete the task.");
      break;
    }

    // we exit the loop if The running time more than the specified time.
    if ((running_time > (long long) timeout * kSecToMillisec)
        && (timeout != 0)) {
      cerr << "[INFO] Success to complete the task." << endl;
      ASC_LOG_ERROR("Success to complete the task.");
      break;
    }

    // record debugging info.
    debug_info_.total_frame++;
    if (debug_info_.total_frame % kDebugRecordFrameThresHold == 0) {
      ASC_LOG_INFO("total frame = %ld,running time = %lldms,"
          "Queue Maximum length = %d.", debug_info_.total_frame, running_time,
          debug_info_.queue_max_length);
    }
  } while (control_object_.loop_flag == kNeedLoop);

  ExitProcess(ret);

  return ret;
}

void MainProcess::ExitProcess(int ret) {
  // close camera.
  if (control_object_.camera != nullptr) {
    CloseCamera(control_object_.camera->GetChannelId());
    // close camera
    cerr << "[INFO] Close camera [" << control_object_.camera->GetChannelId()
         << "]." << endl;
    ASC_LOG_INFO("close camera[%d].", control_object_.camera->GetChannelId());
  }

  // deal with the last frame.
  if (ret != kMainProcessMultiframeMallocFail) {
    MultiFrameBufferTimeoutProc();
  }

  // for the thread abnormal exit,pop all queue element and release buffer
  while ((control_object_.multi_frame_process.safe_queue != nullptr)
      && (!control_object_.multi_frame_process.safe_queue->Empty())
      && (control_object_.multi_frame_process.thread_status
          == kThreadInvalidStatus)) {
    H264Buf *h264_buf = nullptr;
    control_object_.multi_frame_process.safe_queue->TryPop(h264_buf);
    FreeMultiFrameBuffer(h264_buf);
  }

  // release buffer
  if (control_object_.multi_frame_process.current_buf != nullptr) {
    FreeMultiFrameBuffer(control_object_.multi_frame_process.current_buf);
  }

  // close the output channel.
  if (control_object_.output_process != nullptr) {
    control_object_.output_process->CloseChannel();
  }
}
}
}
