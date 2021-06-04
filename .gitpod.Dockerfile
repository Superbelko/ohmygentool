# Test locally with:
# $ docker build -f .gitpod.Dockerfile -t gitpod-dockerfile-test .
FROM gitpod/workspace-full

ARG D_VERSION=ldc-1.26.0
ARG DPATH=/dlang

# cmake, lld, clang, clangd, etc already installed
# See: https://github.com/gitpod-io/workspace-images/blob/master/full/Dockerfile
RUN sudo curl -o /var/lib/apt/dazzle-marks/llvm.gpg -fsSL https://apt.llvm.org/llvm-snapshot.gpg.key \
    && apt-key add /var/lib/apt/dazzle-marks/llvm.gpg \
    && echo "deb https://apt.llvm.org/focal/ llvm-toolchain-focal main" >> /etc/apt/sources.list.d/llvm.list \
    && apt-get update \
    && apt-get install -y libclang-dev llvm-dev lldb \
    && update-alternatives --install "/usr/bin/ld" "ld" "/usr/bin/lld" 50

RUN set -ex \
  && mkdir ${DPATH} \
  && curl -fsS https://dlang.org/install.sh | bash -s ${D_VERSION} -p ${DPATH} \
  && chmod 755 -R ${DPATH} \
  && ln -s ${DPATH}/${D_VERSION} ${DPATH}/dc \
  && ls ${DPATH}

ENV PATH="${DPATH}/${D_VERSION}/bin:${PATH}"
ENV LIBRARY_PATH="${DPATH}/${D_VERSION}/lib:${LIBRARY_PATH}"
ENV LD_LIBRARY_PATH="${DPATH}/${D_VERSION}/lib:${LD_LIBRARY_PATH}"

RUN git clone https://github.com/Tencent/rapidjson.git deps/rapidjson \
  && cp -r deps/rapidjson/include/rapidjson include
