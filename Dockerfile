FROM python:3.7.9 AS base
RUN apt-get update \
  && apt-get install -y ssh \
      build-essential \
      gcc \
      cmake \
      g++ \
      gdb \
      clang \
      rsync \
      tar \
      wget \
      nasm \
  && apt-get clean


RUN pip install "pybind11[global]" aiohttp rx wheel \
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

ARG ARCH=aarch64
ENV ARCH_ENV=$ARCH
ENV MVCAM_COMMON_RUNENV=/opt/MVS/lib
ENV LD_LIBRARY_PATH=/opt/MVS/lib/${ARCH_ENV}


FROM base AS development
RUN ( \
    echo 'PermitRootLogin yes'; \
    echo 'PasswordAuthentication yes'; \
    echo 'Subsystem sftp /usr/lib/openssh/sftp-server'; \
  ) > /etc/ssh/sshd_config_test_clion \
  && mkdir /run/sshd

RUN echo 'root:Docker!' | chpasswd
VOLUME /etc/ssh
CMD ["/usr/sbin/sshd", "-D", "-e", "-f", "/etc/ssh/sshd_config_test_clion"]


FROM base AS build
WORKDIR /root/hikvisionbroker-extension
ADD module.cpp camera.h camera.cpp CMakeLists.txt setup.py ./
ADD /extra/MVS.tar.gz /opt
RUN python setup.py bdist_wheel && rm /opt/MVS -rf
# CMD python server/server.py

FROM python:3.7.9 AS release
WORKDIR /root/hikvisionbroker-extension
RUN pip install aiohttp rx
COPY --from=build /root/hikvisionbroker-extension/dist/hikvisionbroker_extension-0.0.1-cp37-cp37m-linux_aarch64.whl hikvisionbroker_extension-0.0.1-cp37-cp37m-linux_aarch64.whl
RUN pip install hikvisionbroker_extension-0.0.1-cp37-cp37m-linux_aarch64.whl
CMD python server/server.py
