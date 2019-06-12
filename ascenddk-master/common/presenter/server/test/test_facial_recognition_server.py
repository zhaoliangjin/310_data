"""utest presenter socket server module"""

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
import facial_recognition.src.facial_recognition_message_pb2  as facial_pb
from google.protobuf.message import DecodeError 
import facial_recognition.src.facial_recognition_server as facial_recognition_server
from facial_recognition.src.facial_recognition_server import FacialRecognitionManager
from facial_recognition.src.config_parser import ConfigParser

HOST = "127.127.0.1"
STORAGE_DIR = "/var/lib/presenter/facial_recognition"
PORT_BEGIN = 20000
CLIENT_SOCK = None
RUN_FLAG = True
SOCK_RECV_NULL = b''
APP_NAME = "facial_recognition"

def mock_join_exp(a, b):
    raise OSError 

def mock_dump_exp(a, b):
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
    app = facial_pb.RegisterApp()
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
    server_addr = ("127.0.0.1", 7008)
    CLIENT_SOCK = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    CLIENT_SOCK.connect(server_addr)
    CLIENT_SOCK.setblocking(1)
    send_message(facial_pb._REGISTERAPP.full_name, protobuf_register_app())

def protobuf_image_request():
    send_request = facial_pb.FrameInfo()
    send_request.image = b'xxx'

    for i in range(1):
        face = send_request.feature.add()
        face.box.lt_x = 1
        face.box.lt_y = 1
        face.box.rb_x = 1
        face.box.rb_y = 1
        for j in range(1024):
            v = random.randint(0, 100)
            face.vector.append(v)
    
    return send_request.SerializeToString()

def protobuf_open_channel(channel_name):
    open_channel = pb.OpenChannelRequest()
    open_channel.channel_name = channel_name
    open_channel.content_type = pb.kChannelContentTypeVideo
    return open_channel.SerializeToString()

def face_feature(face_id):
    response = facial_pb.FaceResult()
    response.id = face_id
    response.response.ret = facial_pb.kErrorNone
    response.response.message = "succeed"
    for i in range(1):
        face = response.feature.add()
        face.box.lt_x = 1
        face.box.lt_y = 1
        face.box.rb_x = 1
        face.box.rb_y = 1
        for j in range(1024):
            face.vector.append(100)
    return response.SerializeToString()

def process_face_register(message_body):
    face_info = facial_pb.FaceInfo()
    face_info.ParseFromString(message_body)
    face_id = face_info.id
    send_message(facial_pb._FACERESULT.full_name, face_feature(face_id))

def process_message():
    message_head = CLIENT_SOCK.recv(5)
    s = struct.Struct('IB')
    (message_len, messagename_len) = s.unpack(message_head)
    message_len = socket.ntohl(message_len)
    message_name = CLIENT_SOCK.recv(messagename_len)
    message_name = message_name.decode("utf-8")
    read_len = message_len -5 -messagename_len
    ret, message_body = read_socket(CLIENT_SOCK, read_len)
    if not ret:
        return
    if message_name == facial_pb._FACEINFO.full_name:
        process_face_register(message_body)

def thread(func):
    thread = threading.Thread(target=func)
    thread.start()

def client_server():
    register_app()
    while RUN_FLAG:
        send_message(pb._HEARTBEATMESSAGE.full_name, protobuf_heartbeat())
        process_message()
        time.sleep(1)

class Test_FacialRecognitionServer(unittest.TestCase):
    """Test_FacialRecognitionServer"""
    manager = None
    server = None
    def tearDown(self):
        pass

    def setUp(self):
        pass

    @classmethod
    def tearDownClass(cls):
        send_message("unkown message", protobuf_heartbeat())
        global RUN_FLAG
        RUN_FLAG = False
        if os.path.exists(STORAGE_DIR):
            shutil.rmtree(STORAGE_DIR, ignore_errors=True)
        server = Test_FacialRecognitionServer.server
        server.stop_thread()

    @classmethod
    def setUpClass(cls):
        if os.path.exists(STORAGE_DIR):
            shutil.rmtree(STORAGE_DIR, ignore_errors=True)
        os.makedirs(STORAGE_DIR)
        Test_FacialRecognitionServer.server = facial_recognition_server.run()
        Test_FacialRecognitionServer.manager = FacialRecognitionManager()

        @patch("os.path.isfile", return_value=False)
        def test_init_face_database(mock):
            server = Test_FacialRecognitionServer.server
            server._init_face_database()

        test_init_face_database()

        # register app
        thread(client_server)

    def test_get_app_list(self):
        manager = Test_FacialRecognitionServer.manager
        time.sleep(0.5)
        ret = manager.get_app_list()
        self.assertEqual(ret, [APP_NAME])

    @patch('facial_recognition.src.facial_recognition_server.FacialRecognitionServer')
    def test_register_face_fail(self, mock):
        manager = Test_FacialRecognitionServer.manager
        ori_server = manager.server
        manager.server = mock.return_value

        name = 1
        image = b'abc'
        (ret, msg) = manager.register_face(name, image)
        self.assertEqual(ret, False)

        name = "face1"
        image = 2
        (ret, msg) = manager.register_face(name, image)
        self.assertEqual(ret, False)

        name = "face1"
        image = b'data'
        manager.server.max_face_num = 0
        (ret, msg) = manager.register_face(name, image)
        self.assertEqual(ret, False)
        manager.server.max_face_num = 100

        name = "face1"
        image = b'data'
        manager.server.list_registered_apps.return_value = None
        (ret, msg) = manager.register_face(name, image)
        self.assertEqual(ret, False)
        manager.server.list_registered_apps.return_value = ["a"]

        name = "face1"
        image = b'data'
        manager.server.get_app_socket.return_value = None
        (ret, msg) = manager.register_face(name, image)
        self.assertEqual(ret, False)
        manager.server.get_app_socket.return_value = "sock"

        manager.server = ori_server
        name = "face1"
        image = b'data'
        facial_recognition_server.FACE_REGISTER_STATUS_WAITING = 2
        (ret, msg) = manager.register_face(name, image)
        self.assertEqual(ret, False)
        facial_recognition_server.FACE_REGISTER_STATUS_WAITING = 1

        name = "face1"
        image = b'data'
        facial_recognition_server.FACE_REGISTER_STATUS_FAILED = 2
        (ret, msg) = manager.register_face(name, image)
        self.assertEqual(ret, False)
        facial_recognition_server.FACE_REGISTER_STATUS_FAILED = 3

        manager.server = mock.return_value
        name = "face1"
        image = b'data'
        manager.server.save_face_image.return_value = False
        (ret, msg) = manager.register_face(name, image)
        self.assertEqual(ret, False)
        manager.server = ori_server

    def test_unregister_face(self):
        # register a face
        server = Test_FacialRecognitionServer.server
        manager = Test_FacialRecognitionServer.manager
        name = "face1"
        image = b'data'
        (ret, msg) = manager.register_face(name, image)
        self.assertEqual(ret, True)

        @patch('json.dump')
        def test_delete_faces(mock_dump):
            mock_dump.side_effect = mock_dump_exp
            ret = server.delete_faces([name])
            self.assertEqual(ret, False)

        test_delete_faces()
        # open channel to send a frame
        send_message(pb._OPENCHANNELREQUEST.full_name, protobuf_open_channel(APP_NAME))
        time.sleep(0.5) # wait 0.5sec for message processing
        send_message(facial_pb._FRAMEINFO.full_name, protobuf_image_request())
        time.sleep(0.5) # wait 0.5sec for message processing

        # unregister face
        ret = manager.unregister_face([name])
        self.assertEqual(ret, True)

        ret = manager.unregister_face(name)
        self.assertEqual(ret, False)


    def test_get_faces(self):
        # register a face
        manager = Test_FacialRecognitionServer.manager
        name = "face2"
        image = b'data'
        (ret, msg) = manager.register_face(name, image)
        self.assertEqual(ret, True)

        ret = manager.get_faces(name)
        self.assertEqual(ret, [])

        ret = manager.get_faces([name])
        self.assertNotEqual(ret, [])

        # unregister face
        ret = manager.unregister_face([name])
        self.assertEqual(ret, True)

    
    def test_get_faces_with_oserror(self):
        # register a face
        manager = Test_FacialRecognitionServer.manager
        name = "face2"
        image = b'data'
        (ret, msg) = manager.register_face(name, image)
        self.assertEqual(ret, True)

        @patch('os.path.join')
        def test_get_faces(mock_join):
            mock_join.side_effect = mock_join_exp
            ret = manager.get_faces([name])
            self.assertEqual(ret, [])
        
        test_get_faces()
        # unregister face
        ret = manager.unregister_face([name])
        self.assertEqual(ret, True)

    @patch("os.path.isfile")
    def test_filter_registration_data(self, mock):
        server = Test_FacialRecognitionServer.server
        ori_registered_faces = server.registered_faces
        server.registered_faces = {"face_name":"face_data"}
        mock.return_value = False
        server._filter_registration_data()
        server.registered_faces = ori_registered_faces

    @patch("os.path.join")
    def test_save_face_image(self, mock):
        server = Test_FacialRecognitionServer.server
        mock.return_value = None
        ret = server.save_face_image("face_name", "face_data")
        self.assertEqual(ret, False)

    """utest"""
    # @patch("os.path.isfile", return_value=False)
    # def test_parse_protobuf(self, mock):
    #     server = Test_FacialRecognitionServer.server
    #     mock.side_effect = mock_parse_proto_exp
    #     request = facial_pb.RegisterApp() 
    #     ret = server._init_face_database(request, b'xxx')
    #     self.assertEqual(ret, False)

    @patch("google.protobuf.message.Message.ParseFromString")
    def test_parse_protobuf(self, mock):
        server = Test_FacialRecognitionServer.server
        mock.side_effect = mock_parse_proto_exp
        request = facial_pb.RegisterApp() 
        ret = server._parse_protobuf(request, b'xxx')
        self.assertEqual(ret, False)

    @patch("facial_recognition.src.facial_recognition_server.FacialRecognitionServer._parse_protobuf")
    @patch("facial_recognition.src.facial_recognition_server.FacialRecognitionServer.send_message", return_value=True)
    def test_process_register_app_fail(self, mock1, mock2):
        server = Test_FacialRecognitionServer.server
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

        back_up = facial_recognition_server.MAX_APP_NUM
        facial_recognition_server.MAX_APP_NUM = 0
        mock2.side_effect = _parse_protobuf_3
        ret = server._process_register_app(None, b'xxx')
        self.assertEqual(ret, False)
        facial_recognition_server.MAX_APP_NUM = back_up

    @patch("facial_recognition.src.facial_recognition_server.FacialRecognitionServer._update_register_dict", return_value=True)
    @patch("facial_recognition.src.facial_recognition_server.FacialRecognitionServer._parse_protobuf")
    def test_process_face_result_fail(self, mock1, mock2):
        server = Test_FacialRecognitionServer.server
        face_id = "face_test"
        def parse_protobuf(request, msg_data):
            return False

        def _parse_protobuf_1(request, msg_data):
            request.id = face_id
            return True

        def _parse_protobuf_2(request, msg_data):
            request.id = face_id
            request.response.ret = facial_pb.kErrorOther
            return True

        def _parse_protobuf_3(request, msg_data):
            request.id = face_id
            request.response.ret = facial_pb.kErrorNone
            request.response.message = "succeed"
            for i in range(2):
                face = request.feature.add()
                face.box.lt_x = 1
                face.box.lt_y = 1
                face.box.rb_x = 1
                face.box.rb_y = 1
                for j in range(1024):
                    face.vector.append(100)
            return True

        def _parse_protobuf_4(request, msg_data):
            request.id = face_id
            request.response.ret = facial_pb.kErrorNone
            request.response.message = "succeed"
            for i in range(1):
                face = request.feature.add()
                face.box.lt_x = 1
                face.box.lt_y = 1
                face.box.rb_x = 1
                face.box.rb_y = 1
                for j in range(100):
                    face.vector.append(100)
            return True

        mock1.side_effect = parse_protobuf
        ret = server._process_face_result(b'xxx')
        self.assertEqual(ret, False)

        mock1.side_effect = _parse_protobuf_1
        ret = server._process_face_result(b'xxx')
        self.assertEqual(ret, True)

        server.register_dict[face_id] = {
                                            "status":"",
                                            "message":"",
                                            "event":""
                                        }
        mock1.side_effect = _parse_protobuf_2
        ret = server._process_face_result(b'xxx')
        self.assertEqual(ret, True)

        mock1.side_effect = _parse_protobuf_1
        ret = server._process_face_result(b'xxx')
        self.assertEqual(ret, True)

        mock1.side_effect = _parse_protobuf_3
        ret = server._process_face_result(b'xxx')
        self.assertEqual(ret, True)

        mock1.side_effect = _parse_protobuf_4
        ret = server._process_face_result(b'xxx')
        self.assertEqual(ret, True)

        del server.register_dict[face_id]


    def test_save_face_feature_fail(self):
        server = Test_FacialRecognitionServer.server

        face_id = "face_test"
        face_coordinate = None
        feature_vector = None
        back_up = server.face_register_file
        server.face_register_file = 123

        ret = server._save_face_feature(face_id, face_coordinate, feature_vector)
        self.assertEqual(ret, False)
        server.face_register_file = back_up

    @patch("common.channel_manager.ChannelManager.is_channel_busy")
    @patch("common.channel_manager.ChannelManager.is_channel_exist")
    @patch("facial_recognition.src.facial_recognition_server.FacialRecognitionServer._parse_protobuf")
    @patch("facial_recognition.src.facial_recognition_server.FacialRecognitionServer._response_open_channel", return_value=True)
    def test_process_open_channel_fail(self, mock1, mock2, mock3, mock4):
        server = Test_FacialRecognitionServer.server
        def parse_protobuf(request, msg_data):
            return False

        def _parse_protobuf_1(request, msg_data):
            return True

        def _parse_protobuf_2(request, msg_data):
            request.content_type = pb.kChannelContentTypeImage
            return True

        mock2.side_effect = parse_protobuf
        ret = server._process_open_channel(None, b'xxx')
        self.assertEqual(ret, True)

        mock3.return_value = False
        mock2.side_effect = _parse_protobuf_1
        ret = server._process_open_channel(None, b'xxx')
        self.assertEqual(ret, True)

        mock3.return_value = True
        mock4.return_value = True
        mock2.side_effect = _parse_protobuf_1
        ret = server._process_open_channel(None, b'xxx')
        self.assertEqual(ret, True)

        mock3.return_value = True
        mock4.return_value = False
        mock2.side_effect = _parse_protobuf_2
        ret = server._process_open_channel(None, b'xxx')
        self.assertEqual(ret, True)

    @patch("common.channel_manager.ChannelManager.get_channel_handler_by_fd", return_value=None)
    @patch("facial_recognition.src.facial_recognition_server.FacialRecognitionServer._parse_protobuf")
    @patch("facial_recognition.src.facial_recognition_server.FacialRecognitionServer.send_message", return_value=True)
    def test_process_frame_info_fail(self, mock1, mock2, mock3):
        server = Test_FacialRecognitionServer.server
        def parse_protobuf(request, msg_data):
            return False

        def _parse_protobuf_1(request, msg_data):
            return True

        mock2.side_effect = parse_protobuf
        ret = server._process_frame_info(None, b'xxx')
        self.assertEqual(ret, False)

        mock2.side_effect = _parse_protobuf_1
        ret = server._process_frame_info(CLIENT_SOCK, b'xxx')
        self.assertEqual(ret, False)

    def test_recognize_face_fail(self):
        server = Test_FacialRecognitionServer.server
        def produce_face_feature():
            face = facial_pb.FaceFeature()
            face.box.lt_x = 1
            face.box.lt_y = 1
            face.box.rb_x = 1
            face.box.rb_y = 1
            for j in range(100):
                face.vector.append(100)
            return face
        feature = produce_face_feature()
        ret = server._recognize_face([feature])
        self.assertEqual(ret, [])

    @patch("facial_recognition.src.facial_recognition_server.FacialRecognitionServer._compute_similar_degree", return_value=0.1)
    def test_compute_face_feature_fail(self, mock):
        server = Test_FacialRecognitionServer.server
        def produce_face_feature():
            face = facial_pb.FaceFeature()
            face.box.lt_x = 1
            face.box.lt_y = 1
            face.box.rb_x = 1
            face.box.rb_y = 1
            for j in range(100):
                face.vector.append(100)
            return face
        backup = server.registered_faces
        server.registered_faces["face_test"] = {"feature":None}
        feature = produce_face_feature()
        (_, score) = server._compute_face_feature(feature.vector)
        self.assertEqual(score, 0)

    @patch("facial_recognition.src.config_parser.ConfigParser.config_verify", return_value=False)
    def test_run_fail(self, mock):
        ret = facial_recognition_server.run()
        self.assertEqual(ret, None)



if __name__ == '__main__':
    # unittest.main()
    suite = unittest.TestSuite()
    suite.addTest(Test_FacialRecognitionServer("test_save_face_feature_fail"))
    runner = unittest.TextTestRunner()
    runner.run(suite)
