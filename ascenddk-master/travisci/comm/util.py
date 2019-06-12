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

import os
import platform
import signal
import subprocess
import time

import comm.ci_log as cilog


THIS_FILE_NAME = __file__


def execute(cmd, timeout=3600, print_output_flag=False, print_cmd=True, cwd=""):
    '''execute os command'''
    if print_cmd:
        if len(cmd) > 2000:
            cilog.print_in_color("%s ... %s" %
                                 (cmd[0:100], cmd[-100:]), cilog.COLOR_F_YELLOW)
        else:
            cilog.print_in_color(cmd, cilog.COLOR_F_YELLOW)

    is_linux = platform.system() == 'Linux'

    if not cwd:
        cwd = os.getcwd()
    process = subprocess.Popen(cmd, cwd=cwd, bufsize=32768, stdout=subprocess.PIPE,
                               stderr=subprocess.STDOUT, shell=True,
                               preexec_fn=os.setsid if is_linux else None)

    t_beginning = time.time()

    time_gap = 0.01

    str_std_output = ""
    while True:

        str_out = str(process.stdout.read().decode())
        str_std_output = str_std_output + str_out

        if process.poll() is not None:
            break
        seconds_passed = time.time() - t_beginning

        if timeout and seconds_passed > timeout:

            if is_linux:
                os.kill(process.pid, signal.SIGTERM)
            else:
                process.terminate()
            cilog.cilog_error(THIS_FILE_NAME,
                              "execute %s timeout! excute seconds passed " \
                              " :%s, timer length:%s, return code %s",
                              cmd, seconds_passed, timeout, process.returncode)
            return False, process.stdout.readlines()
        time.sleep(time_gap)
    str_std_output = str_std_output.strip()
    std_output_lines_last = []
    std_output_lines = str_std_output.split("\n")
    for i in std_output_lines:
        std_output_lines_last.append(i)

    if process.returncode != 0 or "Traceback" in str_std_output:
        cilog.print_in_color(str_std_output, cilog.COLOR_F_RED)
        return False, std_output_lines_last

    if print_output_flag:
        cilog.print_in_color(str_std_output, cilog.COLOR_F_YELLOW)

    return True, std_output_lines_last
