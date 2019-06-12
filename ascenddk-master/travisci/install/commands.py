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
'''comamnd module'''

import os
import sys
import yaml

THIS_FILE_NAME = __file__
sys.path.append(os.path.join(os.path.dirname(
    os.path.realpath(THIS_FILE_NAME)), ".."))

import comm.ci_log as cilog



CONFIG_PATH = os.path.join(os.path.dirname(
    os.path.realpath(THIS_FILE_NAME)), "config")


class InstallationCommands():
    '''
    read commands from yaml file
    '''

    def __init__(self, command_file_name=None):
        self.commands = {}
        self.command_file_name = command_file_name
        self.error = False
        if self.command_file_name is None:
            self.command_file_name = "default"

        self.command_file = os.path.join(
            CONFIG_PATH, self.command_file_name + ".yaml")
        if not os.path.exists(self.command_file):
            self.error = True
            cilog.cilog_error(
                THIS_FILE_NAME, "command yaml file is not exist: %s", self.command_file)
            return
        cilog.cilog_debug(
            THIS_FILE_NAME, "read command yaml file: %s", self.command_file)

        try:
            stream = open(self.command_file, 'r')
            self.commands = yaml.load(stream)
        except OSError as reason:
            self.error = True
            cilog.cilog_error(
                THIS_FILE_NAME, "read command file failed: %s", reason)
        finally:
            # if stream in current symbol table:locals(), close it
            if stream in locals():
                stream.close()

    def get_install_commands(self):
        '''get install commands function'''
        if self.error:
            cilog.cilog_error(
                THIS_FILE_NAME, "no command yaml or config yaml not exist.")
            return False, None
        commands = self.commands.get("install")
        return True, commands
