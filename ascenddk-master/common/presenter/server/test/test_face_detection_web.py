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

import unittest
import time
import os
import sys
from logging.config import thread
path = os.path.dirname(__file__)
index = path.rfind("ascenddk")
workspace = path[0: index]
path = os.path.join(workspace, "ascenddk/common/presenter/server/")
sys.path.append(path)
import tornado
import tornado.httpclient
import tornado.testing
import face_detection.src.web as webapp
#


class Test_webapp(tornado.testing.AsyncHTTPTestCase):
    """
    """

    def get_app(self):
        self.webapp = webapp.WebApp()
        ret = self.webapp.list_channels()
        for item in ret:
            self.webapp.del_channel(item['name'])

        return webapp.get_webapp()

    def setUp(self):
        self.get_media_data_old = webapp.G_WEBAPP.get_media_data
        super(Test_webapp, self).setUp()

    def tearDown(self):
        super(Test_webapp, self).tearDown()
        webapp.G_WEBAPP.get_media_data = self.get_media_data_old



    def test_add_channel_name_emtpy(self):
        channel_name = ""
        ret = self.webapp.add_channel(channel_name)
        self.assertEqual(ret['ret'], "error", "empty channel_name not allowed")

    def test_add_channel_name_None(self):
        ret = self.webapp.add_channel(None)
        self.assertEqual(ret['ret'], "error", "empty channel_name not allowed")


    def test_add_channel_name_more_than_25(self):
        channel_name = "12345678901234567890123456"
        ret = self.webapp.add_channel(channel_name)
        self.assertEqual(ret['ret'], "error", "channel_name > 25 not allowed")

    def test_add_channel_name_not_validate(self):
        channel_name = "abcdef?/1233"
        ret = self.webapp.add_channel(channel_name)
        self.assertEqual(ret['ret'], "error", "channel_name only support charactor and number")

    def test_add_channel_name_noraml(self):
        channel_name = "123abc"
        ret = self.webapp.add_channel(channel_name)
        self.assertEqual(ret['ret'], "success", "add chann_name failed.")

    def test_add_channel_name_repeat(self):

        channel_name = "123abc"
        ret = self.webapp.add_channel(channel_name)
        self.assertEqual(ret['ret'], "success", "add chann_name failed.")
        channel_name = "123abc"
        ret = self.webapp.add_channel(channel_name)
        self.assertEqual(ret['ret'], "error", "channel name already exist")


    def test_add_channel_name_count_more_than_ten(self):
        for i in range(11):
            channel_name = "abc" + str(i)
            ret = self.webapp.add_channel(channel_name)
            if i < 10:
                self.assertEqual(ret['ret'], "success", "add chann_name.")
            else:
                self.assertEqual(ret['ret'], "error", "channel name max size is 10")

    def test_del_channel_empty(self):
        channel_name = ""
        ret = self.webapp.del_channel(channel_name)
        self.assertEqual(ret['ret'], "error", "empty channel_name not allowed")

    def test_del_channel_normal_1(self):
        channel_name = "aaa"
        ret = self.webapp.add_channel(channel_name)
        self.assertEqual(ret['ret'], "success", "add channel name normal")

        channel_name = "aaa,"
        ret = self.webapp.del_channel(channel_name)
        self.assertEqual(ret['ret'], "success", "delete channel name normal")

    def test_del_channel_normal_2(self):
        channel_name = "aaa"
        ret = self.webapp.add_channel(channel_name)
        self.assertEqual(ret['ret'], "success", "add channel name normal")

        channel_name = "aaa"
        ret = self.webapp.del_channel(channel_name)
        self.assertEqual(ret['ret'], "success", "delete channel name normal")


    def test_list_channels(self):
        channel_name = "aaa"
        ret = self.webapp.add_channel(channel_name)
        self.assertEqual(ret['ret'], "success", "add channel name normal")

        ret = self.webapp.list_channels()

        self.assertEqual(len(ret), 1, "list channel name normal")

    def test_is_channel_exists(self):
        channel_name = "aaa"
        ret = self.webapp.add_channel(channel_name)
        self.assertEqual(ret['ret'], "success", "add channel name normal")

        ret = self.webapp.is_channel_exists(channel_name)
        self.assertEqual(ret, True, "check channel exists normal")

        ret = self.webapp.is_channel_exists("bbb")
        self.assertEqual(ret, False, "check channel not exists")


    def test_add_requstId(self):
        req = ("123123", "123123")
        self.webapp.add_requst(req)

        ret = self.webapp.has_request(req)
        self.assertEqual(ret, True, "request check normal")

        req = ("21222", "21222")
        ret = self.webapp.has_request(req)
        self.assertEqual(ret, False, "request check normal")

    def test_get_media_data(self):
        class entity:
            def __init__(self, media_type, image_data):
                self.media_type = media_type
                self.image_data = image_data

            def get_media_type(self):
                return self.media_type

            def get_image_data(self):
                return self.image_data

            def get_frame(self):
                return self.image_data, 10

        def mock_get_channel_path_image(name):
            return b"image"

        def mock_get_channel_process_entity_by_name_forimage(name):
            return entity("image", b"image")

        def mock_get_channel_process_entity_by_name_forvideo(name):
            return entity("video", b"image")

        channel_name = "aaa"
        index = 0
        item = self.webapp.get_media_data(channel_name)
        self.assertEqual(item['status'], 'error', "channel name not exists")

        self.webapp.add_channel(channel_name)
        item = self.webapp.get_media_data(channel_name)
        self.assertEqual(item['status'], 'loading', "channel name not exists")


        old = self.webapp.channel_mgr.get_channel_image
        self.webapp.channel_mgr.get_channel_image = mock_get_channel_path_image
        item = self.webapp.get_media_data(channel_name)
        self.assertEqual(item['status'], 'ok', "channel name not exists")
        self.webapp.channel_mgr.get_channel_image = old


        old = self.webapp.channel_mgr.get_channel_handler_by_name
        self.webapp.channel_mgr.get_channel_handler_by_name = mock_get_channel_process_entity_by_name_forimage
        item = self.webapp.get_media_data(channel_name)
        self.assertEqual(item['status'], 'ok', "get media data for image")


        self.webapp.channel_mgr.get_channel_handler_by_name = mock_get_channel_process_entity_by_name_forvideo
        item = self.webapp.get_media_data(channel_name)
        self.assertEqual(item['status'], 'ok', "get media data for video")
        self.webapp.channel_mgr.get_channel_handler_by_name = old




    def test_homeurl(self):

        response = self.fetch("/")
        self.assertEqual(response.code, 200, "check home")

    def test_add(self):

        response = self.fetch("/add", method="POST", body="")
        self.assertEqual(response.code, 200, "check add")

    def test_delete(self):
        response = self.fetch("/del", method="POST", body="")
        self.assertEqual(response.code, 200, "check add")

    def test_view(self):
        response = self.fetch("/view")
        self.assertEqual(response.code, 404, "view channel 404")


        response = self.fetch("/add?name=DDDDDDD", method="POST", body="")
        response = self.fetch("/view?name=DDDDDDD")
        self.assertEqual(response.code, 200, "view channel ok")

    def test_mediaview_invalidate(self):
        response = self.fetch("/meidaview")
        self.assertEqual(response.code, 404, "view meida 404")

    def test_mediaview_loading(self):
        try:
            webapp.G_WEBAPP.add_requst("13123213")
            self.fetch("/add?name=bbbb", method="POST", body="")
            response = self.fetch("/meidaview?name=bbbb&&req=13123213")
            self.assertEqual(response.code, 200, "view meida ok")
        except:
            pass

    def test_mediaview_error(self):
        def mock_get_media_data_error(name):
            return [{'status':'error', "image":None}]

        old = webapp.G_WEBAPP.get_media_data
        webapp.G_WEBAPP.get_media_data = mock_get_media_data_error
        try:
            webapp.G_WEBAPP.add_requst("13123213")
            self.fetch("/add?name=bbbb", method="POST", body="")
            response = self.fetch("/meidaview?name=bbbb&&req=13123213")
        except:
            pass
        webapp.G_WEBAPP.get_media_data = old

    def test_mediaview_ok(self):
        def mock_get_media_data_ok(name):
            return [{'status':'ok', "image":"aabb", 'fps':'10', 'type':'1212'}]

        old = webapp.G_WEBAPP.get_media_data
        webapp.G_WEBAPP.get_media_data = mock_get_media_data_ok
        try:
            webapp.G_WEBAPP.add_requst("13123213")
            self.fetch("/add?name=bbbb", method="POST", body="")
            response = self.fetch("/meidaview?name=bbbb&&req=13123213")
        except:
            pass
        webapp.G_WEBAPP.get_media_data = old




    def test_mediaview_ok2(self):
        def mock_get_media_data_ok2(name):
            return [{'status':'ok', "image":"aabb", 'fps':'10', 'type':'image'}]

        old = webapp.G_WEBAPP.get_media_data

        webapp.G_WEBAPP.get_media_data = mock_get_media_data_ok2
        try:
            webapp.G_WEBAPP.add_requst("13123213")
            self.fetch("/add?name=bbbb", method="POST", body="")
            response = self.fetch("/meidaview?name=bbbb&&req=13123213")
        except:
            pass
        webapp.G_WEBAPP.get_media_data = old

    @tornado.testing.gen_test
    def test_websocket_openfailed(self):

        ws_url = "ws://localhost:" + str(self.get_http_port()) + "/websocket?req=1"

        ws_client = yield tornado.websocket.websocket_connect(ws_url)
        try:
            ws_client.write_message("Hi, I'm sending a message to the server.")
        except :
            pass

    @tornado.testing.gen_test
    def test_websocket_normal(self):

        webapp.G_WEBAPP.add_requst(('2222', '2222'))
        webapp.G_WEBAPP.add_channel('2222')
        ws_url = "ws://localhost:" + str(self.get_http_port()) + "/websocket?name=2222&req=2222"

        ws_client = yield tornado.websocket.websocket_connect(ws_url)
        ws_client.write_message("next")

    @tornado.testing.gen_test
    def test_websocket_channel_not_exists(self):

        webapp.G_WEBAPP.add_requst(('2222', '2222'))
        webapp.G_WEBAPP.add_channel('4444')
        ws_url = "ws://localhost:" + str(self.get_http_port()) + "/websocket?name=2222&req=2222"

        ws_client = yield tornado.websocket.websocket_connect(ws_url)
        ws_client.write_message("next")

    @tornado.testing.gen_test
    def test_websocket_media_data_error(self):
        def mock_get_media_data_error(name):
            return {'status':'error', "image":"aabb", 'fps':'10', 'type':'image'}

        old = webapp.G_WEBAPP.get_media_data
        webapp.G_WEBAPP.get_media_data = mock_get_media_data_error

        webapp.G_WEBAPP.add_requst(('2222', '2222'))
        webapp.G_WEBAPP.add_channel('2222')
        ws_url = "ws://localhost:" + str(self.get_http_port()) + "/websocket?name=2222&req=2222"


        ws_client = yield tornado.websocket.websocket_connect(ws_url)
        ws_client.write_message("next")

    @tornado.testing.gen_test
    def test_websocket_runtask_ok(self):
        def mock_get_media_data_ok(name):
            return {'status':'ok', "image":"aabb", 'fps':'10', 'type':'image'}

        old = webapp.G_WEBAPP.get_media_data
        webapp.G_WEBAPP.get_media_data = mock_get_media_data_ok

        webapp.G_WEBAPP.add_requst(('2222', '2222'))
        webapp.G_WEBAPP.add_channel('2222')
        ws_url = "ws://localhost:" + str(self.get_http_port()) + "/websocket?name=2222&req=2222"


        ws_client = yield tornado.websocket.websocket_connect(ws_url)
        ws_client.write_message("next")

    @tornado.testing.gen_test
    def test_websocket_send_message(self):
        def mock_get_media_data_ok(name):
            time.sleep(0.5)
            return {'status':'ok', "image":"aabb", 'fps':'10', 'type':'image'}

        old = webapp.G_WEBAPP.get_media_data
        webapp.G_WEBAPP.get_media_data = mock_get_media_data_ok

        webapp.G_WEBAPP.add_requst(('2222', '2222'))
        webapp.G_WEBAPP.add_channel('2222')
        ws_url = "ws://localhost:" + str(self.get_http_port()) + "/websocket?name=2222&req=2222"


        ws_client = yield tornado.websocket.websocket_connect(ws_url)
        ws_client.write_message("next")
        ws_client.close()



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

#         not cls.ws_connection or not cls.ws_connection.stream.socket




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
