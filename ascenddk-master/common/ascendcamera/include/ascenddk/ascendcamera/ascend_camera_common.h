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

#ifndef ASCENDDK_ASCENDCAMERA_ASCEND_CAMERA_COMMON_H_
#define ASCENDDK_ASCENDCAMERA_ASCEND_CAMERA_COMMON_H_

#include "toolchain/slog.h"

namespace ascend {

namespace ascendcamera {

// used for record info-level log info to dlog
#ifndef ASC_LOG_INFO
#define ASC_LOG_INFO(fmt, ...) \
  dlog_info(kAscendModuleId, "[%s:%d] " fmt "\n", __FILE__, __LINE__, \
            ##__VA_ARGS__)
#endif

// used for record warning-level log info to dlog
#ifndef ASC_LOG_WARN
#define ASC_LOG_WARN(fmt, ...) \
  dlog_warn(kAscendModuleId, "[%s:%d] " fmt "\n", __FILE__,  __LINE__, \
            ##__VA_ARGS__)
#endif

// used for record error-level log info to dlog
#ifndef ASC_LOG_ERROR
#define ASC_LOG_ERROR(fmt, ...) \
  dlog_error(kAscendModuleId, "[%s:%d] " fmt "\n", __FILE__,  __LINE__, \
             ##__VA_ARGS__)
#endif

// used for error info description
struct ErrorDescription {
  int code;
  std::string code_info;
};

// ascend module id, used for record dlog
const int kAscendModuleId = ASCENDDK;

// has no write permission
const int kHasNoAccessPermission = -1;

// the first index
const int kIndexFirst = 0;

// the second index
const int kIndexSecond = 1;

// string compare to equal
const int kCompareEqual = 0;

// parameter maximum length
const int kMaxParamLength = 4096;

// used for image size conversion
const int kYuv420spSizeNumerator = 3;

// used for image size conversion
const int kYuv420spSizeDenominator = 2;
}
}

#endif /* ASCENDDK_ASCENDCAMERA_ASCEND_CAMERA_COMMON_H_ */
