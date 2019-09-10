# Tiger Compiler Labs in C++

## Contents

- [Tiger Compiler Labs in C++](#tiger-compiler-labs-in-c)
  - [Contents](#contents)
  - [Overview](#overview)
  - [Difference Between C Labs and C++ Labs](#difference-between-c-labs-and-c-labs)
  - [Getting Newly Released Labs](#getting-newly-released-labs)
  - [Installing Dependencies](#installing-dependencies)
    - [Ubuntu](#ubuntu)
    - [MacOS or Windows](#macos-or-windows)
  - [Compiling](#compiling)
  - [Debugging](#debugging)
  - [Grading Your Labs](#grading-your-labs)
  - [Submitting Your Labs](#submitting-your-labs)

## Overview

We rewrote the Tiger Compiler labs using C++ programming language. This is because C++ has some features like inheritance and polymorphism, which we think is more suitable for these labs and less error-prone.

We provide you all the codes of all labs in one time. In each lab, you only need to code in some of the directories.

The Tiger Compiler Labs are not perfect, but we have tried our best and spent a lot of time on it. If you find any bugs or have a better design, please inform us.

## Difference Between C Labs and C++ Labs

1. This new labs use [flexc++](https://fbb-git.gitlab.io/flexcpp/manual/flexc++.html) and [bisonc++](https://fbb-git.gitlab.io/bisoncpp/manual/bisonc++.html) instead of flex and bison because flexc++ and bisonc++ is more flexc++ and bisonc++ are able to generate pure C++ codes instead of C codes wrapped in C++ files.

2. The new labs use namespace for modularization and use inheritance and polymorphism to replace unions used in the old labs.

3. This new labs use CMake instead of Makefile to compile and build the target.

## Getting Newly Released Labs

```bash
git clone https://ipads.se.sjtu.edu.cn:1312/lab/tiger-compiler-2019-fall.git
```

**Note:** We may update the framework codes later so you may need to do some code merging.

## Installing Dependencies

flexc++ and bisonc++ will be needed in lab2 and later.
Although these libraries are not needed in lab1, you have to install them before you start lab1.

### Ubuntu

```bash
sudo apt install cmake g++ gcc gdb flexc++ bisonc++
```

### MacOS or Windows

For students who use MacOS or Windows, we provide you a Docker image which has already installed all the dependencies. You can compile your codes directly in this Docker image.

1. Install [Docker](https://docs.docker.com/).

2. Build the docker image using the Dockerfile that we provide.

    ```bash
    cd tiger-compiler-2019-fall
    docker build -t se302/tigerlabs_env .
    ```

3. Run a docker container and mount the lab directory on it.

    ```bash
    docker run -it -v /path/to/tiger-compiler-2019-fall:/home/stu/tiger-compiler-2019-fall se302/tigerlabs_env:latest /bin/bash
    cd tiger-compiler-2019-fall
    ```

## Compiling

```bash
mkdir build
cd build
cmake ..
make test_xxx  # e.g. `make test_slp`
```

## Debugging

```bash
# in build direcotry
cmake -DCMAKE_BUILD_TYPE=Debug ..
make test_xxx # e.g. `make test_slp`
gdb test_xxx # e.g. `gdb test_slp`
```

## Grading Your Labs

```bash
# cd to the lab root directory, i.e. tiger-compiler-2019-fall
./gradelabx.sh # e.g. `./gradelab1.sh`
```

## Submitting Your Labs

```bash
./handin.sh
```
