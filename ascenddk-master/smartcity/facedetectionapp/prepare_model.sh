#!/bin/bash
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

# ************************Variable*********************************************
script_path="$( cd "$(dirname "$0")" ; pwd -P )"

tools_version=$1

function check()
{
    model_name=$1
    
    if [ -f "${script_path}/${model_name}.om" ];then
        return 0
    fi
    return 1
}

function download()
{
    model_name=$1
    model_remote_path=$2
    if [ ! -f "${script_path}/get_download_branch.sh" ];then
        wget -O ${script_path}/get_download_branch.sh "https://raw.githubusercontent.com/Ascend/models/master/get_download_branch.sh" --no-check-certificate
        if [ $? -ne 0 ];then
            echo "ERROR: download failed, please https://raw.githubusercontent.com/Ascend/models/master/get_download_branch.sh connetction."
            rm -rf ${script_path}/get_download_branch.sh
            return 1
        fi
    fi
    download_version=`/bin/bash ${script_path}/get_download_branch.sh ${tools_version}`
    if [ ! -f "${script_path}/${model_name}.om" ];then
        download_url="https://media.githubusercontent.com/media/Ascend/models/${download_version}/${model_remote_path}/${model_name}/${model_name}.om"
        wget -O ${script_path}/${model_name}.om ${download_url} --no-check-certificate
        if [ $? -ne 0 ];then
            echo "ERROR: download failed, please ${download_url} connetction."
            rm -rf ${script_path}/${model_name}.om
            return 1
        fi
    fi
    return 0
}

function prepare()
{
    model_name=$1
    model_remote_path=$2
    if [ -f "${script_path}/MyModel/${model_name}/device/${model_name}.om" ];then
        echo "${model_name} already exsits, finish to prepare successfully."
        return 0
    fi
    
    check ${model_name}
    if [ $? -eq 0 ];then
        echo "${model_name} is already downloaded, skip to prepare it."
    else
        #download
        if [ ${tools_version}"X" == "X" ];then
            echo "ERROR: tools version is empty and current path has no ${model_name}.om."
            echo "Please try: "
            echo "\tPrepare models automatically, excute:./prepare_model.sh tools_version"
            echo "\tDownload models to current path manually, and excute: ./prepare_model.sh"
            
            return 1
        fi
        download ${model_name} ${model_remote_path}
        if [ $? -ne 0 ];then
            return 1
        fi
    fi
    
    mkdir -p ${script_path}/MyModel/${model_name}/device
    cp ${script_path}/${model_name}.om ${script_path}/MyModel/${model_name}/device/
    echo "${model_name} finish to prepare successfully."
}

main()
{
    model_name="face_detection"
    model_remote_path="computer_vision/object_detect"
    prepare ${model_name} ${model_remote_path}
    if [ $? -ne 0 ];then
        exit 1
    fi
    exit 0
}

main
