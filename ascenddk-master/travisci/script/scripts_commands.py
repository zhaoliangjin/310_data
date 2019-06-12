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
'''static check commands'''

import os
import sys
import yaml

THIS_FILE_NAME = __file__
sys.path.append(os.path.join(os.path.dirname(
    os.path.realpath(THIS_FILE_NAME)), ".."))

import comm.ci_log as cilog

CONFIG_PATH = os.path.join(os.path.dirname(
    os.path.realpath(THIS_FILE_NAME)), "config")


class ScriptsCommands():
    '''
    read commands from yaml file
    '''

    def __init__(self, check_type, command_type):
        '''init function'''
        self.commands = {}
        self.sub_params = None
        self.check_type = check_type
        self.command_type = command_type
        self.error = False

        self.static_check_command_file = os.path.join(
            CONFIG_PATH, self.check_type + ".yaml")
        if not os.path.exists(self.static_check_command_file):
            self.error = True
            cilog.cilog_error(
                THIS_FILE_NAME, "static check file is not exist: static_check.yaml")

        cilog.cilog_debug(
            THIS_FILE_NAME, "read command yaml file: %s", self.static_check_command_file)

        try:
            static_stream = open(self.static_check_command_file, 'r')
            static_dict = yaml.load(static_stream)
            self.commands = static_dict.get(command_type)
            self.validate_commands()
            if self.error:
                return

            self.command_file = os.path.join(
                CONFIG_PATH, self.check_type + "_" + self.command_type + ".yaml")
            if os.path.exists(self.command_file):
                cilog.cilog_debug(
                    THIS_FILE_NAME, "read sub command yaml file: %s", self.command_file)
                try:
                    sub_stream = open(self.command_file, 'r')
                    self.sub_params = yaml.load(sub_stream)
                except OSError as reason:
                    self.error = True
                    cilog.cilog_error(
                        THIS_FILE_NAME, "read command file failed: %s", reason)
                finally:
                    # if stream in current symbol table:locals(), close it
                    if sub_stream in locals():
                        sub_stream.close()
        except OSError as reason:
            self.error = True
            cilog.cilog_error(
                THIS_FILE_NAME, "read command file failed: %s", reason)
        finally:
            # if stream in current symbol table:locals(), close it
            if static_stream in locals():
                static_stream.close()

    def get_commands(self):
        '''get static check commands'''
        if self.error is True:
            cilog.cilog_error(
                THIS_FILE_NAME, "get command failed.")
            return False, None
        return True, self.commands

    def get_sub_params(self):
        '''get static check sub params'''
        if self.error is True:
            cilog.cilog_error(
                THIS_FILE_NAME, "get sub params failed.")
            return False, None
        return True, self.sub_params

    def validate_commands(self):
        '''validate commands'''
        for each_command in self.commands:
            command_type = each_command.get("type")
            if command_type is None:
                cilog.cilog_error(
                    THIS_FILE_NAME, "type is invalid: %s", each_command)
                self.error = True
                break

            if command_type == "command":
                if "cmd" not in each_command.keys():
                    cilog.cilog_error(
                        THIS_FILE_NAME, "cmd is invalid: %s", each_command)
                    self.error = True
                    break
            elif command_type == "function":
                if "function_name" not in each_command.keys():
                    cilog.cilog_error(
                        THIS_FILE_NAME, "function_name is invalid: %s", each_command)
                    self.error = True
                    break
            else:
                cilog.cilog_error(
                    THIS_FILE_NAME, "type is invalid: %s", each_command)
                self.error = True
                break
