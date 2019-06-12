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


import time
import os
import sys
import json
import urllib
from logging.config import thread
import unittest
from unittest.mock import patch
path = os.path.dirname(__file__)
index = path.rfind("ascenddk")
workspace = path[0: index]
path = os.path.join(workspace, "ascenddk/presenter/server/")
sys.path.append(path)
import tornado
import tornado.httpclient
import tornado.testing

from video_analysis.src.video_analysis_server import VideoAnalysisManager
import video_analysis.src.web as webapp


#
class Test_webapp(tornado.testing.AsyncHTTPTestCase):
    """
    """
    def get_app(self):
        self.webapp = webapp.VideoWebApp()
        '''
        ret = self.webapp.list_channels()
        for item in ret:
            self.webapp.del_channel(item['name'])
        '''
        return webapp.get_webapp()


    def setUp(self):
        super(Test_webapp, self).setUp()

    def tearDown(self):
        super(Test_webapp, self).tearDown()

    
   
    @patch("video_analysis.src.video_analysis_server.VideoAnalysisManager.get_channel_name")
    @patch("video_analysis.src.video_analysis_server.VideoAnalysisManager.get_channel_list")
    @patch("video_analysis.src.video_analysis_server.VideoAnalysisManager.get_app_list")
    def test_list_app(self, mock1, mock2, mock3):
        mock1.return_value=["App1"]
        mock2.return_value=["Channel1"]
        mock3.return_value="testPath"
        ret = self.webapp.list_app()
        for app in ret:
            self.assertDictEqual(app,{"text":"App1","nodes":[{"text":"Channel1","videoName":"testPath"}]}) 

    @patch("video_analysis.src.video_analysis_server.VideoAnalysisManager.get_channel_name")
    @patch("video_analysis.src.video_analysis_server.VideoAnalysisManager.get_channel_list")
    @patch("video_analysis.src.video_analysis_server.VideoAnalysisManager.get_app_list")
    def test_list_app_with_empty_videoName(self, mock1, mock2, mock3):
        mock1.return_value=["App1"]
        mock2.return_value=["Channel1"]
        mock3.return_value=None
        ret = self.webapp.list_app()
        for app in ret:
            self.assertDictEqual(app,{"text":"App1","nodes":[{"text":"Channel1","videoName":""}]})

    @patch("video_analysis.src.video_analysis_server.VideoAnalysisManager.present_frame")
    def test_show_img_success(self, mock):
        mock.return_value= {
                "original_image": "abcr34r344r".encode(),
                "car_list":[{"image":"abcr34r344r".encode()}],
                "bus_list":[{"image":"abcr34r344r".encode()}],
                "person_list":[{"image":"abcr34r344r".encode()}]
            }
        ret = self.webapp.show_img("App1","Channel1",0)
        self.assertEqual(len(ret["data"]["car_list"]), 2, "show img car list length")
            
    @patch("video_analysis.src.video_analysis_server.VideoAnalysisManager.present_frame")
    def test_show_img_fail(self, mock):
        mock.return_value=None
        ret = self.webapp.show_img("App1","Channel1",0)
        self.assertEqual(ret["ret"], "1", "show img fail")

    @patch("video_analysis.src.video_analysis_server.VideoAnalysisManager.clean_dir")
    def test_delete_channle_img_success(self, mock):
        app_list="[{\"text\":\"APP1\",\"nodes\":[{\"text\":\"Channel1\",\"state\":{\"checked\":true}},{\"text\":\"Channel2\",\"state\":{\"checked\":false}}],\"isCheckedAllChannel\":false}]"
        mock.return_value=True
        ret = self.webapp.delete_img(app_list)
        self.assertEqual(ret["ret"], "0", "delete Channel success")

    @patch("video_analysis.src.video_analysis_server.VideoAnalysisManager.clean_dir")
    def test_delete_channle_img_fail(self, mock):
        app_list="[{\"text\":\"APP1\",\"nodes\":[{\"text\":\"Channel1\",\"state\":{\"checked\":true}},{\"text\":\"Channel2\",\"state\":{\"checked\":false}}],\"isCheckedAllChannel\":false}]"
        mock.return_value=False
        ret = self.webapp.delete_img(app_list)
        self.assertEqual(ret["ret"], "1", "delete Channel fail")

    @patch("video_analysis.src.video_analysis_server.VideoAnalysisManager.clean_dir")
    def test_delete_app_img_success(self, mock):
        app_list="[{\"text\":\"APP1\",\"nodes\":[{\"text\":\"Channel1\"}],\"isCheckedAllChannel\":true}]"
        mock.return_value=True
        ret = self.webapp.delete_img(app_list)
        self.assertEqual(ret["ret"], "0", "delete app success")
    
    @patch("video_analysis.src.video_analysis_server.VideoAnalysisManager.clean_dir")
    def test_delete_app_img_fail(self, mock):
        app_list="[{\"text\":\"APP1\",\"nodes\":[{\"text\":\"Channel1\"}],\"isCheckedAllChannel\":true}]"
        mock.return_value=False
        ret = self.webapp.delete_img(app_list)
        self.assertEqual(ret["ret"], "1", "delete app fail")
    
    @patch("video_analysis.src.video_analysis_server.VideoAnalysisManager.get_frame_number")
    def test_get_total_frame(self, mock):
        app_name="App1"
        channel_name="Channel1"
        mock.return_value=120
        ret = self.webapp.get_total_frame(app_name,channel_name)
        self.assertEqual(ret["frame"], 120, "get total frame")

    @patch("video_analysis.src.video_analysis_server.VideoAnalysisManager.get_channel_name")
    @patch("video_analysis.src.video_analysis_server.VideoAnalysisManager.get_channel_list")
    @patch("video_analysis.src.video_analysis_server.VideoAnalysisManager.get_app_list")
    def test_url_home(self, mock1, mock2, mock3):
        mock1.return_value=["App1"]
        mock2.return_value=["Channel1"]
        mock3.return_value=None
        response = self.fetch("/")
        self.assertEqual(response.code, 200, "check home")
    

    @patch("video_analysis.src.video_analysis_server.VideoAnalysisManager.present_frame")
    def test_url_showimg(self,mock):
        mock.return_value=None
        response = self.fetch("/showimg?appName=App1&channelName=Channel1", method="GET",validate_cert = False)
        self.assertEqual(response.code, 200, "check show img")

    @patch("video_analysis.src.video_analysis_server.VideoAnalysisManager.clean_dir")
    def test_url_delete(self,mock):
        mock.return_value=True
        body = "{\"applist\":\"[{\"text\":\"APP1\",\"nodes\":[{\"text\":\"Channel1\"}],\"isCheckedAllChannel\":true}]\"}"
        #{"applist": [{"text":"APP1","nodes":[{"text":"Channel1"}],"isCheckedAllChannel":"true"}]}
        #body = urllib.parse.urlencode(body)
        response = self.fetch("/del", method="POST", body=body.encode(encoding="utf-8"))
        self.assertEqual(response.code, 200, "check delete channel")

    @patch("video_analysis.src.video_analysis_server.VideoAnalysisManager.get_frame_number")
    def test_url_gettotalframe(self,mock):
        mock.return_value=120
        response = self.fetch("/gettotalframe?appName=App1&&channelName=Channel1", method="GET",validate_cert = False)
        self.assertEqual(response.code, 200, "get total frame 200")

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
