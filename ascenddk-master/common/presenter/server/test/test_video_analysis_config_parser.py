"""utest config parser module"""

# -*- coding: UTF-8 -*-
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
import os
import sys
import unittest
from unittest.mock import patch
path = os.path.dirname(__file__)
index = path.rfind("ascenddk")
workspace = path[0: index]
path = os.path.join(workspace, "ascenddk/common/presenter/server/")
sys.path.append(path)

import video_analysis.src.config_parser as config_parser

class TestConfigReader(unittest.TestCase):
    @patch('os.path.isdir')
    def test_config_verify(self, mock):
        config = config_parser.ConfigParser()

        config_parser.ConfigParser.reserved_space = 500
        ret = config.config_verify()
        self.assertEqual(ret, False)
        config_parser.ConfigParser.reserved_space = "abc"
        ret = config.config_verify()
        self.assertEqual(ret, False)
        config_parser.ConfigParser.reserved_space = 1024


        backup = config_parser.ConfigParser.web_server_ip
        config_parser.ConfigParser.web_server_ip = '0.0.0.0'
        ret = config.config_verify()
        self.assertEqual(ret, False)
        config_parser.ConfigParser.web_server_ip = backup

        backup = config_parser.ConfigParser.max_app_num
        config_parser.ConfigParser.max_app_num = 100
        ret = config.config_verify()
        self.assertEqual(ret, False)
        config_parser.ConfigParser.max_app_num = backup

        mock.return_value = False
        ret = config.config_verify()
        self.assertEqual(ret, False)

        mock.return_value = True
        ret = config.config_verify()
        self.assertEqual(ret, True)
if __name__ == '__main__':
    unittest.main()
