FROM ubuntu:18.04

# Use aliyun registry
RUN  sed -i s@/archive.ubuntu.com/@/mirrors.aliyun.com/@g /etc/apt/sources.list
RUN  apt-get clean

RUN apt-get update && apt-get install -y git cmake g++ gcc vim tar gdb flexc++ bisonc++

RUN useradd -ms /bin/bash stu

USER stu

WORKDIR /home/stu
