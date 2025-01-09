# Intel CaaS (AMR)
ARG DOCKER_PROJECT=amr-registry.caas.intel.com/bxd-sw

FROM ${DOCKER_PROJECT}/bf-syslibs:latest

ENV DEBIAN_FRONTEND noninteractive

ENV BFPYTHON_DEPS autoconf-archive \
                  libffi-dev \
                  libssl-dev

# Default to using 2 make jobs, which is a good default for CI. If you're
# building locally or you know there are more cores available, you may want to
# override this.
ARG MAKEFLAGS=-j2

# Set up Intel proxies.
ENV http_proxy='http://proxy-dmz.intel.com:911'
ENV https_proxy='http://proxy-dmz.intel.com:912'
ENV ftp_proxy='http://proxy-dmz.intel.com:21'
ENV socks_proxy='proxy-dmz.intel.com:1080'
ENV no_proxy='intel.com,*.intel.com,localhost,127.0.0.1,10.0.0.0/8,192.168.0.0/16,172.16.0.0/12'
ENV ALL_PROXY='socks5://proxy-us.intel.com'

RUN apt-get update && \
    apt-get install -y $BFPYTHON_DEPS

COPY . /bfn/bf-utils/

# make and install bf-utils
WORKDIR /bfn/bf-utils/
RUN ./autogen.sh && \
    mkdir build && cd build && \
    export LDFLAGS="-Wl,-s,-rpath=/usr/local/lib" && \
    export CFLAGS="-g -O0"  && \
    ../configure --enable-shared && \
    make && make install-strip
WORKDIR /bfn/bf-utils/third-party/pip_packages
RUN pip3 install --no-index --find-links . -r bf-python.requirements.txt

# cleanup all sources and build directories
WORKDIR /bfn/
RUN rm -rf *
