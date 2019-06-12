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
'''warn check'''
import json
import os
import re
import sys

from concurrent.futures import ProcessPoolExecutor
from concurrent.futures import as_completed

THIS_FILE_NAME = __file__

sys.path.append(os.path.join(os.path.dirname(
    os.path.realpath(THIS_FILE_NAME)), ".."))

import comm.ci_log as cilog
import comm.util as util
import scripts_util as sc_util


def single_warn_check_compile(cmd, mind_file, oi_engine_config_dict):
    '''single warm_check path in compile mode'''
    mind_file_paths = os.path.split(mind_file)
    mind_file_path = mind_file_paths[0]

    try:
        mind_file_stream = open(mind_file, 'r')
        mind_file_info = json.load(mind_file_stream)
        mind_nodes = mind_file_info.get("node")
    except OSError as reason:
        cilog.cilog_error(
            THIS_FILE_NAME, "read %s failed: %s", mind_file, reason)
        return False

    result = True
    checked_file_path = ""
    for mind_node in mind_nodes:
        if mind_node.get("group") == "MyModel":
            continue

        name = mind_node.get("name")
        run_side = mind_node.get("params").get("runSide")

        checked_file_path = os.path.join(mind_file_path, name)

        header_list = oi_engine_config_dict.get(
            run_side.lower()).get("includes").get("include")
        header_list = list(map(lambda x:
                               re.sub(r"\$\(SRC_DIR\)", checked_file_path, x), header_list))
        header_list = list(map(lambda x:
                               re.sub(r"\.\.", mind_file_path, x), header_list))
        header_list = list(map(lambda x:
                               re.sub(r" \\", "", x), header_list))
        header_list = list(map(lambda x:
                               re.sub(r"\$\(DDK_HOME\)", os.getenv("DDK_HOME"), x), header_list))

        replaced_cmd = re.sub("__WARN_CHECK_HEADERS__",
                              " ".join(header_list), cmd)

        checked_file_cmd = "find " + checked_file_path + \
            " -name \"*.cpp\" -o -name \"*.h\""
        ret = util.execute(checked_file_cmd, print_output_flag=True)
        if ret[0] is False:
            result = False
            continue

        checked_files = ret[1]

        for file in checked_files:
            file_path_name = os.path.split(file)
            file_names = os.path.splitext(file_path_name[1])
            file_name = file_names[0] + ".o"
            temp_cmd = re.sub(r"__WARN_CHECK_FILE__", file, replaced_cmd)
            temp_cmd = re.sub(r"__WARN_CHECK_FILE_NAME__",
                              file_name, temp_cmd)
            ret = util.execute(temp_cmd, print_output_flag=True)
            if ret[0] is False:
                result = False

    return result


def warn_check_compile(cmd):
    '''warm check in compile mode'''
    engine_mind_cmd = "find " + \
        os.path.join(sc_util.ASCEND_ROOT_PATH,
                     "ascenddk/engine") + " -name \"*.mind\""
    ret = util.execute(engine_mind_cmd, print_output_flag=True)
    if ret[0] is False:
        return False
    mind_files = ret[1]

    ddk_engine_config_path = os.path.join(
        os.getenv("DDK_HOME"), "conf/settings_engine.conf")
    try:
        ddk_engine_config_file = open(ddk_engine_config_path, 'r')
        ddk_engine_config_info = json.load(ddk_engine_config_file)
        oi_engine_config_dict = ddk_engine_config_info.get(
            "configuration").get("OI")
    except OSError as reason:
        cilog.cilog_error(
            THIS_FILE_NAME, "read ddk conf/settings_engine.conf failed: %s", reason)
        return False
    finally:
        if ddk_engine_config_file in locals():
            ddk_engine_config_file.close()

    oi_lower_dict = {k.lower(): v for k, v in oi_engine_config_dict.items()}

    result = True
    with ProcessPoolExecutor(max_workers=5) as executor:
        futures_pool = {executor.submit(
            single_warn_check_compile, cmd, mind_file, oi_lower_dict): \
            mind_file for mind_file in mind_files}
        for future in as_completed(futures_pool):
            if future.result() is False:
                result = False
    return result


def validate_makefile(makefile_path):
    '''validate -Wall in makefile'''
    result = False
    try:
        makefile_stream = open(makefile_path, "r")
        while True:
            lines = makefile_stream.readlines(1000)
            if not lines:
                break
            for line in lines:
                if "-Wall" in line:
                    result = True
                    break
    except OSError as reason:
        cilog.cilog_error(
            THIS_FILE_NAME, "read makefile %s failed: %s", makefile_path, reason)
    finally:
        if makefile_stream in locals():
            makefile_stream.close()

    if result is False:
        cilog.cilog_error(
            THIS_FILE_NAME, "makefile %s is invalid:"\
            " no -Wall compile parameters", makefile_path)
    return result


def single_warn_check_makefile(cmd, makefile_path):
    '''single warn check in makefile mode'''
    ret = validate_makefile(makefile_path)

    if ret is False:
        return False
    makefile_path = os.path.split(makefile_path)[0]
    cmd = re.sub(r"(__[\w+_\w+]*__)", makefile_path, cmd)
    ret = util.execute(cmd, print_output_flag=True)
    return ret[0]


def warn_check_makefile(cmd):
    '''warm check in makefile mode'''
    # find path which need to be checked
    ret = sc_util.find_checked_path()
    if ret[0] is False:
        return False
    checked_path = ret[1]

    if checked_path is None:
        cilog.cilog_info(THIS_FILE_NAME, "no path to check in makefile mode")
        return True

    makefile_path_list = []

    for each_path in checked_path:
        makefile_cmd = "find " + each_path + " -name \"Makefile\""
        ret = util.execute(makefile_cmd, print_output_flag=True)
        if ret[0] is False:
            return False

        makefile_path_list.extend(ret[1])
    if ""  in makefile_path_list:
        makefile_path_list.remove("")

    # base so should be executed fist in sequence and copy to DDK path
    make_install_path = "make install -C __INSTALL_MAKEFILE_PATH__"
    base_so_path = sc_util.get_base_list()
    for each_path in base_so_path:
        each_path = sc_util.replace_env(each_path)
        makefile_path_list.remove(each_path)
        ret = single_warn_check_makefile(cmd, each_path)
        if ret is False:
            return False
        temp_copy_cmd = re.sub(r"(__[\w+_\w+]*__)",
                               os.path.split(each_path)[0], make_install_path)
        ret = util.execute(temp_copy_cmd, print_output_flag=True)
        if ret[0] is False:
            return False

    if makefile_path_list is None:
        cilog.cilog_info(
            THIS_FILE_NAME, "no Makefile to check in makefile mode")
        return True

    result = True
    with ProcessPoolExecutor(max_workers=5) as executor:
        futures_pool = {executor.submit(
            single_warn_check_makefile, cmd, mind_file_path): \
            mind_file_path for mind_file_path in makefile_path_list}
        for future in as_completed(futures_pool):
            if future.result() is False:
                result = False
    return result


def warn_check(compile_cmd, makefile_cmd):
    '''warn check'''
    result = warn_check_makefile(makefile_cmd)

    ret = warn_check_compile(compile_cmd)

    if ret is False:
        result = ret

    return result


def filter_warn_check_is_none(file_name):
    '''check warning in warn check result'''
    # replace env in the file_name
    file_name = sc_util.replace_env(file_name)
    try:
        file_stream = open(file_name, 'r')
        while True:
            lines = file_stream.readlines(100000)
            if not lines:
                break
            for line in lines:
                if "warning" in line:
                    return False
    except OSError as reason:
        cilog.cilog_error(
            THIS_FILE_NAME, "read file failed: %s", reason)
        return False
    finally:
        if file_stream in locals():
            file_stream.close()
    return True
