FROM python:3.7.9 AS base
RUN apt-get update \
  && apt-get install -y ssh \
      build-essential \
      gcc \
      g++ \
      gdb \
      clang \
      rsync \
      tar \
      wget \
      nasm \
  && apt-get clean

RUN wget https://github.com/Kitware/CMake/releases/download/v3.17.5/cmake-3.17.5-Linux-x86_64.sh \
  && bash cmake-3.17.5-Linux-x86_64.sh --prefix=/usr/local --skip-license \
  && rm cmake-3.17.5-Linux-x86_64.sh

RUN pip install "pybind11[global]" aiohttp rx \
  && git clone --recursive https://github.com/ReactiveX/RxCpp.git \
  && cd RxCpp \
  && mkdir build \
  && cd build \
  && cmake .. \
  && make install \
  && make clean \
  && cd ../../ \
  && rm RxCpp -rf

RUN git clone --recursive https://github.com/libjpeg-turbo/libjpeg-turbo.git \
  && cd libjpeg-turbo \
  && mkdir build \
  && cd build \
  && cmake -DCMAKE_INSTALL_PREFIX=/usr/local .. \
  && make -j8 \
  && make install \
  && make clean \
  && cd ../.. \
  && rm libjpeg-turbo -rf \
  && ldconfig

ENV MVCAM_COMMON_RUNENV=/opt/MVS/lib
ENV LD_LIBRARY_PATH=/opt/MVS/lib/64


FROM base AS development
RUN ( \
    echo 'PermitRootLogin yes'; \
    echo 'PasswordAuthentication yes'; \
    echo 'Subsystem sftp /usr/lib/openssh/sftp-server'; \
  ) > /etc/ssh/sshd_config_test_clion \
  && mkdir /run/sshd

RUN echo 'root:Docker!' | chpasswd

CMD ["/usr/sbin/sshd", "-D", "-e", "-f", "/etc/ssh/sshd_config_test_clion"]


FROM base AS release
WORKDIR /root/hikvisionbroker-extension
ADD . .
ADD /extra/MVS.tar.gz /opt
RUN python setup.py install && rm /opt/MVS -rf
CMD python server/server.py