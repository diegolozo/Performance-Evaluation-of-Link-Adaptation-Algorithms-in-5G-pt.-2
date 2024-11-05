#!/bin/bash

## Install Prerequisites for ns-3
cd ~
sudo apt update
sudo apt install g++ -y
sudo apt-get install build-essential libssl-dev -y
sudo apt install git -y
wget https://github.com/Kitware/CMake/releases/download/v3.30.3/cmake-3.30.3.tar.gz
tar -zxvf cmake-3.30.3.tar.gz
cd ~/cmake-3.30.3/
chmod +x bootstrap
bash bootstrap
make
sudo make install
cd ~
sudo apt-get install ninja-build -y

## Install and build ns-3.41
cd ~
git clone https://gitlab.com/nsnam/ns-3-dev.git
echo 'PATH="~/ns-3-dev:$PATH"' >> ~/.bashrc
source ~/.bashrc
cd ns-3-dev
git checkout -b ns-3.41-release ns-3.41
ns3 configure --enable-examples --enable-tests
ns3 build
./test.py

## Install and build 5G-LENA
cd ~
sudo apt-get install libc6-dev -y
sudo apt-get install sqlite sqlite3 libsqlite3-dev -y
sudo apt-get install libeigen3-dev -y
cd ~/ns-3-dev/contrib
git clone https://gitlab.com/cttc-lena/nr.git
cd nr
git checkout -b 5g-lena-v3.0.y origin/5g-lena-v3.0.y
ns3 configure --enable-examples --enable-tests
ns3 build

## Install and build ns3-ai
cd ~
sudo apt install libboost-all-dev -y
sudo apt install libprotobuf-dev protobuf-compiler -y
sudo apt install pybind11-dev -y
sudo apt install python3-pip -y
cd ~/ns-3-dev/
git clone https://github.com/hust-diangroup/ns3-ai.git contrib/ai
ns3 configure --enable-examples
ns3 build ai
pip install -e contrib/ai/python_utils
pip install -e contrib/ai/model/gym-interface/py

echo ------------------------------------------------
echo INSTALLATION OF PREREQUISITES FOR ns-3 COMPLETED !
echo

## Install QUIC module for ns-3.41
# https://github.com/signetlabdei/quic/tree/release-3-41
echo To Install QUIC module for ns-3.41, see the tutorial in this repository, you have to change manually some files.
echo https://github.com/signetlabdei/quic/tree/release-3-41
echo Remember to checkout to release-3-41 branch after cloning the QUIC repository


