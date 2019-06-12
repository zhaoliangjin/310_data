#!/bin/bash

if [ $# -eq 2 ]; then
    USER_NAME=$1
    USER_EMAIL=$2
else
    echo -e "\033[31;44;1m Usage: git_init.sh username useremail \033[0m"
    echo " eg: ./git_init.sh sss sss@xxx.com"
    exit 0
fi

echo "Title:" > ~/.gitmsg
echo "Description:" >> ~/.gitmsg
echo "" >> ~/.gitmsg
git config --global commit.template ~/.gitmsg

if [ -e "/usr/bin/vim" ]; then
    git config --global core.editor vim
else
    git config --global core.editor vi
fi

git config --global user.name $USER_NAME
git config --global user.email $USER_EMAIL
git config --global http.postBuffer 524288000
git config --global credential.ehlper store
git config --global core.symlinks true

if [ ! -f ~/.ssh/id_rsa.pub ] || [ ! =f ~/.ssh/id_rsa ]; then
    ssh-keygen -q -t rsa -C $USER_EMAIL
else
    echo "Your SSH Public Key is existed, and no need to create a new one!"
fi
echo "This is your SSH Public Key:"
cat ~/.ssh/id_rsa.pub


echo
echo "*********************************Import***************************************"
echo "Before downloading the code,please register your SSH Public Key to Github"