FROM ubuntu:18.04

RUN apt-get update && apt-get install -y git cmake gdb flexc++ bisonc++

RUN useradd -ms /bin/bash stu

USER stu

WORKDIR /home/stu
