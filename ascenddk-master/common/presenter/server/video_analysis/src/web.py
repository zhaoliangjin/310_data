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
"""
video anlysis web application for presenter server.
"""
import os
import base64
import threading
import json
import logging
import tornado.ioloop
import tornado.web
import tornado.gen
import tornado.websocket
import video_analysis.src.config_parser as config_parser
from video_analysis.src.video_analysis_server import VideoAnalysisManager

RET_CODE_SUCCESS="0"
RET_CODE_FAIL="1"
RET_MSG_GET_FRAME_FAIL="Can not get frame.Please retry."
RET_MSG_DEL_APP_FAIL="Delete app failed.Please retry."
RET_MSG_DEL_CHANNEL_FAIL="Delete app failed.Please retry."
FLAG_CHECKED_ALL="isCheckedAllChannel"
BASE_64_HEADER="data:image/jpeg;base64,"

class VideoWebApp:
    """
    web application
    """
    __instance = None

    def __init__(self):
        """
        init method
        """
        self.mgr = VideoAnalysisManager()

        self.request_list = set()

        self.lock = threading.Lock()

    def __new__(cls, *args, **kwargs):

        # if instance is None than create one
        if cls.__instance is None:
            cls.__instance = object.__new__(cls, *args, **kwargs)
        return cls.__instance

    def list_app(self):
        """
        list app & channel

        @return: return app and children channel and message (for error status)

        """
        app_node_list = []

        # check applist
        app_list = self.mgr.get_app_list()
        for app in app_list:
            app_node = {"text": app, "nodes": []}
            for i in self.mgr.get_channel_list(app):
                channel_path = self.mgr.get_channel_name(app,i)
                if channel_path is None: 
                   channel_path=""
                channel = {"text": i,"videoName":channel_path}
                app_node["nodes"].append(channel)
            app_node_list.append(app_node)

        return app_node_list

    def show_img(self, app_name, channel_name, frame):
        """
        present image

        @param channel_name  name of channel
        @return: return frame image and small img

        """
        ret = {"ret": RET_CODE_SUCCESS, "msg": "", "data": None}
        # invoke present frame
        data = self.mgr.present_frame(app_name, channel_name, frame)
        if data is None:
            ret["ret"] = RET_CODE_FAIL
            ret["msg"] = RET_MSG_GET_FRAME_FAIL
        else:
            data["original_image"] = BASE_64_HEADER\
                                    +base64.b64encode(data["original_image"]).decode("utf-8")
            for car in data["car_list"]:
                car["image"] = BASE_64_HEADER\
                            +base64.b64encode(car["image"]).decode("utf-8")
            for bus in data["bus_list"]:
                bus["image"] = BASE_64_HEADER\
                            +base64.b64encode(bus["image"]).decode("utf-8")
            for person in data["person_list"]:
                person["image"] = BASE_64_HEADER\
                                +base64.b64encode(person["image"]).decode("utf-8")
            data["car_list"].extend(data["bus_list"])
            ret["data"] = data
        return ret

    def delete_img(self, app_list):
        """
        delete image

        @param app_list  list of app which from tree
        """
        ret = {"ret": RET_CODE_SUCCESS, "msg": ""}
        if app_list:
            app_list = json.loads(app_list)
        else:
            ret["ret"]=RET_CODE_FAIL
            ret["msg"]=RET_MSG_DEL_CHANNEL_FAIL
        
        # ivoke delete img
        for app in app_list:
            if app[FLAG_CHECKED_ALL]:
                result = self.mgr.clean_dir(app["text"])
                if result:
                    logging.info("delete app: %s successfully.", app["text"])
                else:
                    logging.warning("delete app: %s failed.", app["text"])
                    ret["ret"]=RET_CODE_FAIL
                    ret["msg"]=RET_MSG_DEL_APP_FAIL
            else:
                for channel in app["nodes"]:
                    if channel["state"]["checked"]:
                        result = self.mgr.clean_dir(app["text"], channel["text"])
                        if result:
                            logging.info("delete channel: %s which belongs to app:%s successfully.", channel["text"], app["text"])
                        else:
                            logging.warning("delete channel: %s which belongs to app:%s failed.", channel["text"], app["text"])
                            ret["ret"]=RET_CODE_FAIL
                            ret["msg"]=RET_MSG_DEL_CHANNEL_FAIL
        return ret

    def get_total_frame(self, app_name, channel_name):
        ret = {"ret": RET_CODE_SUCCESS, "msg": "", "frame": None}
        ret["frame"] = self.mgr.get_frame_number(app_name, channel_name)
        return ret


# pylint: disable=abstract-method
class BaseHandler(tornado.web.RequestHandler):
    """
    base handler.
    """
    #pass

# pylint: disable=abstract-method


class HomeHandler(BaseHandler):
    """
    handler index request
    """

    @tornado.web.asynchronous
    def get(self, *args, **kwargs):
        """
        handle home or index request only for get
        """
        self.render("home.html", treedata=G_WEBAPP.list_app())


class ShowImgHandler(BaseHandler):
    """
    handler index request
    """

    @tornado.web.asynchronous
    def get(self, *args, **kwargs):
        """
        handle home or index request only for get
        """
        app_name = self.get_argument('appName', '')
        channel_name = self.get_argument('channelName', '')
        frame = self.get_argument('frame', '')
        self.finish(G_WEBAPP.show_img(app_name, channel_name, frame))


class DelHandler(BaseHandler):
    """
    handler index request
    """

    @tornado.web.asynchronous
    def post(self, *args, **kwargs):
        """
        handle home or index request only for get
        """
        app_list = self.get_argument('applist', '')
        self.finish(G_WEBAPP.delete_img(app_list))


class GetTotalFrameHandler(BaseHandler):
    """
    handler index request
    """

    @tornado.web.asynchronous
    def get(self, *args, **kwargs):
        """
        handle home or index request only for get
        """
        app_name = self.get_argument('appName', '')
        channel_name = self.get_argument('channelName', '')
        self.finish(G_WEBAPP.get_total_frame(app_name, channel_name))


def get_webapp():
    """
    start web applicatioin
    """
    # get template file and static file path.
    templatepath = os.path.join(
        config_parser.ConfigParser.get_rootpath(), "ui/templates")
    staticfilepath = os.path.join(
        config_parser.ConfigParser.get_rootpath(), "ui/static")

    # create application object.
    app = tornado.web.Application(handlers=[(r"/", HomeHandler),
                                            (r"/index", HomeHandler),
                                            (r"/showimg", ShowImgHandler),
                                            (r"/del", DelHandler),
                                            (r"/gettotalframe",
                                             GetTotalFrameHandler),
                                            (r"/static/(.*)",
                                             tornado.web.StaticFileHandler,
                                             {"path": staticfilepath})],
                                  template_path=templatepath)

    # create server
    http_server = tornado.httpserver.HTTPServer(app)

    return http_server


def start_webapp():
    """
    start webapp
    """
    global G_WEBAPP
    G_WEBAPP = VideoWebApp()
    http_server = get_webapp()
    config = config_parser.ConfigParser()
    http_server.listen(config.web_server_port, address=config.web_server_ip)

    print("Please visit http://" + config.web_server_ip + ":" +
          str(config.web_server_port) + " for video presenter server")
    tornado.ioloop.IOLoop.instance().start()


def stop_webapp():
    """
    stop web app
    """
    tornado.ioloop.IOLoop.instance().stop()
