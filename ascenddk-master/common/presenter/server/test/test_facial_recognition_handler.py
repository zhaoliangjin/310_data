"""utest channel handler module"""

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
import time
import threading
import unittest
from unittest.mock import patch
path = os.path.dirname(__file__)
index = path.rfind("ascenddk")
workspace = path[0: index]
path = os.path.join(workspace, "ascenddk/common/presenter/server/")
sys.path.append(path)
from common.channel_manager import ChannelManager
import facial_recognition.src.facial_recognition_handler as facial_recognition_handler
from facial_recognition.src.facial_recognition_handler import FacialRecognitionHandler
import common.channel_handler as channel_handler


def mock_wait(a):
    time.sleep(0.1)
    return True

class TestFacialRecognitionHandler(unittest.TestCase):
    """TestFacialRecognitionHandler"""
    channel_name = "facial_recognition"
    media_type = "video"
    channel_manager = ChannelManager()
    handler = None

    def func_end(self):
        self.handler.close_thread_switch = True
        channel_name = TestFacialRecognitionHandler.channel_name
        TestFacialRecognitionHandler.channel_manager.unregister_one_channel(channel_name)

    def func_begin(self):
        channel_name = TestFacialRecognitionHandler.channel_name
        media_type = TestFacialRecognitionHandler.media_type
        TestFacialRecognitionHandler.channel_manager.register_one_channel(channel_name)
        self.handler = FacialRecognitionHandler(channel_name, media_type)

    @classmethod
    def tearDownClass(cls):
        pass

    @classmethod
    def setUpClass(cls):
        pass


    def run_thread(self, func):
        thread = threading.Thread(target=func)
        thread.start()

    def set_img_data(self):
        time.sleep(0.5)
        self.handler.img_data = b'1234'

    def set_img_data_none(self):
        time.sleep(1.5)
        self.handler.img_data = None


    @patch('threading.Event.clear', return_value = True)
    @patch('threading.Event.wait', return_value = True)
    def test_save_frame1(self, mock1, mock2):
        self.func_begin()
        image = b'1234'
        face_list = []
        self.handler.img_data = b'12'
        self.handler.save_frame(image, face_list)
        self.func_end()


    def test_save_frame2(self):
        self.func_begin()
        image = b'1234'
        face_list = []
        self.handler.img_data = b'12'
        self.run_thread(self.set_img_data_none)
        self.handler.save_frame(image, face_list)
        self.func_end()


    @patch('threading.Event.clear', return_value = True)
    @patch('threading.Event.wait', return_value = True)
    def test_frames(self, mock1, mock2):        
        self.func_begin()
        self.handler.close_thread_switch = True
        time.sleep(0.5) # wait thread exit
        self.handler.img_data = None
        backup_heartbeat = facial_recognition_handler.HEARTBEAT_TIMEOUT
        facial_recognition_handler.HEARTBEAT_TIMEOUT = 0
        self.handler.close_thread_switch = False
        for frame in self.handler.frames():
            self.assertEqual(frame, None)
            break
        facial_recognition_handler.HEARTBEAT_TIMEOUT = backup_heartbeat
        self.func_end()

    @patch('threading.Event.clear', return_value = True)
    @patch('threading.Event.wait')
    def test_get_frame(self, mock1, mock2):
        self.func_begin()
        self.handler.frame_data = b'123'
        self.handler.face_list = []
        self.handler.fps = 5
        mock1.return_value = True
        ret = self.handler.get_frame()
        self.assertNotEqual(ret, {})

        mock1.return_value = False
        ret = self.handler.get_frame()
        self.assertEqual(ret, {})
        mock1.return_value = True
        self.func_end()

if __name__ == '__main__':
    # unittest.main()
    suite = unittest.TestSuite()
    suite.addTest(TestFacialRecognitionHandler("test_frames"))
    runner = unittest.TextTestRunner()
    runner.run(suite)