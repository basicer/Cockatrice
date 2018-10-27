FROM ubuntu:latest
MAINTAINER Gavin Bisesi <Daenyth@gmail.com>

RUN cat /etc/os-release
RUN grep PRETTY_NAME /etc/os-release
RUN echo $PRETTY_NAME
RUN source /etc/os-release | echo $NAME $VERSION
RUN source /etc/os-release
RUN export $NAME
RUN echo $NAME
RUN sh /etc/os-release | export $NAME | echo $NAME
RUN grep PRETTY_NAME /etc/os-release | cut -c 13-
RUN grep NAME /etc/os-release | cut -c 6-
RUN grep VERSION /etc/os-release | cut -c 9-
RUN grep PRETTY_NAME /etc/os-release | cut -d= -f2
RUN apt update && apt install -y software-properties-common
RUN apt update && apt install -y\
  build-essential\
  g++\
  cmake\
  git\
  libprotobuf-dev\
  protobuf-compiler\
  qt5-default\
  qttools5-dev\
  qttools5-dev-tools\
  libqt5sql5-mysql\
  libqt5websockets5-dev

ENV dir /home/servatrice/code
WORKDIR $dir
COPY LICENSE LICENSE
COPY CMakeLists.txt CMakeLists.txt
COPY cmake/ cmake/
COPY common/ common/
COPY servatrice/ servatrice/
COPY README.md README.md

WORKDIR build
RUN cmake .. -DWITH_SERVER=1 -DWITH_CLIENT=0 -DWITH_ORACLE=0 &&\
  make &&\
  make install

WORKDIR /home/servatrice

EXPOSE 4747

ENTRYPOINT [ "servatrice" ]
