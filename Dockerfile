FROM python:3.7

WORKDIR /usr/src/app
ADD . /usr/src/app

RUN apt-get update
RUN apt-get -y install apt-transport-https libtbb-dev cmake

RUN python3 -m pip install numpy

RUN wget https://dl.bintray.com/boostorg/release/1.67.0/source/boost_1_67_0.tar.gz
RUN tar xfzv boost_1_67_0.tar.gz
RUN cd boost_1_67_0 && CPLUS_INCLUDE_PATH="$CPLUS_INCLUDE_PATH:/usr/local/include/python3.7m" C_INCLUDE_PATH="$C_INCLUDE_PATH:/usr/local/include/python3.7m" ./bootstrap.sh --with-python=python3 
RUN cd boost_1_67_0 && CPLUS_INCLUDE_PATH="$CPLUS_INCLUDE_PATH:/usr/local/include/python3.7m" C_INCLUDE_PATH="$C_INCLUDE_PATH:/usr/local/include/python3.7m" ./b2 -j4 install

RUN ln -s /usr/local/lib/libboost_python37.so /usr/local/lib/libboost_python.so
RUN ls -al /usr/local/lib

RUN python3 -m pip install codecov nose mock
RUN python3 -m pip install -r requirements.txt

RUN make test
RUN codecov --token 10cff21a-48fa-4326-9714-fd63aa7c785d

