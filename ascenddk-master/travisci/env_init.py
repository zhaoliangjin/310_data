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
''' env init '''

import os

import yaml

import comm.ci_log as cilog
import comm.util as util


def main():
    '''env init'''
    base_path = os.path.dirname(
        os.path.realpath(__file__))
    ret = util.execute("git branch", cwd=base_path)

    if ret[0] is False:
        exit(-1)

    branch_info = ret[1]
    code_branch = branch_info[0].split(" ")[1]
    cilog.print_in_color(code_branch, cilog.COLOR_F_YELLOW)

    file_path = os.path.join(base_path, "../.travis.yml")

    try:
        file_stream = open(file_path, 'r')
        travis_dict = yaml.load(file_stream)
        env_variables = travis_dict.get("env")
    except OSError as reason:
        print(reason)
        exit(-1)
    finally:
        if file_stream in locals():
            file_stream.close()
    try:
        env_variables_list = env_variables.split(" ")

        env_variables_list.append("TRAVIS_BRANCH=" + code_branch)

        env_variables_list = list(
            map(lambda x: "export " + x + "\n", env_variables_list))
        cilog.print_in_color("env list: %s" %
                             env_variables_list, cilog.COLOR_F_YELLOW)
        env_file = os.path.join(os.getenv("HOME"), ".bashrc_ascend")
        env_stream = open(env_file, 'w')
        env_stream.writelines(env_variables_list)
    except OSError as reason:
        print(reason)
        exit(-1)
    finally:
        if env_stream in locals():
            env_stream.close()

    # add env to bashrc
    bashrc_file = os.path.join(os.getenv("HOME"), ".bashrc")
    try:
        bashrc_read_stream = open(bashrc_file, 'r')
        all_lines = []
        while True:
            lines = bashrc_read_stream.readlines(10000)
            if not lines:
                break
            all_lines.extend(lines)
    except OSError as reason:
        print(reason)
        exit(-1)
    finally:
        if bashrc_read_stream in locals():
            bashrc_read_stream.close()
    # if bashrc haven been added, skip it
    if ". ~/.bashrc_ascend" not in all_lines:
        try:
            bashrc_write_stream = open(bashrc_file, 'a')
            bashrc_write_stream.write(". ~/.bashrc_ascend")
        except OSError as reason:
            print(reason)
        finally:
            if bashrc_write_stream in locals():
                bashrc_write_stream.close()


if __name__ == '__main__':
    main()
