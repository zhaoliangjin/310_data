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
import random
import time
import json
import threading
import unittest
import select
import socket
from unittest.mock import patch
import struct
import shutil
from json.decoder import JSONDecodeError

path = os.path.dirname(__file__)
index = path.rfind("ascenddk")
workspace = path[0: index]
path = os.path.join(workspace, "ascenddk/common/presenter/server")
sys.path.append(path)

import common.channel_manager as channel_manager
import common.channel_handler as channel_handler
import common.presenter_message_pb2 as pb
import video_analysis.src.video_analysis_message_pb2  as video_pb
from google.protobuf.message import DecodeError 
import video_analysis.src.video_analysis_server as video_analysis_server
from video_analysis.src.video_analysis_server import VideoAnalysisManager
from video_analysis.src.config_parser import ConfigParser

HOST = "127.127.0.1"
STORAGE_DIR = "/var/lib/presenter/video_analysis"
PORT_BEGIN = 20000
CLIENT_SOCK = None
RUN_FLAG = True
SOCK_RECV_NULL = b''
APP_NAME = "video_analysis"
CHANNEL_ID = "channel_2"
CHANNEL_NAME = "xxx.mp4"
FRAME_ID = "8888"
IMG = b'xxx'
HUMAN_PROPERTY_LIST = ["Age16-30", "Age31-45", "Age46-60", "AgeAbove61", "Backpack", "CarryingOther", "Casual lower", "Casual upper", "Formal lower", "Formal upper", "Hat", "Jacket", "Jeans", "Leather Shoes", "Logo", "Long hair", "Male", "Messenger Bag", "Muffler", "No accessory", "No carrying", "Plaid", "PlasticBags", "Sandals", "Shoes", "Shorts", "Short Sleeve", "Skirt", "Sneaker", "Stripes", "Sunglasses", "Trousers", "Tshirt", "UpperOther", "V-Neck"]
def mock_join_exp(a, b):
    raise OSError 

def mock_dump_exp(a, b):
    raise OSError

def mock_oserr(a):
    raise OSError    

def mock_parse_proto_exp(a):
    raise DecodeError

def read_socket(conn, read_len):
    has_read_len = 0
    read_buf = SOCK_RECV_NULL
    total_buf = SOCK_RECV_NULL
    while has_read_len != read_len:
        try:
            read_buf = conn.recv(read_len - has_read_len)
        except socket.error:
            return False, None
        if read_buf == SOCK_RECV_NULL:
            return False, None
        total_buf += read_buf
        has_read_len = len(total_buf)

    return True, total_buf

def protobuf_register_app():
    app = video_pb.RegisterApp()
    app.id = APP_NAME
    app.type = APP_NAME
    return app.SerializeToString()

def protobuf_heartbeat():
    heartbeat = pb.HeartbeatMessage()
    return heartbeat.SerializeToString()

def send_message(message_name, proto_data):
    message_name_size = len(message_name)
    msg_data = proto_data
    msg_data_size = len(msg_data)
    message_total_size = 5 + message_name_size + msg_data_size
    message_head = (socket.htonl(message_total_size), message_name_size)
    s = struct.Struct('IB')
    packed_data = s.pack(*message_head)
    bytes(message_name, encoding="utf-8")
    message_data = packed_data + bytes(message_name, encoding="utf-8") + msg_data 
    CLIENT_SOCK.sendall(message_data[:10])
    CLIENT_SOCK.sendall(message_data[10:])

def register_app():
    global CLIENT_SOCK
    server_addr = ("127.0.0.1", 7004)
    CLIENT_SOCK = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    CLIENT_SOCK.connect(server_addr)
    CLIENT_SOCK.setblocking(1)
    send_message(video_pb._REGISTERAPP.full_name, protobuf_register_app())
    send_message(pb._HEARTBEATMESSAGE.full_name, protobuf_heartbeat())

def response_register_app():
    message_head = CLIENT_SOCK.recv(5)
    s = struct.Struct('Ib')
    (message_len, messagename_len) = s.unpack(message_head)
    message_len = socket.ntohl(message_len)
    message_name = CLIENT_SOCK.recv(messagename_len)
    message_body = CLIENT_SOCK.recv(message_len -5 -messagename_len)
    response = video_pb.CommonResponse()
    response.ParseFromString(message_body)
    if response.ret == video_pb.kErrorNone:
        return True
    return False

def protobuf_open_channel(channel_name):
    open_channel = pb.OpenChannelRequest()
    open_channel.channel_name = channel_name
    open_channel.content_type = pb.kChannelContentTypeVideo
    return open_channel.SerializeToString()

def protobuf_image_request(frame_id):
    send_request = video_pb.ImageSet()
    send_request.frame_image = IMG
    send_request.frame_index.app_id = APP_NAME
    send_request.frame_index.channel_id = CHANNEL_ID
    send_request.frame_index.channel_name = CHANNEL_NAME
    send_request.frame_index.frame_id = frame_id

    object = send_request.object.add()
    object.id = "car_1"
    object.confidence = 0.95
    object.image = IMG
    
    object = send_request.object.add()
    object.id = "car_2"
    object.confidence = 0.95
    object.image = IMG
    
    object = send_request.object.add()
    object.id = "bus_1"
    object.confidence = 0.95
    object.image = IMG

    object = send_request.object.add()
    object.id = "person_1"
    object.confidence = 0.95
    object.image = IMG
    
    return send_request.SerializeToString()
    
def protobuf_car_color_inference(object_id, frame_id):
    send_request = video_pb.CarInferenceResult()
    send_request.frame_index.app_id = APP_NAME
    send_request.frame_index.channel_id = CHANNEL_ID
    send_request.frame_index.channel_name = CHANNEL_NAME
    send_request.frame_index.frame_id = frame_id
    send_request.object_id = object_id
    send_request.type = video_pb.kCarColor
    send_request.confidence = 0.8
    send_request.value = "blue"

    return send_request.SerializeToString()

def protobuf_car_brand_inference(object_id, frame_id):
    send_request = video_pb.CarInferenceResult()
    send_request.frame_index.app_id = APP_NAME
    send_request.frame_index.channel_id = CHANNEL_ID
    send_request.frame_index.channel_name = CHANNEL_NAME
    send_request.frame_index.frame_id = frame_id
    send_request.object_id = object_id
    send_request.type = video_pb.kCarBrand
    send_request.confidence = 0.8
    send_request.value = "BMW"
  
    return send_request.SerializeToString()
    
def protobuf_human_property_inference(object_id, frame_id):
    send_request = video_pb.HumanInferenceResult()
    send_request.frame_index.app_id = APP_NAME
    send_request.frame_index.channel_id = CHANNEL_ID
    send_request.frame_index.channel_name = CHANNEL_NAME
    send_request.frame_index.frame_id = frame_id
    send_request.object_id = object_id

    for i in HUMAN_PROPERTY_LIST:
        human_property = send_request.human_property.add()
        human_property.key = i
        human_property.value = 0.8
  
    return send_request.SerializeToString()

def send_image_set(frame_id):
    message_name = video_pb._IMAGESET.full_name
    image_data = protobuf_image_request(frame_id)
    send_message(message_name, image_data)
   

def send_msg(msg_name, func, arg1, arg2):
    image_data = func(arg1, arg2)
    send_message(msg_name, image_data)

def send_frame(frame_id):
    send_image_set(frame_id)
    send_msg(video_pb._CARINFERENCERESULT.full_name, protobuf_car_color_inference, "car_1", frame_id)
    send_msg(video_pb._CARINFERENCERESULT.full_name, protobuf_car_brand_inference, "car_1", frame_id)
    send_msg(video_pb._CARINFERENCERESULT.full_name, protobuf_car_color_inference, "car_2", frame_id)
    send_msg(video_pb._CARINFERENCERESULT.full_name, protobuf_car_brand_inference, "car_2", frame_id)
    send_msg(video_pb._CARINFERENCERESULT.full_name, protobuf_car_color_inference, "bus_1", frame_id)
    send_msg(video_pb._CARINFERENCERESULT.full_name, protobuf_car_brand_inference, "bus_1", frame_id)
    send_msg(video_pb._CARINFERENCERESULT.full_name, protobuf_car_color_inference, "bus_2", frame_id)
    send_msg(video_pb._CARINFERENCERESULT.full_name, protobuf_car_brand_inference, "bus_2", frame_id)
    send_msg(video_pb._HUMANINFERENCERESULT.full_name, protobuf_human_property_inference, "person_1", frame_id)

def thread(func):
    thread = threading.Thread(target=func)
    thread.start()

def client_server():
    register_app()
    while RUN_FLAG:
        try:
            time.sleep(2)
        except:
            break

class Test_VideoAnalysisServer(unittest.TestCase):
    """Test_VideoAnalysisServer"""
    manager = None
    server = None
    def tearDown(self):
        pass

    def setUp(self):
        pass

    @classmethod
    def tearDownClass(cls):
        try:
            send_message("unkown message", protobuf_heartbeat())
            time.sleep(0.5)
        except:
            pass
        global RUN_FLAG
        RUN_FLAG = False
        if os.path.exists(STORAGE_DIR):
            shutil.rmtree(STORAGE_DIR, ignore_errors=True)
        server = Test_VideoAnalysisServer.server
        server.stop_thread()

    @classmethod
    def setUpClass(cls):
        if os.path.exists(STORAGE_DIR):
            shutil.rmtree(STORAGE_DIR, ignore_errors=True)
        os.makedirs(STORAGE_DIR)
        Test_VideoAnalysisServer.server = video_analysis_server.run()
        Test_VideoAnalysisServer.manager = VideoAnalysisManager()

        # register app
        thread(client_server)
        # wait for app register
        time.sleep(1)
        ret = response_register_app()
        if not ret:
            raise Exception
        send_frame(FRAME_ID)
        time.sleep(0.1)

    def test_get_app_list(self):
        manager = Test_VideoAnalysisServer.manager
        ret = manager.get_app_list()
        self.assertEqual(ret, [APP_NAME])

    @patch('os.path.isdir', return_value=False)
    def test_get_app_list_null(self, mock):
        manager = Test_VideoAnalysisServer.manager
        ret = manager.get_app_list()
        self.assertEqual(ret, [])

    def test_get_channel_list(self):
        manager = Test_VideoAnalysisServer.manager
        ret = manager.get_channel_list(APP_NAME)
        self.assertEqual(ret, [CHANNEL_ID])

        ret = manager.get_channel_list(123)
        self.assertEqual(ret, [])

        ret = manager.get_channel_list("123")
        self.assertEqual(ret, [])

    def test_get_frame_number(self):
        manager = Test_VideoAnalysisServer.manager
        ret = manager.get_frame_number(APP_NAME, CHANNEL_ID)
        self.assertEqual(ret, 1)

        ret = manager.get_frame_number(APP_NAME, "123")
        self.assertEqual(ret, 0)


    def test_get_channel_name(self):
        manager = Test_VideoAnalysisServer.manager
        ret = manager.get_channel_name(APP_NAME, CHANNEL_ID)
        self.assertEqual(ret, CHANNEL_NAME)

        ret = manager.get_channel_name("123", CHANNEL_ID)
        self.assertEqual(ret, None)

        ret = manager.get_channel_name(APP_NAME, "123")
        self.assertEqual(ret, None)

    @patch('os.path.isfile', side_effect=mock_oserr)
    def test_get_channel_name_exp(self, mock):
        manager = Test_VideoAnalysisServer.manager
        ret = manager.get_channel_name(APP_NAME, CHANNEL_ID)
        self.assertEqual(ret, None)


    def test_present_frame(self):
        manager = Test_VideoAnalysisServer.manager
        ret = manager.present_frame(APP_NAME, CHANNEL_ID)
        self.assertNotEqual(ret, None)

        ret = manager.present_frame(APP_NAME, CHANNEL_ID, "0")
        self.assertNotEqual(ret, None)

        ret = manager.present_frame(APP_NAME, "123")
        self.assertEqual(ret, None)

        ret = manager.present_frame(APP_NAME, CHANNEL_ID, "1")
        self.assertEqual(ret, None)

    def test_present_frame_exp(self):
        manager = Test_VideoAnalysisServer.manager
        backup = video_analysis_server.IMAGE_FILE
        video_analysis_server.IMAGE_FILE = "123"
        ret = manager.present_frame(APP_NAME, CHANNEL_ID)
        self.assertEqual(ret, None)
        video_analysis_server.IMAGE_FILE = backup

    @patch('os.path.exists')
    def test_extract_vehicle_info_exp(self, mock):
        manager = Test_VideoAnalysisServer.manager
        stack_dir = os.path.join(manager.storage_dir, APP_NAME, CHANNEL_ID, "stack_0") 
        frame_dir = os.path.join(stack_dir, FRAME_ID)
        vehicle_id = "car_1"
        mock.return_value = False
        ret = manager._extract_vehicle_info(frame_dir, vehicle_id)

        mock.side_effect = mock_oserr
        ret = manager._extract_vehicle_info(frame_dir, vehicle_id)
        self.assertEqual(ret, None)


    @patch('os.path.isfile', return_value=True)
    @patch('os.path.exists')
    def test_extract_human_property_info_exp(self, mock, mock1):
        manager = Test_VideoAnalysisServer.manager
        stack_dir = os.path.join(manager.storage_dir, APP_NAME, CHANNEL_ID, "stack_0") 
        frame_dir = os.path.join(stack_dir, FRAME_ID)
        vehicle_id = "person_1"
        mock.return_value = False
        ret = manager._extract_human_property_info(frame_dir, vehicle_id)

        mock.side_effect = mock_oserr
        ret = manager._extract_human_property_info(frame_dir, vehicle_id)
        self.assertEqual(ret, None)

        mock1.return_value = False
        ret = manager._extract_human_property_info(frame_dir, vehicle_id)
        self.assertEqual(ret, None)

    @patch("google.protobuf.message.Message.ParseFromString")
    def test_parse_protobuf(self, mock):
        server = Test_VideoAnalysisServer.server
        mock.side_effect = mock_parse_proto_exp
        request = video_pb.RegisterApp() 
        ret = server._parse_protobuf(request, b'xxx')
        self.assertEqual(ret, False)

    @patch("video_analysis.src.video_analysis_server.VideoAnalysisServer._parse_protobuf")
    @patch("video_analysis.src.video_analysis_server.VideoAnalysisServer.send_message", return_value=True)
    def test_process_register_app_fail(self, mock1, mock2):
        server = Test_VideoAnalysisServer.server
        def parse_protobuf(request, msg_data):
            return False

        def _parse_protobuf_1(request, msg_data):
            request.id = APP_NAME
            request.type = APP_NAME
            return True

        def _parse_protobuf_2(request, msg_data):
            request.id = "new_app"
            request.type = "invalid"
            return True

        def _parse_protobuf_3(request, msg_data):
            request.id = "aaaaaaaaaaaaaaaaaaaaaaaaaa"
            request.type = APP_NAME
            return True

        def _parse_protobuf_4(request, msg_data):
            request.id = "new_app"
            request.type = APP_NAME
            return True


        mock2.side_effect = parse_protobuf
        ret = server._process_register_app(None, b'xxx')
        self.assertEqual(ret, False)

        mock2.side_effect = _parse_protobuf_1
        ret = server._process_register_app(None, b'xxx')
        self.assertEqual(ret, False)

        mock2.side_effect = _parse_protobuf_2
        ret = server._process_register_app(None, b'xxx')
        self.assertEqual(ret, False)

        mock2.side_effect = _parse_protobuf_3
        ret = server._process_register_app(None, b'xxx')
        self.assertEqual(ret, False)

        back_up = server.max_app_num
        server.max_app_num = 0
        mock2.side_effect = _parse_protobuf_3
        ret = server._process_register_app(None, b'xxx')
        self.assertEqual(ret, False)
        server.max_app_num = back_up

        back_up = server.reserved_space
        server.reserved_space = 10000000000000
        mock2.side_effect = _parse_protobuf_4
        ret = server._process_register_app(None, b'xxx')
        self.assertEqual(ret, False)
        server.reserved_space = back_up

    @patch("video_analysis.src.video_analysis_server.VideoAnalysisServer._save_inference_result")
    @patch("video_analysis.src.video_analysis_server.VideoAnalysisServer._save_image")
    @patch("video_analysis.src.video_analysis_server.VideoAnalysisServer._parse_protobuf")
    @patch("video_analysis.src.video_analysis_server.VideoAnalysisServer.send_message", return_value=True)
    def test_process_image_set_fail(self, mock1, mock2, mock3, mock4):
        server = Test_VideoAnalysisServer.server
        def parse_protobuf(request, msg_data):
            return False

        def _parse_protobuf_1(request, msg_data):
            request.frame_image = IMG
            request.frame_index.app_id = "nonexist_app"
            return True

        def _parse_protobuf_2(request, msg_data):
            request.frame_image = IMG
            request.frame_index.app_id = APP_NAME
            request.frame_index.channel_id = CHANNEL_ID
            request.frame_index.channel_name = CHANNEL_NAME
            request.frame_index.frame_id = FRAME_ID

            object = request.object.add()
            object.id = "car_1"
            object.confidence = 0.95
            object.image = IMG
            
            object = request.object.add()
            object.id = "car_2"
            object.confidence = 0.95
            object.image = IMG
            
            object = request.object.add()
            object.id = "bus_1"
            object.confidence = 0.95
            object.image = IMG

            object = request.object.add()
            object.id = "person_1"
            object.confidence = 0.95
            object.image = IMG
            return True

        mock2.side_effect = parse_protobuf
        ret = server._process_image_set(None, b'xxx')
        self.assertEqual(ret, False)

        mock2.side_effect = _parse_protobuf_1
        ret = server._process_image_set(None, b'xxx')
        self.assertEqual(ret, False)

        mock2.side_effect = _parse_protobuf_2
        backup_interval = video_analysis_server.CHECK_INTERCAL
        backup_reserve_space = server.reserved_space
        video_analysis_server.CHECK_INTERCAL = 1
        server.reserved_space = 1000000000000
        ret = server._process_image_set(None, b'xxx')
        self.assertEqual(ret, False)
        video_analysis_server.CHECK_INTERCAL = backup_interval
        server.reserved_space = backup_reserve_space

        mock2.side_effect = _parse_protobuf_2
        mock3.return_value = False
        ret = server._process_image_set(None, b'xxx')
        self.assertEqual(ret, False)

        mock2.side_effect = _parse_protobuf_2
        mock3.return_value = True
        mock4.return_value = False
        ret = server._process_image_set(None, b'xxx')
        self.assertEqual(ret, False)

    @patch("video_analysis.src.video_analysis_server.VideoAnalysisServer._save_inference_result")
    @patch("video_analysis.src.video_analysis_server.VideoAnalysisServer._parse_protobuf")
    @patch("video_analysis.src.video_analysis_server.VideoAnalysisServer.send_message", return_value=True)
    def test_process_car_inference_result_fail(self, mock1, mock2, mock3):
        server = Test_VideoAnalysisServer.server
        object_id = "car_1"
        def parse_protobuf(request, msg_data):
            return False

        def _parse_protobuf_1(request, msg_data):
            request.frame_index.app_id = "nonexist_app"
            return True

        def _parse_protobuf_2(request, msg_data):
            request.frame_index.app_id = APP_NAME
            request.frame_index.channel_id = CHANNEL_ID
            request.frame_index.channel_name = CHANNEL_NAME
            request.frame_index.frame_id = FRAME_ID
            request.object_id = object_id
            request.type = 100
            request.confidence = 0.8
            request.value = "blue"
            return True

        def _parse_protobuf_3(request, msg_data):
            request.frame_index.app_id = APP_NAME
            request.frame_index.channel_id = CHANNEL_ID
            request.frame_index.channel_name = CHANNEL_NAME
            request.frame_index.frame_id = FRAME_ID
            request.object_id = object_id
            request.type = video_pb.kCarColor
            request.confidence = 0.8
            request.value = "blue"
            return True

        mock2.side_effect = parse_protobuf
        ret = server._process_car_inference_result(None, b'xxx')
        self.assertEqual(ret, False)


        mock2.side_effect = _parse_protobuf_1
        ret = server._process_car_inference_result(None, b'xxx')
        self.assertEqual(ret, False)

        mock2.side_effect = _parse_protobuf_2
        ret = server._process_car_inference_result(None, b'xxx')
        self.assertEqual(ret, False)

        mock2.side_effect = _parse_protobuf_3
        mock3.return_value = False
        ret = server._process_car_inference_result(None, b'xxx')
        self.assertEqual(ret, False)

    @patch("video_analysis.src.video_analysis_server.VideoAnalysisServer._save_inference_result")
    @patch("video_analysis.src.video_analysis_server.VideoAnalysisServer._parse_protobuf")
    @patch("video_analysis.src.video_analysis_server.VideoAnalysisServer.send_message", return_value=True)
    def test_process_human_inference_result_fail(self, mock1, mock2, mock3):
        server = Test_VideoAnalysisServer.server
        object_id = "person_1"
        def parse_protobuf(request, msg_data):
            return False

        def _parse_protobuf_1(request, msg_data):
            request.frame_index.app_id = "nonexist_app"
            return True

        def _parse_protobuf_2(request, msg_data):
            request.frame_index.app_id = APP_NAME
            request.frame_index.channel_id = CHANNEL_ID
            request.frame_index.channel_name = CHANNEL_NAME
            request.frame_index.frame_id = FRAME_ID
            request.object_id = object_id

            for i in HUMAN_PROPERTY_LIST:
                human_property = request.human_property.add()
                human_property.key = i
                human_property.value = 0.8
            return True

        mock2.side_effect = parse_protobuf
        ret = server._process_human_inference_result(None, b'xxx')
        self.assertEqual(ret, False)


        mock2.side_effect = _parse_protobuf_1
        ret = server._process_human_inference_result(None, b'xxx')
        self.assertEqual(ret, False)

        mock2.side_effect = _parse_protobuf_2
        mock3.return_value = False
        ret = server._process_human_inference_result(None, b'xxx')
        self.assertEqual(ret, False)

    @patch("os.path.isdir", side_effect=mock_oserr)
    def test_save_image_fail(self, mock):
        server = Test_VideoAnalysisServer.server
        ret = server._save_image("/", b'xxx')
        self.assertEqual(ret, False)

    @patch("os.path.isfile", side_effect=mock_oserr)
    def test_save_channel_name_fail(self, mock):
        server = Test_VideoAnalysisServer.server
        ret = server._save_channel_name("/", None, None)
        self.assertEqual(ret, False)

    @patch("os.path.isdir", side_effect=mock_oserr)
    def test_save_inference_result_fail(self, mock):
        server = Test_VideoAnalysisServer.server
        ret = server._save_inference_result("/", None)
        self.assertEqual(ret, False)

    @patch("video_analysis.src.config_parser.ConfigParser.config_verify", return_value=False)
    def test_run_fail(self, mock):
        ret = video_analysis_server.run()
        self.assertEqual(ret, None)

    @patch('common.app_manager.AppManager.get_socket_by_app_id', return_value=True)
    @patch('common.app_manager.AppManager.unregister_app_by_fd', return_value=True)
    @patch("shutil.rmtree", return_value=True)
    def test_clean_dir(self, mock1, mock2, mock3):
        manager = Test_VideoAnalysisServer.manager
        manager.clean_dir(123)
        manager.clean_dir(APP_NAME, CHANNEL_ID)
        manager.clean_dir("123")
        manager.clean_dir(APP_NAME)

    @patch('common.app_manager.AppManager.get_socket_by_app_id', side_effect=mock_oserr)
    def test_clean_dir_exp(self, mock):
        manager = Test_VideoAnalysisServer.manager
        manager.clean_dir(APP_NAME, CHANNEL_ID)

if __name__ == '__main__':
    # unittest.main()
    suite = unittest.TestSuite()
    suite.addTest(Test_VideoAnalysisServer("test_get_app_list"))
    runner = unittest.TextTestRunner()
    runner.run(suite)
