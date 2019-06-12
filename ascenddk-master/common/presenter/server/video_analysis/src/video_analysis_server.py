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
"""video analysis server module"""

import os
import re
import shutil
import json
from json.decoder import JSONDecodeError
import logging
from logging.config import fileConfig
from google.protobuf.message import DecodeError
import video_analysis.src.video_analysis_message_pb2 as pb2
from video_analysis.src.config_parser import ConfigParser
import common.presenter_message_pb2 as presenter_message_pb2
from common.channel_manager import ChannelManager
from common.presenter_socket_server import PresenterSocketServer
from common.app_manager import AppManager


IMAGE_FILE = "image.jpg"
JSON_FILE = "inference.json"
CHANNEL_NAME_FILE = "channel_name.txt"
SERVER_TYPE = "video_analysis"
CHECK_INTERCAL = 100
MAX_SUB_DIRECTORY_NUM = 30000

class VideoAnalysisServer(PresenterSocketServer):
    '''Video Analysis Server'''
    def __init__(self, config):
        server_address = (config.presenter_server_ip,
                          int(config.presenter_server_port))
        self.storage_dir = config.storage_dir
        self.max_app_num = int(config.max_app_num)
        self.reserved_space = int(config.reserved_space)
        self.app_manager = AppManager()
        self.frame_num = 0
        super(VideoAnalysisServer, self).__init__(server_address)

    def _clean_connect(self, sock_fileno, epoll, conns, msgs):
        """
        Description: close socket, and clean local variables
        Args:
            sock_fileno: a socket fileno, return value of socket.fileno()
            epoll: a set of select.epoll.
            conns: all socket connections registered in epoll
            msgs: msg read from a socket
        """
        logging.info("clean fd:%s, conns:%s", sock_fileno, conns)
        self.app_manager.unregister_app_by_fd(sock_fileno)
        epoll.unregister(sock_fileno)
        conns[sock_fileno].close()
        del conns[sock_fileno]
        del msgs[sock_fileno]

    def _process_msg(self, conn, msg_name, msg_data):
        """
        Description: Total entrance to process protobuf msg
        Args:
            conn: a socket connection
            msg_name: name of a msg.
            msg_data: msg body, serialized by protobuf

        Returns:
            False:somme error occured
            True:succeed

        """
        # process open channel request
        if msg_name == pb2._REGISTERAPP.full_name:
            ret = self._process_register_app(conn, msg_data)
        elif msg_name == pb2._IMAGESET.full_name:
            ret = self._process_image_set(conn, msg_data)
        elif msg_name == pb2._CARINFERENCERESULT.full_name:
            ret = self._process_car_inference_result(conn, msg_data)
        elif msg_name == pb2._HUMANINFERENCERESULT.full_name:
            ret = self._process_human_inference_result(conn, msg_data)
        elif msg_name == presenter_message_pb2._HEARTBEATMESSAGE.full_name:
            ret = self._process_heartbeat(conn)
        # process image request, receive an image data from presenter agent
        else:
            logging.error("Not recognized msg type %s", msg_name)
            ret = False

        return ret

    def _process_heartbeat(self, conn):
        '''
        Description: set heartbeat
        Input:
            conn: a socket connection
        Returns:
            True: set heartbeat ok.

        '''
        if self.app_manager.get_app_id_by_socket(conn.fileno()):
            self.app_manager.set_heartbeat(conn.fileno())
        return True

    def _parse_protobuf(self, protobuf, msg_data):
        """
        Description:  parse protobuf
        Input:
            protobuf: a struct defined by protobuf
            msg_data: msg body, serialized by protobuf
        Returns: True or False
        """
        try:
            protobuf.ParseFromString(msg_data)
            return True
        except DecodeError as exp:
            logging.error(exp)
            return False

    def _process_register_app(self, conn, msg_data):
        '''
        Description: process register_app message
        Input:
            conn: a socket connection
            msg_data: message data.
        Returns: True or False
        '''
        request = pb2.RegisterApp()
        response = pb2.CommonResponse()
        msg_name = pb2._COMMONRESPONSE.full_name

        if not self._parse_protobuf(request, msg_data):
            self._response_error_unknown(conn)
            return False

        app_id = request.id
        app_type = request.type
        # check app id if exist
        app_dir = os.path.join(self.storage_dir, app_id)
        if os.path.isdir(app_dir):
            logging.error("App %s is already exist.", app_id)
            response.ret = pb2.kErrorAppRegisterExist
            response.message = "App {} is already exist.".format(app_id)
            self.send_message(conn, response, msg_name)
        elif self.app_manager.get_app_num() >= self.max_app_num:
            logging.error("App number reach the upper limit")
            response.ret = pb2.kErrorAppRegisterLimit
            response.message = "App number reach the upper limit"
            self.send_message(conn, response, msg_name)
        elif app_type != SERVER_TYPE:
            logging.error("App type %s error", app_type)
            response.ret = pb2.kErrorAppRegisterType
            response.message = "App type {} error".format(app_type)
            self.send_message(conn, response, msg_name)
        elif self._is_app_id_invalid(app_id):
            logging.error("App id %s is too long", app_id)
            response.ret = pb2.kErrorOther
            response.message = "App id: {} is too long".format(app_id)
            self.send_message(conn, response, msg_name)
        elif self._remain_space() < self.reserved_space:
            logging.error("Insufficient storage space on Presenter Server.")
            response.ret = pb2.kErrorAppRegisterNoStorage
            response.message = "Insufficient storage space on Presenter Server"
            self.send_message(conn, response, msg_name)
        else:
            self.app_manager.register_app(app_id, conn)
            # app_dir = os.path.join(self.storage_dir, app_id)
            # if not os.path.isdir(app_dir):
            #     os.makedirs(app_dir)
            response.ret = pb2.kErrorNone
            response.message = "Register app {} succeed".format(app_id)
            self.send_message(conn, response, msg_name)
            return True
        return False

    def _process_image_set(self, conn, msg_data):
        '''
        Description: process image_set message
        Input:
            conn: a socket connection
            msg_data: message data.
        Returns: True or False
        '''
        request = pb2.ImageSet()
        response = pb2.CommonResponse()
        msg_name = msg_name = pb2._COMMONRESPONSE.full_name
        if not self._parse_protobuf(request, msg_data):
            self._response_error_unknown(conn)
            return False

        app_id = request.frame_index.app_id
        channel_id = request.frame_index.channel_id
        channel_name = request.frame_index.channel_name
        frame_id = request.frame_index.frame_id
        frame_image = request.frame_image

        if not self.app_manager.is_app_exist(app_id):
            logging.error("app_id: %s not exist", app_id)
            response.ret = pb2.kErrorAppLost
            response.message = "app_id: %s not exist"%(app_id)
            self.send_message(conn, response, msg_name)
            return False

        frame_num = self.app_manager.get_frame_num(app_id, channel_id)
        if frame_num % CHECK_INTERCAL == 0:
            if self._remain_space() <= self.reserved_space:
                logging.error("Insufficient storage space on Server.")
                response.ret = pb2.kErrorStorageLimit
                response.message = "Insufficient storage space on Server."
                self.send_message(conn, response, msg_name)
                return False

        stack_index = frame_num // MAX_SUB_DIRECTORY_NUM
        stack_directory = "stack_{}/".format(stack_index)
        frame = stack_directory + frame_id
        frame_dir = os.path.join(self.storage_dir, app_id, channel_id, frame)
        if not self._save_image(frame_dir, frame_image):
            self._response_error_unknown(conn)
            logging.error("save_image: %s error.", frame_dir)
            return False

        app_dir = os.path.join(self.storage_dir, app_id)
        self._save_channel_name(app_dir, channel_id, channel_name)

        for i in request.object:
            object_id = i.id
            object_confidence = i.confidence
            object_image = i.image
            object_dir = os.path.join(frame_dir, object_id)
            inference_dict = {"confidence" : object_confidence}

            if not self._save_image(object_dir, object_image) or \
              not self._save_inference_result(object_dir, inference_dict):
                self._response_error_unknown(conn)
                logging.error("save image: %s error.", object_dir)
                return False

        self.app_manager.increase_frame_num(app_id, channel_id)

        self.app_manager.set_heartbeat(conn.fileno())
        response.ret = pb2.kErrorNone
        response.message = "image set process succeed"
        self.send_message(conn, response, msg_name)
        return True

    def _process_car_inference_result(self, conn, msg_data):
        '''
        Description: process car_inference_result message
        Input:
            conn: a socket connection
            msg_data: message data.
        Returns: True or False
        '''
        request = pb2.CarInferenceResult()
        response = pb2.CommonResponse()
        msg_name = msg_name = pb2._COMMONRESPONSE.full_name
        inference_dict = {}
        if not self._parse_protobuf(request, msg_data):
            self._response_error_unknown(conn)
            return False

        app_id = request.frame_index.app_id
        channel_id = request.frame_index.channel_id
        frame_id = request.frame_index.frame_id
        object_id = request.object_id

        if not self.app_manager.is_app_exist(app_id):
            logging.error("app_id: %s not exist", app_id)
            response.ret = pb2.kErrorAppLost
            response.message = "app_id: %s not exist"%(app_id)
            self.send_message(conn, response, msg_name)
            return False

        channel_dir = os.path.join(self.storage_dir, app_id, channel_id)
        stack_list = os.listdir(channel_dir)
        stack_list.sort()
        current_stack = stack_list[-1]
        object_dir = os.path.join(channel_dir, current_stack, frame_id, object_id)
        if request.type == pb2.kCarColor:
            inference_dict["color_confidence"] = request.confidence
            inference_dict["color"] = request.value
        elif request.type == pb2.kCarBrand:
            inference_dict["brand_confidence"] = request.confidence
            inference_dict["brand"] = request.value
        else:
            logging.error("unknown type %d", request.type)
            self._response_error_unknown(conn)
            return False

        if not self._save_inference_result(object_dir, inference_dict):
            self._response_error_unknown(conn)
            return False
        self.app_manager.set_heartbeat(conn.fileno())
        response.ret = pb2.kErrorNone
        response.message = "car inference process succeed"
        self.send_message(conn, response, msg_name)

        return True

    def _process_human_inference_result(self, conn, msg_data):
        '''
        Description: process human_inference_result message
        Input:
            conn: a socket connection
            msg_data: message data.
        Returns: True or False
        '''
        request = pb2.HumanInferenceResult()
        response = pb2.CommonResponse()
        msg_name = msg_name = pb2._COMMONRESPONSE.full_name
        inference_dict = {}
        if not self._parse_protobuf(request, msg_data):
            self._response_error_unknown(conn)
            return False

        app_id = request.frame_index.app_id
        channel_id = request.frame_index.channel_id
        frame_id = request.frame_index.frame_id
        object_id = request.object_id

        if not self.app_manager.is_app_exist(app_id):
            logging.error("app_id: %s not exist", app_id)
            response.ret = pb2.kErrorAppLost
            response.message = "app_id: %s not exist"%(app_id)
            self.send_message(conn, response, msg_name)
            return False

        channel_dir = os.path.join(self.storage_dir, app_id, channel_id)
        stack_list = os.listdir(channel_dir)
        stack_list.sort()
        current_stack = stack_list[-1]
        object_dir = os.path.join(channel_dir, current_stack, frame_id, object_id)

        inference_dict["property"] = {}
        for item in request.human_property:
            inference_dict["property"][item.key] = item.value

        if not self._save_inference_result(object_dir, inference_dict):
            self._response_error_unknown(conn)
            return False
        self.app_manager.set_heartbeat(conn.fileno())
        response.ret = pb2.kErrorNone
        response.message = "human inference process succeed"
        self.send_message(conn, response, msg_name)
        return True

    def _response_error_unknown(self, conn):
        '''
        Description: response error_unknown message
        Input:
            conn: a socket connection
        Returns: NA
        '''
        msg_name = msg_name = pb2._COMMONRESPONSE.full_name
        response = pb2.CommonResponse()
        response.ret = pb2.kErrorOther
        response.message = "Error unknown on Presenter Server"
        self.send_message(conn, response, msg_name)

    def _save_image(self, directory, image):
        '''
        Description: save image
        Input:
            conn: a socket connection
            image: image content
        Returns: True or False
        '''
        try:
            if not os.path.isdir(directory):
                os.makedirs(directory)
            image_file = os.path.join(directory, IMAGE_FILE)
            with open(image_file, "wb") as f:
                f.write(image)
            return True
        except OSError as exp:
            logging.error(exp)
            return False

    def _save_channel_name(self, app_dir, channel_id, channel_name):
        '''
        Description: save channel_name to json file
        Input:
            app_dir: app directory
            channel_id: channel id
            channel_name: channel name
        Returns: NA
        '''
        channel_name_file = os.path.join(app_dir, CHANNEL_NAME_FILE)
        item = {channel_id: channel_name}
        try:
            if not os.path.isfile(channel_name_file):
                os.mknod(channel_name_file)
            with open(channel_name_file, 'r') as json_f:
                try:
                    json_dict = json.load(json_f)
                except (OSError, JSONDecodeError):
                    json_dict = {}
            with open(channel_name_file, 'w') as json_f:
                    json_dict.update(item)
                    json.dump(json_dict, json_f)
        except (OSError, JSONDecodeError):
            return False
        return True

    def _save_inference_result(self, directory, inference_dict):
        '''
        Description: save inference_result to json file
        Input:
            directory: a directory
            inference_dict: inference result
        Returns: NA
        '''
        json_dict = {}
        try:
            if not os.path.isdir(directory):
                os.makedirs(directory)

            json_file = os.path.join(directory, JSON_FILE)
            try:
                with open(json_file, 'r') as json_f:
                    json_dict = json.load(json_f)
            except (OSError, JSONDecodeError):
                json_dict = {}

            json_dict = dict(json_dict, **inference_dict)
            with open(json_file, 'w') as json_f:
                json.dump(json_dict, json_f)
            return True
        except (OSError, JSONDecodeError) as exp:
            logging.error(exp)
            return False

    def _remain_space(self):
        '''
        Description: compute system remain space.
        Input: NA
        Returns: NA
        '''
        disk = os.statvfs(self.storage_dir)
        available_mbit = (disk.f_bsize * disk.f_bavail) / (1024 * 1024)
        return available_mbit

    def _is_app_id_invalid(self, app_id):
        pattern = re.compile(r'([a-zA-Z0-9_]{3,20})$')
        if pattern.match(app_id):
            return False
        return True

    def stop_thread(self):
        """
        Description: clean thread when process exit.
        Input: NA
        Returns: NA
        """
        channel_manager = ChannelManager([])
        channel_manager.close_all_thread()
        self.set_exit_switch()
        self.app_manager.set_thread_switch()

class VideoAnalysisManager():
    '''Video Analysis Manager class, providing APIs for web.py'''
    __instance = None
    server = None

    def __init__(self, server=None):
        self.app_manager = AppManager()
        if server is not None:
            VideoAnalysisManager.server = server
            self.storage_dir = self.server.storage_dir

    def __new__(cls, server=None):
        """ensure only a single instance created. """
        if cls.__instance is None:
            cls.__instance = object.__new__(cls)
            cls.server = server
        return cls.__instance

    def get_app_list(self):
        """
        Description: get app list
        Input: NA
        Returns: a app list
        """
        if os.path.isdir(self.storage_dir):
            dir_list = os.listdir(self.storage_dir)
            return dir_list

        return []

    def get_channel_list(self, app_id):
        """
        Description: get channel list
        Input:
            app_id: specified app id
        Returns: a channel list
        """
        if not isinstance(app_id, str):
            return []

        app_dir = os.path.join(self.storage_dir, app_id)
        if os.path.isdir(app_dir):
            channel_list = os.listdir(app_dir)
            if CHANNEL_NAME_FILE in channel_list:
                channel_list.remove(CHANNEL_NAME_FILE)
            return channel_list

        return []

    def get_frame_number(self, app_id, channel_id):
        """
        Description: get total frame number of the specified app and channel
        Input:
            app_id: a app id
            channel_id: a channel id
        Returns: total frame number
        """
        channel_dir = os.path.join(self.storage_dir, app_id, channel_id)
        frame_total_num = 0
        if os.path.exists(channel_dir):
            sub_dirs = os.listdir(channel_dir)
            for sub_dir in sub_dirs:
                stack_dir = os.path.join(channel_dir, sub_dir)
                sub_dir_len = len(os.listdir(stack_dir))
                frame_total_num += sub_dir_len
            return frame_total_num
        return 0

    def clean_dir(self, app_id, channel=None):
        """
        Description: API for cleaning the specified app and channel
        Input:
            app_id: a app id
            channel_id: a channel id
        Returns: True or False
        """
        if not isinstance(app_id, str):
            return False

        app_dir = os.path.join(self.storage_dir, app_id)
        if channel is None:
            channel_dir = app_dir
        else:
            channel_dir = os.path.join(app_dir, channel)

        if os.path.isdir(channel_dir):
            try:
                sock_fileno = self.app_manager.get_socket_by_app_id(app_id)
                self.app_manager.unregister_app_by_fd(sock_fileno)
                shutil.rmtree(channel_dir, ignore_errors=True)
                logging.info("delete directory %s", channel_dir)
            except OSError as exp:
                logging.error(exp)
                return False
            return True

        logging.error("Directory %s not exist.", channel_dir)
        return False

    def get_channel_name(self, app_id, channel_id):
        """
        Description: API for getting a frame info
        Input:
            app_id: a app id
            channel_id: a channel id
        Returns: channel_name
        """
        try:
            channel_name_file = os.path.join(self.storage_dir,
                                             app_id, CHANNEL_NAME_FILE)
            if os.path.isfile(channel_name_file):
                with open(channel_name_file, 'r') as json_f:
                    json_dict = json.load(json_f)
                    if json_dict.get(channel_id):
                        channel_path = json_dict[channel_id]
                        return os.path.basename(channel_path)
                    return None
            return None
        except (OSError, JSONDecodeError) as exp:
            return None

    def _get_sub_dir_list(self, dir):
        sub_dir_list = os.listdir(dir)
        sub_dir_list = list(map(int, sub_dir_list))
        sub_dir_list.sort()
        return list(map(str, sub_dir_list))

    def present_frame(self, app_id, channel_id, frame_id=None):
        """
        Description: API for getting a frame info
        Input:
            app_id: a app id
            channel_id: a channel id
            frame_id: a frame id
        Returns: True or False
            {
                original_image: data
                car_list:[{
                    car_id:value,
                    car_confidence:value,
                    car_image:data,
                    color: value,
                    color_confidence:value,
                    brand:value,
                    brand_confidence:value}]
                bus_list:[{
                    bus_id:value,
                    bus_confidence:value,
                    bus_image:data,
                    color: value,
                    color_confidence:value,
                    brand:value,
                    brand_confidence:value}]
                person_list:[{}]
            }
        """
        if frame_id is None or frame_id == "":
            frame_num = 0
        else:
            frame_num = int(frame_id)

        # compute frame directory
        channel_dir = os.path.join(self.storage_dir, app_id, channel_id)
        index_stack = frame_num // MAX_SUB_DIRECTORY_NUM
        index_frame = frame_num % MAX_SUB_DIRECTORY_NUM
        stack = "stack_{}".format(index_stack)
        stack_dir = os.path.join(channel_dir, stack)
        if not os.path.exists(stack_dir):
            logging.error("stack directory:%s not exist!", stack_dir)
            return None
        frame_list = self._get_sub_dir_list(stack_dir)
        if index_frame >= len(frame_list):
            logging.error("frame index:%s exceed max:%s",
                          index_frame, len(frame_list))
            return None
        frame = frame_list[index_frame]
        frame_dir = os.path.join(stack_dir, frame)
        frame_info = self._create_frame_info()
        try:
            original_image = os.path.join(frame_dir, IMAGE_FILE)
            frame_info["original_image"] = open(original_image, 'rb').read()
        except OSError as exp:
            logging.error(exp)
            return None

        car_list = [i for i in os.listdir(frame_dir) if i[:3] == "car"]
        for i in car_list:
            car_info = self._extract_vehicle_info(frame_dir, i)
            if car_info is not None:
                frame_info["car_list"].append(car_info)

        bus_list = [i for i in os.listdir(frame_dir) if i[:3] == "bus"]
        for i in bus_list:
            bus_info = self._extract_vehicle_info(frame_dir, i)
            if bus_info is not None:
                frame_info["bus_list"].append(bus_info)

        person_list = [i for i in os.listdir(frame_dir) if i[:3] == "per"]
        for i in person_list:
            person_info = self._extract_human_property_info(frame_dir, i)
            if person_info is not None:
                frame_info["person_list"].append(person_info)

        return frame_info

    def _create_frame_info(self):
        """
        Description: init frame_info
        Input: NA
        Returns: null frame_info
        """
        frame_info = {}
        frame_info["original_image"] = None
        frame_info["car_list"] = []
        frame_info["bus_list"] = []
        frame_info["person_list"] = []

        return frame_info

    def _extract_vehicle_info(self, frame_dir, vehicle_id):
        """
        Description: extract vehicle info
        Input:
            frame_dir: a frame directory
            vehicle_id: the index of vehicle
        Returns: vehicle info
        """
        try:
            vehicle_info = {}
            vehicle_info["id"] = vehicle_id
            image_path = os.path.join(frame_dir, vehicle_id, IMAGE_FILE)
            if not os.path.isfile(image_path):
                logging.warning("object image:%s lost!", image_path)
                return None

            vehicle_info["image"] = open(image_path, 'rb').read()
            json_path = os.path.join(frame_dir, vehicle_id, JSON_FILE)
            if not os.path.exists(json_path):
                logging.warning("json file:%s lost!", json_path)
                return vehicle_info

            with open(json_path, 'r') as json_f:
                load_dict = json.load(json_f)
                vehicle_info = dict(vehicle_info, **load_dict)
            return vehicle_info
        except (OSError, JSONDecodeError) as exp:
            logging.error(exp)
            return None

    def _extract_human_property_info(self, frame_dir, person_id):
        """
        Description: extract human property info
        Input:
            frame_dir: a frame directory
            person_id: the index of person
        Returns: person info
        """
        try:
            person_info = {}
            person_info["id"] = person_id
            image_path = os.path.join(frame_dir, person_id, IMAGE_FILE)
            if not os.path.isfile(image_path):
                logging.warning("object image:%s lost!", image_path)
                return None
            person_info["image"] = open(image_path, 'rb').read()
            json_path = os.path.join(frame_dir, person_id, JSON_FILE)
            if not os.path.exists(json_path):
                logging.warning("json file:%s lost!", json_path)
                return person_info

            with open(json_path, 'r') as json_f:
                load_dict = json.load(json_f)
                person_info = dict(person_info, **load_dict)

            return person_info
        except (OSError, JSONDecodeError) as exp:
            logging.error(exp)
            return None

def run():
    '''Video Analysis server startup function'''
    # read config file
    config = ConfigParser()

    # config log
    log_file_path = os.path.join(ConfigParser.root_path, "config/logging.conf")
    fileConfig(log_file_path)
    logging.getLogger('video_analysis')

    if not config.config_verify():
        return None

    server = VideoAnalysisServer(config)
    VideoAnalysisManager(server)
    return server
