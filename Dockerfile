FROM python:3.7.9
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
  && apt-get clean

RUN wget https://github.com/Kitware/CMake/releases/download/v3.17.5/cmake-3.17.5-Linux-x86_64.sh \
  && bash cmake-3.17.5-Linux-x86_64.sh --prefix=/usr/local --skip-license \
  && rm cmake-3.17.5-Linux-x86_64.sh

RUN pip install "pybind11[global]" \
  && git clone --recursive https://github.com/ReactiveX/RxCpp.git \
  && cd RxCpp \
  && mkdir build \
  && cd build \
  && cmake .. \
  && make install \
  && make clean \
  && cd ../../ \
  && rm RxCpp -rf

RUN ( \
    echo 'PermitRootLogin yes'; \
    echo 'PasswordAuthentication yes'; \
    echo 'Subsystem sftp /usr/lib/openssh/sftp-server'; \
  ) > /etc/ssh/sshd_config_test_clion \
  && mkdir /run/sshd

RUN echo 'root:Docker!' | chpasswd

ENV MVCAM_COMMON_RUNENV=/opt/MVS/lib
ENV LD_LIBRARY_PATH=/opt/MVS/lib/64
CMD ["/usr/sbin/sshd", "-D", "-e", "-f", "/etc/ssh/sshd_config_test_clion"]
