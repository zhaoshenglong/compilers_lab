# Tiger Compiler Labs

## Contents

- [Tiger Compiler Labs](#tiger-compiler-labs)
  - [Contents](#contents)
  - [Getting Newly Released Labs](#getting-newly-released-labs)
  - [Installing Dependencies](#installing-dependencies)
    - [Ubuntu](#ubuntu)
    - [MacOS or Windows](#macos-or-windows)
  - [Compiling](#compiling)
  - [Grading Your Labs](#grading-your-labs)
  - [Submitting Your Labs](#submitting-your-labs)

## Getting Newly Released Labs

```bash
git clone https://ipads.se.sjtu.edu.cn:1312/lab/tiger-compiler-2019-fall.git
```

## Installing Dependencies

### Ubuntu

```bash
sudo apt install flexc++ bisonc++
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

## Grading Your Labs

```bash
# cd to the lab root directory, i.e. tiger-compiler-2019-fall
./gradelabx.sh # e.g. `./gradelab1.sh`
```

## Submitting Your Labs

```bash
./handin.sh
```
