FROM ubuntu:21.04 as build


ARG DEBIAN_FRONTEND=noninteractive
RUN apt-get update && apt-get install -y --no-install-recommends \
  cmake \
  git \
  tar zip unzip xz-utils \
  ninja-build make \
  clang-11 lld-11 \
  libclang-11-dev llvm-11-dev \
  curl wget \
  ca-certificates \
  && update-alternatives --install "/usr/bin/ld" "ld" "/usr/bin/lld-11" 50 \
  && ln -s /usr/lib/llvm-11/lib/libclang-cpp.so.11 /usr/lib/llvm-11/lib/libclang-cpp.so 

ARG DPATH=/dlang
ARG D_VERSION=ldc-1.26.0
RUN mkdir ${DPATH} \
  && curl -fsS https://dlang.org/install.sh | bash -s ${D_VERSION},dub -p ${DPATH}
   

WORKDIR gentool
COPY . .
RUN rm -rf build && mkdir build && cd build \
    && . /dlang/${D_VERSION}/activate \
    && ls -s \
    && export CC=clang-11 \
    && export CXX=clang++-11 \
    && cmake .. -DGENTOOL_LIB=ON \
    && cmake --build . --config Release \
    && cd .. \
    && ./dubBuild.sh

# prod image
FROM ubuntu:21.04 as prod
RUN apt-get update && apt-get install -y --no-install-recommends clang-11 llvm-11
WORKDIR /root/
COPY --from=build /gentool/build/gentool .
ENTRYPOINT ["./gentool"]
