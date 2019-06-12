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

import unittest
import time
import os
import sys
import base64
from logging.config import thread
from unittest.mock import patch
path = os.path.dirname(__file__)
index = path.rfind("ascenddk")
# index = path.rfind("server")
# print(index)
workspace = path[0: index]
# print(workspace)
path = os.path.join(workspace, "ascenddk/common/presenter/server/")
# path = os.path.join(workspace, "server/")
# print(path)
sys.path.append(path)
import tornado
import tornado.httpclient
import tornado.testing
import facial_recognition.src.web as webapp

# jpeg base64 header
JPEG_BASE64_HEADER = "data:image/jpeg;base64,"

# get request
REQUEST = "req"

# get appname
APP_NAME = "app_name"

# get username
USER_NAME = "user_name"

# get image
IMAGE = "image_data"

# return code
RET_CODE_SUCCESS = "0"
RET_CODE_FAIL = "1"


class Test_webapp(tornado.testing.AsyncHTTPTestCase):
    """
    """
    @classmethod
    def setUpClass(cls):
        webapp.G_WEBAPP=webapp.WebApp()

    def get_app(self):
        self.webapp = webapp.WebApp()
        return webapp.get_webapp()

    def test_list_registered_apps(self):
        def mock_list_apps():
            return ["abc"]
        old = self.webapp.facial_recognize_manage.get_app_list
        self.webapp.facial_recognize_manage.get_app_list = mock_list_apps
        ret = self.webapp.list_registered_apps()
        self.assertEqual(ret,[{"id":1,"appname":"abc"}],"List all registered apps")
        self.webapp.facial_recognize_manage.get_app_list = old

    def test_is_channel_exists(self):
        def mock_is_channel_exists(name):
            return True
        app_name = "123"
        old = self.webapp.channel_mgr.is_channel_exist
        self.webapp.channel_mgr.is_channel_exist = mock_is_channel_exists
        ret = self.webapp.is_channel_exists(app_name)
        self.assertEqual(ret,True,"Channel exists")
        self.webapp.channel_mgr.is_channel_exist = old


    def test_register_face_name_empty(self):
        user_name = ""
        image_data = JPEG_BASE64_HEADER+"/9j"
        ret = self.webapp.register_face(user_name,image_data)
        self.assertEqual(ret['ret'],RET_CODE_FAIL,"Empty user_name not allowed")

    def test_register_face_name_None(self):
        image_data = JPEG_BASE64_HEADER+"/9j"
        ret = self.webapp.register_face(None,image_data)
        self.assertEqual(ret['ret'],RET_CODE_FAIL,"Empty user_name not allowed")

    def test_register_face_name_length_more_than_50(self):
        user_name = "123456789012345678901234567890123456789012345678901"
        image_data = JPEG_BASE64_HEADER+"/9j"
        ret = self.webapp.register_face(user_name,image_data)
        self.assertEqual(ret['ret'],RET_CODE_FAIL,"Length of user_name >50 not allowed")

    def test_register_face_name_invalid(self):
        user_name = "123?456"
        image_data = JPEG_BASE64_HEADER+"/9j"
        ret = self.webapp.register_face(user_name,image_data)
        self.assertEqual(ret['ret'],RET_CODE_FAIL,"user_name only support character, number and space ")

    def test_register_face_image_None(self):
        user_name = "123456"
        ret = self.webapp.register_face(user_name,None)
        self.assertEqual(ret['ret'],RET_CODE_FAIL,"image_data only support jpg/jpeg format ")
    
    def test_register_face_image_invalid1(self):
        user_name = "123456"
        image_data = "123456"
        ret = self.webapp.register_face(user_name,image_data)
        self.assertEqual(ret['ret'],RET_CODE_FAIL,"image_data only support jpg/jpeg format ")

    def test_register_face_image_invalid2(self):
        user_name = "123456"
        image_data = "123456789012345678901234567890"
        ret = self.webapp.register_face(user_name,image_data)
        self.assertEqual(ret['ret'],RET_CODE_FAIL,"image_data only support jpg/jpeg format ")

    def test_register_face_image_invalid3(self):
        user_name = "123456"
        image_data = JPEG_BASE64_HEADER+"12345"
        ret = self.webapp.register_face(user_name,image_data)
        self.assertEqual(ret['ret'],RET_CODE_FAIL,"image_data decode error ")

    def test_register_face_normal(self):

        def mock_register_face(user_name, decode_img):
            return (True, "Successful registration")
        
        user_name = "123456"
        image_data = JPEG_BASE64_HEADER+"/9j/4AAQSkZJRgABAQAAAQABAAD/"
        old = self.webapp.facial_recognize_manage.register_face
        self.webapp.facial_recognize_manage.register_face = mock_register_face
        ret = self.webapp.register_face(user_name,image_data)
        self.assertEqual(ret['ret'],RET_CODE_SUCCESS,"register face success ")
        self.webapp.facial_recognize_manage.register_face = old

    def test_register_face_abnormal(self):
        def mock_register_face(user_name, decode_img):
            return (False, "Register face failed")

        user_name = "123456"
        image_data = JPEG_BASE64_HEADER+"/9j/4AAQSkZJRgABAQAAAQABAAD/"
        old = self.webapp.facial_recognize_manage.register_face
        self.webapp.facial_recognize_manage.register_face = mock_register_face
        ret = self.webapp.register_face(user_name,image_data)
        self.assertEqual(ret['ret'],RET_CODE_FAIL,"register face failed ")
        self.webapp.facial_recognize_manage.register_face = old

    def test_unregister_face_name_list_None(self):
        ret=self.webapp.unregister_face(None)
        self.assertEqual(ret['ret'],RET_CODE_FAIL,"unregister face failed ")

    def test_unregister_face_name_list_Normal(self):
        def mock_register_face(user_name, decode_img):
            return (True, "Successful registration")

        def mock_unregister_face(name_list):
            return True

        user_name = "123456"
        image_data = JPEG_BASE64_HEADER+"/9j/4AAQSkZJRgABAQAAAQABAAD/"
        old = self.webapp.facial_recognize_manage.register_face
        self.webapp.facial_recognize_manage.register_face = mock_register_face
        ret = self.webapp.register_face(user_name,image_data)
        self.assertEqual(ret['ret'],RET_CODE_SUCCESS,"register face success ")
        self.webapp.facial_recognize_manage.register_face = old

        name_list = ["123456"]
        old = self.webapp.facial_recognize_manage.unregister_face
        self.webapp.facial_recognize_manage.unregister_face = mock_unregister_face
        ret=self.webapp.unregister_face(name_list)
        self.assertEqual(ret['ret'],RET_CODE_SUCCESS,"unregister face success ")
        self.webapp.facial_recognize_manage.unregister_face = old


    def test_unregister_face_name_list_abnormal(self):
        def mock_register_face(user_name, decode_img):
            return (True, "Successful registration")

        def mock_unregister_face(name_list):
            return False

        user_name = "123456"
        image_data = JPEG_BASE64_HEADER+"/9j/4AAQSkZJRgABAQAAAQABAAD/"
        old = self.webapp.facial_recognize_manage.register_face
        self.webapp.facial_recognize_manage.register_face = mock_register_face
        ret = self.webapp.register_face(user_name,image_data)
        self.assertEqual(ret['ret'],RET_CODE_SUCCESS,"register face success ")
        self.webapp.facial_recognize_manage.register_face = old

        name_list = ["123456"]
        old = self.webapp.facial_recognize_manage.unregister_face
        self.webapp.facial_recognize_manage.unregister_face = mock_unregister_face
        ret=self.webapp.unregister_face(name_list)
        self.assertEqual(ret['ret'],RET_CODE_FAIL,"unregister face failed ")
        self.webapp.facial_recognize_manage.unregister_face = old

    def test_add_requstId(self):
        req = ("123123", "123123")
        self.webapp.add_requst(req)

        ret = self.webapp.has_request(req)
        self.assertEqual(ret, True, "request check normal")

        req = ("21222", "21222")
        ret = self.webapp.has_request(req)
        self.assertEqual(ret, False, "request check normal")
    
    def test_list_allface1(self):
        def mock_get_all_face_name():
            return ["abc"]
        def mock_get_faces(name_list):
            return [{"name":"abc","image":b"image"}]
        old_get_all_face_name = self.webapp.facial_recognize_manage.get_all_face_name
        old_get_faces = self.webapp.facial_recognize_manage.get_faces
        self.webapp.facial_recognize_manage.get_all_face_name = mock_get_all_face_name
        self.webapp.facial_recognize_manage.get_faces = mock_get_faces
        ret = self.webapp.list_allface()
        self.assertEqual(ret, [{"name":"abc","image":JPEG_BASE64_HEADER+"aW1hZ2U="}],"List all face success")
        self.webapp.facial_recognize_manage.get_all_face_name = old_get_all_face_name
        self.webapp.facial_recognize_manage.get_faces = mock_get_faces = old_get_faces

    def test_list_allface2(self):
        def mock_get_all_face_name():
            return []
        old_get_all_face_name = self.webapp.facial_recognize_manage.get_all_face_name
        self.webapp.facial_recognize_manage.get_all_face_name = mock_get_all_face_name
        ret = self.webapp.list_allface()
        self.assertEqual(ret, [],"List all face success")
        self.webapp.facial_recognize_manage.get_all_face_name = old_get_all_face_name

    def test_list_allface3(self):
        def mock_get_all_face_name():
            return ["abc"]
        def mock_get_faces(name_list):
            return [{"name":"abc","image":"123"}]
        old_get_all_face_name = self.webapp.facial_recognize_manage.get_all_face_name
        old_get_faces = self.webapp.facial_recognize_manage.get_faces
        self.webapp.facial_recognize_manage.get_all_face_name = mock_get_all_face_name
        self.webapp.facial_recognize_manage.get_faces = mock_get_faces
        ret = self.webapp.list_allface()
        self.assertEqual(ret, [],"Image data encode error")
        self.webapp.facial_recognize_manage.get_all_face_name = old_get_all_face_name
        self.webapp.facial_recognize_manage.get_faces = mock_get_faces = old_get_faces

    def test_list_allfacename(self):
        def mock_list_allfacename():
            return []
        old = self.webapp.facial_recognize_manage.get_all_face_name
        self.webapp.facial_recognize_manage.get_all_face_name = mock_list_allfacename
        ret = self.webapp.list_allfacename()
        self.assertEqual(ret, [], "List all face name success")
        self.webapp.facial_recognize_manage.get_all_face_name = old

    def test_get_media_data1(self):
        class entity:
            def __init__(self, media_type, image_data):
                self.media_type = media_type
                self.image_data = image_data

            def get_media_type(self):
                return self.media_type

            def get_image_data(self):
                return self.image_data

            def get_frame(self):
                return {"image":b'image',"fps":0,"face_list":[]}
            
        def mock_is_app_exists(name):
            return True

        def mock_get_channel_handler_by_name(name):
            print("12341234513413512351")
            return entity("video",b"image")
            
        app_name = "123"
        ret = self.webapp.get_media_data(app_name)
        self.assertEqual(ret["ret"], RET_CODE_FAIL, "app_name not exists")

        old_is_app_exists=self.webapp.channel_mgr.is_channel_exist
        self.webapp.channel_mgr.is_channel_exist=mock_is_app_exists
        ret=self.webapp.get_media_data(app_name)
        self.assertEqual(ret["ret"],RET_CODE_FAIL, "get media data failed")

        old_get_channel_handler_by_name = self.webapp.channel_mgr.get_channel_handler_by_name
        self.webapp.channel_mgr.get_channel_handler_by_name = mock_get_channel_handler_by_name
        ret=self.webapp.get_media_data(app_name)
        self.assertEqual(ret["ret"],RET_CODE_SUCCESS, "get media data success")

        self.webapp.channel_mgr.is_channel_exist = old_is_app_exists
        self.webapp.channel_mgr.get_channel_handler_by_name = old_get_channel_handler_by_name

    def test_get_media_data2(self):
        class entity:
            def __init__(self, media_type, image_data):
                self.media_type = media_type
                self.image_data = image_data

            def get_media_type(self):
                return self.media_type

            def get_image_data(self):
                return self.image_data

            def get_frame(self):
                return {}
            
        def mock_is_app_exists(name):
            return True

        def mock_get_channel_handler_by_name(name):
            print("12341234513413512351")
            return entity("video",b"image")
            
        app_name = "123"
        ret = self.webapp.get_media_data(app_name)
        self.assertEqual(ret["ret"], RET_CODE_FAIL, "app_name not exists")

        old_is_app_exists=self.webapp.channel_mgr.is_channel_exist
        self.webapp.channel_mgr.is_channel_exist=mock_is_app_exists
        ret=self.webapp.get_media_data(app_name)
        self.assertEqual(ret["ret"],RET_CODE_FAIL, "get media data failed")

        old_get_channel_handler_by_name = self.webapp.channel_mgr.get_channel_handler_by_name
        self.webapp.channel_mgr.get_channel_handler_by_name = mock_get_channel_handler_by_name
        ret=self.webapp.get_media_data(app_name)
        self.assertEqual(ret["ret"],RET_CODE_FAIL, "get media data failed")

        self.webapp.channel_mgr.is_channel_exist = old_is_app_exists
        self.webapp.channel_mgr.get_channel_handler_by_name = old_get_channel_handler_by_name


    def test_get_media_data3(self):
        class entity:
            def __init__(self, media_type, image_data):
                self.media_type = media_type
                self.image_data = image_data

            def get_media_type(self):
                return self.media_type

            def get_image_data(self):
                return self.image_data

            def get_frame(self):
                return {"image":'image',"fps":0,"face_list":[]}
            
        def mock_is_app_exists(name):
            return True

        def mock_get_channel_handler_by_name(name):
            print("12341234513413512351")
            return entity("video",b"1234")
            
        app_name = "123"
        ret = self.webapp.get_media_data(app_name)
        self.assertEqual(ret["ret"], RET_CODE_FAIL, "app_name not exists")

        old_is_app_exists=self.webapp.channel_mgr.is_channel_exist
        self.webapp.channel_mgr.is_channel_exist=mock_is_app_exists
        ret=self.webapp.get_media_data(app_name)
        self.assertEqual(ret["ret"],RET_CODE_FAIL, "get media data failed")

        old_get_channel_handler_by_name = self.webapp.channel_mgr.get_channel_handler_by_name
        self.webapp.channel_mgr.get_channel_handler_by_name = mock_get_channel_handler_by_name
        ret=self.webapp.get_media_data(app_name)
        self.assertEqual(ret["ret"],RET_CODE_SUCCESS, "get media data success")

        self.webapp.channel_mgr.is_channel_exist = old_is_app_exists
        self.webapp.channel_mgr.get_channel_handler_by_name = old_get_channel_handler_by_name


    
    def test_applist_url(self):
        def mock_list_apps():
            return []
        def mock_list_allface():
            [{"name":"abc","image":JPEG_BASE64_HEADER+"aW1hZ2U="}]
        old_list_registered_apps = self.webapp.list_registered_apps
        self.webapp.list_registered_apps = mock_list_apps
        old_list_old_face = self.webapp.list_allface
        self.webapp.list_allface = mock_list_allface
        response = self.fetch("/")
        print("=========")
        print(response)
        print("=========")
        self.assertEqual(response.code, 200, "check applist")
        self.webapp.list_registered_apps = old_list_registered_apps
        self.webapp.list_allface = old_list_old_face

    
    def test_register1(self):
        def mock_list_allfacename():
            return []
        old_list_allfacename = self.webapp.list_allfacename
        self.webapp.list_allfacename = mock_list_allfacename
        response = self.fetch("/register", method="POST", body="")
        self.assertEqual(response.code, 200, "check register")

    def test_register2(self):
        def mock_list_allfacename():
            return ["123"]
        old_list_allfacename = self.webapp.list_allfacename
        self.webapp.list_allfacename = mock_list_allfacename
        response = self.fetch("/register?"+ USER_NAME+ "=123", method="POST", body="")
        self.assertEqual(response.code, 200, "check register")

    def test_delete(self):
        response = self.fetch("/delete", method="POST", body="")
        self.assertEqual(response.code, 200, "check delete")

    def test_view(self):
        # def mock_is_channel_exists(name):
        #     return True

        response = self.fetch("/view")
        self.assertEqual(response.code, 200, "view channel 200")

        # old = self.webapp.is_channel_exists
        # self.webapp.is_channel_exists = mock_is_channel_exists
        # response = self.fetch("/view")
        # self.assertEqual(response.code, 200, "check view")

    def test_view2(self):
        def mock_is_channel_exists(name):
            return True
        old = self.webapp.is_channel_exists
        self.webapp.is_channel_exists = mock_is_channel_exists
        response = self.fetch("/view")
        self.assertEqual(response.code, 200, "view channel 200")

        # old = self.webapp.is_channel_exists
        # self.webapp.is_channel_exists = mock_is_channel_exists
        # response = self.fetch("/view")
        # self.assertEqual(response.code, 200, "check view")


    @tornado.testing.gen_test
    def test_websocket_openfailed(self):
        def mock_has_request(request):
            return False
        old_has_request = self.webapp.has_request
        self.webapp.has_request = mock_has_request
        ws_url = "ws://localhost:" + str(self.get_http_port()) + "/websocket?req=1"

        ws_client = yield tornado.websocket.websocket_connect(ws_url)
        # try:
        #     ws_client.write_message("Hi, I'm sending a message to the server.")
        # except :
        #     pass

    @tornado.testing.gen_test
    def test_websocket_runtask_error(self):
        def mock_has_request1(request):
            return True
        def mock_has_request2(request):
            return False
        old_has_request = self.webapp.has_request
        self.webapp.has_request = mock_has_request1
        ws_url = "ws://localhost:" + str(self.get_http_port()) + "/websocket?name=2222&req=2222"

        ws_client = yield tornado.websocket.websocket_connect(ws_url)
        self.webapp.has_request = mock_has_request2
        ws_client.write_message("next")

    @tornado.testing.gen_test
    def test_websocket_get_media_data_error(self):
        def mock_has_request1(request):
            return True
        def mock_is_channel_exists(name):
            return True
        def mock_get_media_data_fail(name):
            return {"ret":RET_CODE_FAIL, "image":"", "fps":"0", "face_list":""}
        old_has_request = self.webapp.has_request
        self.webapp.has_request = mock_has_request1
        old_is_channel_exists = self.webapp.is_channel_exists
        self.webapp.is_channel_exists = mock_is_channel_exists
        old_get_media_data = self.webapp.get_media_data
        self.webapp.get_media_data = mock_get_media_data_fail
        ws_url = "ws://localhost:" + str(self.get_http_port()) + "/websocket?name=2222&req=2222"
        ws_client = yield tornado.websocket.websocket_connect(ws_url)
        ws_client.write_message("next")

    
    @tornado.testing.gen_test
    def test_websocket_get_media_data(self):
        def mock_has_request1(request):
            return True
        def mock_is_channel_exists(name):
            return True
        def mock_get_media_data_fail(name):
            return {"ret":RET_CODE_SUCCESS, "image":"", "fps":"0", "face_list":""}
        old_has_request = self.webapp.has_request
        self.webapp.has_request = mock_has_request1
        old_is_channel_exists = self.webapp.is_channel_exists
        self.webapp.is_channel_exists = mock_is_channel_exists
        old_get_media_data = self.webapp.get_media_data
        self.webapp.get_media_data = mock_get_media_data_fail
        ws_url = "ws://localhost:" + str(self.get_http_port()) + "/websocket?name=2222&req=2222"
        ws_client = yield tornado.websocket.websocket_connect(ws_url)
        ws_client.write_message("next")



    def test_websocket_send_message_socket_error(self):
        class streamobj:
            def __init__(self, socket):
                self.socket = socket
        class WsObj(object):

            def __init__(self, flag):
                self.ws_connection = flag

            def write_message(self, message):
                pass

        webObject = WsObj(None)
        webapp.WebSocket.send_message(webObject, "xxx")

    def test_websocket_send_message_socket_exception(self):

        class StreamObj:
            def __init__(self, socket):
                self.socket = socket

        class Connect:
            def __init__(self, stream):
                self.stream = stream

        class WsObj(object):

            def __init__(self, flag):
                self.ws_connection = flag

            def write_message(self, message, binary):
                raise tornado.websocket.WebSocketClosedError()

        webObject = WsObj(Connect(StreamObj('aaa')))
        webapp.WebSocket.send_message(webObject, "xxx")
    

    def test_start_webapp(self):
        class MyWebapp:
            def listen(self, port, address):
                pass
            def start(self):
                pass
        def get_webapp():
            return MyWebapp()
        old = webapp.get_webapp
        webapp.get_webapp = get_webapp

        oldinstance = tornado.ioloop.IOLoop.instance
        tornado.ioloop.IOLoop.instance = MyWebapp()
        try:
            webapp.start_webapp()
        except:
            pass
        webapp.get_webapp = old
        tornado.ioloop.IOLoop.instance = oldinstance

    def test_stop_webapp(self):
        try:
            webapp.stop_webapp()
        except:
            pass




#         tornado.websocket.WebSocketHandler = oldHandler
if __name__ == "__main__":
    # import sys;sys.argv = ['', 'Test.testName']
    unittest.main()