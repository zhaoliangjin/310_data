#
#   =======================================================================
#
# Copyright (C) 2018, Hisilicon Technologies Co., Ltd. All Rights Reserved.
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions are met:
#
#   1 Redistributions of source code must retain the above copyright notice,
#     this list of conditions and the following disclaimer.
#
#   2 Redistributions in binary form must reproduce the above copyright notice,
#     this list of conditions and the following disclaimer in the documentation
#     and/or other materials provided with the distribution.
#
#   3 Neither the names of the copyright holders nor the names of the
#   contributors may be used to endorse or promote products derived from this
#   software without specific prior written permission.
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
# AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
# IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
# ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
# LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
# CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
# SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
# INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
# CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
# ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
# POSSIBILITY OF SUCH DAMAGE.
#   =======================================================================
#
'''common log module'''

import datetime
import inspect
import os
import sys

LEVELS = {"DEBUG": 1, "INFO": 2, "WARNING": 3, "ERROR": 4}

# 打印颜色，’B‘开头的表示背景色，’F‘开关表示前景色
COLOR_B_BLACK = 40
COLOR_B_RED = 41
COLOR_B_GREEN = 42
COLOR_B_YELLOW = 43
COLOR_B_BLUE = 44
COLOR_B_PURPLE = 45
COLOR_B_CYANINE = 46
COLOR_B_WHITE = 47
COLOR_F_BLACK = 30
COLOR_F_RED = 31
COLOR_F_GREEN = 32
COLOR_F_YELLOW = 33
COLOR_F_BLUE = 34
COLOR_F_PURPLE = 35
COLOR_F_CYANINE = 36
COLOR_F_WHITE = 37


def cilog_get_timestamp():
    '''get timestamp'''
    timestamp_microsecond = datetime.datetime.now().strftime('%Y%m%d_%H%M%S%f')
    return timestamp_microsecond[0:18]


def cilog_print_element(cilog_element):
    '''print log element'''
    print("[" + cilog_element + "]", end=' ')


def cilog_logmsg(log_level, filename, line_no, funcname, log_msg, *log_paras):
    '''print log message'''
    if LEVELS[log_level] < LEVELS[os.getenv("LOG_LEVEL", "DEBUG")]:
        return
    log_timestamp = cilog_get_timestamp()

    cilog_print_element(log_timestamp)
    cilog_print_element(log_level)
    cilog_print_element(filename)
    cilog_print_element(str(line_no))
    cilog_print_element(funcname)

    print(log_msg % log_paras[0])
    sys.stdout.flush()


def cilog_debug(filename, log_msg, *log_paras):
    '''print log in debug level'''
    line_no = inspect.currentframe().f_back.f_lineno
    funcname = inspect.currentframe().f_back.f_code.co_name
    cilog_logmsg("DEBUG", filename, line_no, funcname, log_msg, log_paras)


def cilog_error(filename, log_msg, *log_paras):
    '''print log in error level'''
    line_no = inspect.currentframe().f_back.f_lineno
    funcname = inspect.currentframe().f_back.f_code.co_name
    cilog_logmsg("ERROR", filename, line_no, funcname,
                 "\x1b[41m" + log_msg + "\x1b[0m", log_paras)


def cilog_warning(filename, log_msg, *log_paras):
    '''print log in warning level'''
    line_no = inspect.currentframe().f_back.f_lineno
    funcname = inspect.currentframe().f_back.f_code.co_name
    cilog_logmsg("WARNING", filename, line_no, funcname,
                 "\x1b[43m" + log_msg + "\x1b[0m", log_paras)


def cilog_info(filename, log_msg, *log_paras):
    '''print log in info level'''
    line_no = inspect.currentframe().f_back.f_lineno
    funcname = inspect.currentframe().f_back.f_code.co_name
    cilog_logmsg("INFO", filename, line_no, funcname, log_msg, log_paras)


def cilog_info_color(filename, color, log_msg, *log_paras):
    '''print log in defined color'''
    color_str = "\x1b[%dm" % color
    line_no = inspect.currentframe().f_back.f_lineno
    funcname = inspect.currentframe().f_back.f_code.co_name
    cilog_logmsg("INFO", filename, line_no, funcname,
                 color_str + log_msg + "\x1b[0m", log_paras)


def print_in_color(msg, color):
    '''print standard output in defined color'''
    color_str = "\x1b[%dm" % color
    print(color_str + msg + "\x1b[0m")
