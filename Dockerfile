FROM --platform=linux/amd64 ubuntu:20.04
WORKDIR /app

ARG DEBIAN_FRONTEND=noninteractive

RUN apt update && apt install -y \
        wget \
        build-essential \
        zip \
        git \
        xxd \
        libarchive-zip-perl \
    && apt-get clean && rm -rf /var/lib/apt/lists/* /tmp/* /var/tmp/*

# GCC ARM
RUN if [ $(uname -m) = "x86_64" ]; then export ARCH=x86_64; else export ARCH=aarch64; fi && \
    mkdir -p /opt/gcc && \
    cd /opt/gcc && \
    wget https://armkeil.blob.core.windows.net/developer/Files/downloads/gnu-rm/10-2020q4/gcc-arm-none-eabi-10-2020-q4-major-$ARCH-linux.tar.bz2 && \
    tar xjf gcc-arm-none-eabi-10-2020-q4-major-$ARCH-linux.tar.bz2 && \
    rm -rf /opt/gcc/*.tar.bz2

ENV PATH="/opt/gcc/gcc-arm-none-eabi-10-2020-q4-major/bin:${PATH}"

# GET Particle Build sys & Device OS version 5.5.0
RUN git clone --depth=1 --branch master https://github.com/edgeimpulse/particle-build-tools.git $HOME/.particle \
&& cd $HOME/.particle && git reset --hard a221068abf681c607a607316b456917654ce16af \
&& cd $HOME/.particle/toolchains \
&& git clone https://github.com/particle-iot/device-os.git \
&& cd $HOME/.particle/toolchains/device-os \
&& git reset --hard 8dcc34057748c90a8cf2e6eba157fd06c16ce1b7 \
&& git clean -df \
&& git submodule update --init \
&& sed -i 's/16/20/g' modules/shared/rtl872x/build_linker_script.mk
# In the last step we up the reserved bytes for alignment to omit the 4 byte SRAM or PSRAM error
