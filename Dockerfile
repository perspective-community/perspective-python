FROM python:3.7

WORKDIR /usr/src/app
ADD . /usr/src/app

RUN apt-get update
RUN apt-get -y install apt-transport-https libboost-python-dev libtbb-dev cmake
RUN ls -al /usr/lib/x86_64-linux-gnu
RUN rm -f /usr/lib/libboost_python.so
RUN rm -f /usr/lib/x86_64-linux-gnu/libboost_python.so
RUN rm -f /usr/lib/libboost_python-py27*
RUN rm -f /usr/lib/x86_64-linux-gnu/libboost_python-py27*
RUN ln -s /usr/lib/x86_64-linux-gnu/libboost_python-py35.so.1.62.0 /usr/lib/x86_64-linux-gnu/libboost_python.so
RUN ln -s /usr/lib/x86_64-linux-gnu/libboost_python-py35.so.1.62.0 /usr/lib/libboost_python.so


RUN curl -sL https://deb.nodesource.com/setup_9.x | bash -
RUN apt-get install -y libboost-python-dev

RUN python3 -m pip install codecov nose mock
RUN python3 -m pip install -r requirements.txt

RUN make test
RUN codecov --token 10cff21a-48fa-4326-9714-fd63aa7c785d