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
'''common installation'''
import os
import platform
import signal
import subprocess
import time
import getpass
import json
import pexpect
from collections import OrderedDict
import re
import datetime

#ssh expect prompt
PROMPT = ['# ', '>>> ', '> ', '\$ ']

FFMPEG_CONFIGURE_OPTIONS = " --cross-prefix={cross_prefix} --enable-pthreads --enable-cross-compile --target-os=linux --arch=aarch64 --enable-shared --enable-network --enable-protocol=tcp --enable-protocol=udp --enable-protocol=rtp --enable-demuxer=rtsp --disable-debug --disable-stripping --disable-doc --disable-ffplay --disable-ffprobe --disable-htmlpages --disable-manpages --disable-podpages  --disable-txtpages --disable-w32threads --disable-os2threads {sysroot} "

FFMPEG_CONFIGURE_PREFIX = " --prefix="

FFMPEG_ENGINE_SETTING_LINK = ["-lavcodec \\","-lavformat \\","-lavdevice \\","-lavutil \\","-lswresample \\","-lavfilter \\","-lswscale \\"]

FFMPEG_ENGINE_SETTING_INCLUDE = ["-I$(HOME)/ascend_ddk/include/third_party/ffmpeg \\"]

CURRENT_PATH = os.path.dirname(
    os.path.realpath(__file__))

def execute(cmd, timeout=3600, cwd=None):
    '''execute os command'''
    print(cmd)
    is_linux = platform.system() == 'Linux'

    if not cwd:
        cwd = os.getcwd()
    process = subprocess.Popen(cmd, cwd=cwd, bufsize=32768, stdout=subprocess.PIPE,
                               stderr=subprocess.STDOUT, shell=True,
                               preexec_fn=os.setsid if is_linux else None)

    t_beginning = time.time()

    # cycle times
    time_gap = 0.01

    str_std_output = ""
    while True:

        str_out = str(process.stdout.read().decode())
        str_std_output = str_std_output + str_out

        if process.poll() is not None:
            break
        seconds_passed = time.time() - t_beginning

        if timeout and seconds_passed > timeout:

            if is_linux:
                os.kill(process.pid, signal.SIGTERM)
            else:
                process.terminate()
            return False, process.stdout.readlines()
        time.sleep(time_gap)
    str_std_output = str_std_output.strip()

    # print(str_std_output)
    
    std_output_lines_last = []
    std_output_lines = str_std_output.split("\n")
    for i in std_output_lines:
        std_output_lines_last.append(i)

    if process.returncode != 0 or "Traceback" in str_std_output:
        return False, std_output_lines_last

    return True, std_output_lines_last

def scp_file_to_remote(user, ip, port, password, local_file, remote_file):
    '''do scp file to remote node'''
    cmd = "scp -P {port} {local_file} {user}@{ip}:{remote_file} ".format(
        ip=ip, port=port, user=user, local_file=local_file, remote_file=remote_file)
    print(cmd)
    try:
        process = pexpect.spawn(cmd)

        ret = process.expect(
            ["[Pp]assword", "Are you sure you want to continue connecting"])
        if ret == 1:
            process.sendline("yes")
            print(process.before)
            ret = process.expect("[Pp]assword")
        if ret != 0:
            return False

        process.sendline(password)
        process.read()
        print(process.before)
        process.expect(pexpect.EOF)
    except:
        return False
    finally:
        process.close(force=True)
    return True


def ssh_to_remote(user, ip, port, password, cmd_list):
    '''ssh to remote and execute command'''
    cmd = "ssh -p {port} {user}@{ip} ".format(ip=ip, port=port, user=user)
    print(cmd)
    try:
        process = pexpect.spawn(cmd)
        ret = process.expect(
            ["[Pp]assword", "Are you sure you want to continue connecting"])
        if ret == 1:
            process.sendline("yes")
            ret = process.expect("[Pp]assword")
        if ret != 0:
            return False

        process.sendline(password)
        process.expect(PROMPT)

        for cmd in cmd_list:
            if cmd.get("type") == "expect":
                process.expect(cmd.get("value"))
            else:
                if cmd.get("secure") is False:
                    print(cmd.get("value"))
                process.sendline(cmd.get("value"))
    except:
        return False
    finally:
        process.close(force=True)
    return True


def add_engine_setting(settings_link, settings_include):
    '''add common so configuration to ddk engine default setting file'''
    ddk_engine_config_path = os.path.join(
        os.getenv("DDK_HOME"), "conf/settings_engine.conf")
    try:
        ddk_engine_config_file = open(
            ddk_engine_config_path, 'r', encoding='utf-8')
        ddk_engine_config_info = json.load(
            ddk_engine_config_file, object_pairs_hook=OrderedDict)

        engine_info = ddk_engine_config_info.get(
            "configuration").get("OI")
            
        engine_link_obj = engine_info.get("Device").get("linkflags").get("linkobj")
        engine_include = engine_info.get("Device").get("includes").get("include")

        for each_new_setting in settings_link:
            if each_new_setting not in engine_link_obj:
                engine_link_obj.insert(-1, each_new_setting)
        for each_new_include in settings_include:
            if each_new_include not in engine_include:
                engine_include.insert(-1, each_new_include)       
                
        ddk_engine_config_new_file = open(
            ddk_engine_config_path, 'w', encoding='utf-8')
        json.dump(ddk_engine_config_info, ddk_engine_config_new_file, indent=2)
    except OSError as reason:
        print("[ERROR] Read engine setting file failed")
        exit(-1)
    finally:
        if ddk_engine_config_file in locals():
            ddk_engine_config_file.close()
        if ddk_engine_config_new_file in locals():
            ddk_engine_config_new_file.close()


def main():
    '''main function: install common so files'''
    
    ddk_engine_config_path = os.path.join(
        os.getenv("DDK_HOME"), "conf/settings_engine.conf")
    if not os.path.exists(ddk_engine_config_path):
        print("Can not find setings_engine.conf, please check DDK installation.")
        exit(-1)

    code_path = "";
    install_path = "";
    ddk_home = "";
    
    # check user want to compile and install FFmpeg
    while(True):
        user_want_ffmpeg = input("[INFO] Do you want to compile and install FFmpeg on this computer?(yes/no, default:yes):")

        # verify input path format
        if user_want_ffmpeg == "yes" or  user_want_ffmpeg == "no" or user_want_ffmpeg == "":
            break
        else:
            print("[ERROR] The input string:\"%s\" is invalid, please input \"yes\" or \"no\"!" % user_want_ffmpeg)
    
    if user_want_ffmpeg == "yes" or user_want_ffmpeg == "":
        # get FFmpeg source code path from user input
        while(True):
            code_path = input("[INFO] Please input FFmpeg source code path:")

            # verify input path format
            if re.match(r"^/((?!\.\.).)*$", code_path):
                break
            else:
                print("[ERROR] FFmpeg source code path:\"%s\" is invalid!" % code_path)

        # check the input is exist
        if not os.path.exists(code_path):
            print("[ERROR] FFmpeg source code path:\"%s\" is not exist!" % code_path)
            exit(-1)
            
        # check ffmpeg source code
        if not os.path.exists("{path}/libavcodec".format(path=code_path)):
            print("[ERROR] The path:\"%s\" does not contain the full FFmpeg code, please check the path!" % code_path)
            exit(-1)
        if not os.path.exists("{path}/libavformat".format(path=code_path)):
            print("[ERROR] The path:\"%s\" does not contain the full FFmpeg code, please check the path!" % code_path)
            exit(-1)
        if not os.path.exists("{path}/configure".format(path=code_path)):
            print("[ERROR] The path:\"%s\" does not contain the full FFmpeg code, please check the path!" % code_path)
            exit(-1)
    
        # check ddk home and configure file
        ddk_home = os.getenv("DDK_HOME")
        if not ddk_home or ddk_home == "":
            print("[ERROR] The environment variable DDK_HOME has no value, please set it!")
            exit(-1)
        ddk_engine_config_path = os.path.join(
            os.getenv("DDK_HOME"), "conf/settings_engine.conf")
        if not os.path.exists(ddk_engine_config_path):
            print("[ERROR] Can not find setings_engine.conf, please check DDK installation.")
            exit(-1)
        
        # compile and install ffmpeg
        print("[INFO] FFmpeg installation is beginning, the process will takes several minutes, please wait a while.")
          
        cross_prefix = "/usr/bin/aarch64-linux-gnu-"
        sysroot = ""

        ffmpeg_build_options = FFMPEG_CONFIGURE_OPTIONS.format(cross_prefix=cross_prefix, sysroot=sysroot)
        
        install_path = "{path}/install_path".format(path=code_path)        
        execute("mkdir -p {path}".format(path=install_path))
        execute("make clean -C {path}".format(path=code_path))
        execute("cd {path1} && bash {path1}/configure {options}{prefix}{path2}".format(path1=code_path,options=ffmpeg_build_options,prefix=FFMPEG_CONFIGURE_PREFIX,path2=install_path))
        execute("make -j 8 -C {path}".format(path=code_path))
        execute("make install -C {path}".format(path=code_path))
    
        # check ffmpeg install result
        if not os.path.exists("{path}/include".format(path=install_path)):
            print("[ERROR] FFmpeg install failed! Please check that the FFmpeg code is complete, the version is correct, and the permissions are correct. Then try again.")
            exit(-1)
        if not os.path.exists("{path}/lib".format(path=install_path)):
            print("[ERROR] FFmpeg install failed! Please check that the FFmpeg code is complete, the version is correct, and the permissions are correct. Then try again.")
            exit(-1)
        
        # copy ffmpeg head files to ascend ddk home
        ascend_ddk_home = os.path.join(
            os.getenv("HOME"), "ascend_ddk")
        execute("mkdir -p {path}/include/third_party/ffmpeg".format(path=ascend_ddk_home))
        execute("cp -rdp {path1}/include/* {path2}/include/third_party/ffmpeg".format(path1=install_path,path2=ascend_ddk_home))
        execute("cp -rdp {path1}/lib/* {path2}/device/lib".format(path1=install_path,path2=ascend_ddk_home))

        # adding engine setting in ddk configuration
        print("[INFO] Adding engine setting in DDK configuration.")
        engine_settings_link = []
        for each_engine_link in FFMPEG_ENGINE_SETTING_LINK:
            engine_settings_link.append(each_engine_link)
        engine_settings_include = []
        for each_engine_include in FFMPEG_ENGINE_SETTING_INCLUDE:
            engine_settings_include.append(each_engine_include)
        add_engine_setting(engine_settings_link, engine_settings_include)    
            
        print("[INFO] FFmpeg installation is finished.")
    
    mode = "Atlas DK Development Board"
    # check user want to install ffmpeg on Atlas
    while(True):
        atlas_want_ffmpeg = input("[INFO] Do you want to install FFmpeg on %s?(yes/no, default:yes):" % mode)

        # verify input path format
        if atlas_want_ffmpeg == "yes" or  atlas_want_ffmpeg == "no" or atlas_want_ffmpeg == "":
            break
        else:
            print("[ERROR] The input string:\"%s\" is invalid, please input \"yes\" or \"no\"!" % atlas_want_ffmpeg)
    
    if atlas_want_ffmpeg == "yes" or atlas_want_ffmpeg == "":    
        if code_path == "":
            # get FFmpeg source code path from user input
            while(True):
                code_path = input("[INFO] Please input FFmpeg source code path:")

                # verify input path format
                if re.match(r"^/((?!\.\.).)*$", code_path):
                    break
                else:
                    print("[ERROR] FFmpeg source code path:\"%s\" is invalid!" % code_path)            
        
            install_path = "{path}/install_path".format(path=code_path)
            
            # check ffmpeg install result
            if not os.path.exists("{path}/include".format(path=install_path)):
                print("[ERROR] The FFmpeg does not installed in the path:\"%s\"!" % code_path)
                exit(-1)
            if not os.path.exists("{path}/lib".format(path=install_path)):
                print("[ERROR] The FFmpeg does not installed in the path:\"%s\"!" % code_path)
                exit(-1)
        

        # clear compress package if exist
        if os.path.exists("{path}/lib/ffmpeg_lib.tar".format(path=install_path)):
            execute("rm -f {path}/lib/ffmpeg_lib.tar".format(path=install_path))
        if os.path.exists("{path}/lib/ffmpeg_lib.tar.gz".format(path=install_path)):
            execute("rm -f {path}/lib/ffmpeg_lib.tar.gz".format(path=install_path))
            
        # compress ffmpeg so file 
        execute("cd {path}/lib && tar -cvf ffmpeg_lib.tar ./*".format(path=install_path))
        execute("cd {path}/lib && gzip ffmpeg_lib.tar".format(path=install_path))
        ffmpeg_pakage = "{path}/lib/ffmpeg_lib.tar.gz".format(path=install_path);
    
        # transmit ffmpeg packet to Atlas
        while(True):
            developer_ip = input("[INFO] Please input %s IP:" % mode)

            if re.match(r"^(?:(?:25[0-5]|2[0-4][0-9]|[01]?[0-9][0-9]?)\.){3}(?:25[0-5]|2[0-4][0-9]|[01]?[0-9][0-9]?)$", developer_ip):
                break
            else:
                print("[ERROR] The user input IP: %s is invalid!" % developer_ip)
        developer_ssh_user = input(
            "[INFO] Please input %s SSH user(default: HwHiAiUser):" % mode)
        if developer_ssh_user == "":
            developer_ssh_user = "HwHiAiUser"

        developer_ssh_pwd = getpass.getpass(
            "[INFO] Please input %s SSH user password:" % mode)

        developer_ssh_port = input("[INFO] Please input %s SSH port(default: 22):" % mode)
        if developer_ssh_port == "":
            developer_ssh_port = "22"

        if developer_ssh_user != "root":
            developer_root_pwd = getpass.getpass(
                "[INFO] Please input %s root user password:" % mode)

        print("[INFO] Transmit ffmpeg package is beginning.")
        
        # mkdir temp path on Atlas, used for store FFmpeg package
        now_time = datetime.datetime.now().strftime('scp_lib_%Y%m%d%H%M%S')
        mkdir_expect = PROMPT
        mkdir_expect.append("File exists")
        ret = ssh_to_remote(developer_ssh_user, developer_ip, developer_ssh_port,
                      developer_ssh_pwd, [{"type": "cmd",
                                         "value": "mkdir ./{scp_lib}".format(scp_lib=now_time),
                                         "secure": False},
                                        {"type": "expect",
                                         "value": mkdir_expect}])
        if not ret:
            print("[ERROR] Mkdir dir in %s failed, please check your env and input." % developer_ip)
            exit(-1)        
        
        # transmit FFmpeg package to Atlas
        ret = scp_file_to_remote(developer_ssh_user, developer_ip, developer_ssh_port,
                               developer_ssh_pwd, ffmpeg_pakage, "./{scp_lib}".format(scp_lib=now_time))
        if not ret:
            print("[ERROR] Scp ffmpeg package to %s is failed, please check your env and input." % developer_ip)
            exit(-1)
            
        # decompress ffmpeg package on Atlas
        cmd_list = []
        if developer_ssh_user != "root":
            cmd_list.extend([{"type":"cmd",
                              "value":"su root",
                              "secure":False},
                             {"type":"cmd",
                              "value":developer_root_pwd,
                              "secure":True},
                             {"type":"expect",
                              "value":PROMPT}])
        cmd_list.extend([{"type": "cmd",
                         "value": "chmod -R 644 ./{scp_lib}/ffmpeg_lib.tar.gz".format(scp_lib=now_time),
                         "secure": False},
                        {"type": "expect",
                         "value": PROMPT},
                        {"type": "cmd",
                         "value": "cd ./{scp_lib} && tar -zxvf ffmpeg_lib.tar.gz && cd ..".format(scp_lib=now_time),
                         "secure": False},
                        {"type": "expect",
                         "value": PROMPT},
                        {"type": "cmd",
                         "value": "chown -R root:root ./{scp_lib}/*".format(scp_lib=now_time),
                         "secure": False},
                        {"type": "expect",
                         "value": PROMPT},
                        {"type": "cmd",
                         "value": "cp -rdp  ./{scp_lib}/* /usr/lib64".format(scp_lib=now_time),
                         "secure": False},
                        {"type": "expect",
                         "value": PROMPT},
                        {"type": "cmd",
                         "value": "rm -rf ./{scp_lib}".format(scp_lib=now_time),
                         "secure": False},
                        {"type": "expect",
                         "value": PROMPT},
                        {"type": "cmd",
                         "value": "rm -f /usr/lib64/ffmpeg_lib.tar.gz",
                         "secure": False},
                        {"type": "expect",
                         "value": PROMPT},
                        {"type": "cmd",
                         "value": "ldconfig",
                         "secure": False},
                        {"type": "expect",
                         "value": PROMPT}])
        ret = ssh_to_remote(developer_ssh_user, developer_ip, developer_ssh_port,
                      developer_ssh_pwd, cmd_list)
        if not ret:
            print("[ERROR] Installation ffmpeg on Atlas is failed.")
            exit(-1)
    
        # clear compress package if exist
        if os.path.exists("{path}/lib/ffmpeg_lib.tar".format(path=install_path)):
            execute("rm -f {path}/lib/ffmpeg_lib.tar".format(path=install_path))
        if os.path.exists("{path}/lib/ffmpeg_lib.tar.gz".format(path=install_path)):
            execute("rm -f {path}/lib/ffmpeg_lib.tar.gz".format(path=install_path))
        
    print("[INFO] All steps are completed.")

if __name__ == '__main__':
    main()
