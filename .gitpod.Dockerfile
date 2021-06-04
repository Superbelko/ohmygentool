# Test locally with:
# $ docker build -f .gitpod.Dockerfile -t gitpod-dockerfile-test .
FROM gitpod/workspace-full

ARG D_VERSION=ldc-1.26.0
ARG DPATH=/dlang

# cmake, lld, clang, clangd, etc already installed
# See: https://github.com/gitpod-io/workspace-images/blob/master/full/Dockerfile
RUN sudo curl -o /var/lib/apt/dazzle-marks/llvm.gpg -fsSL https://apt.llvm.org/llvm-snapshot.gpg.key \
    && sudo apt-key add /var/lib/apt/dazzle-marks/llvm.gpg \
    && echo "deb https://apt.llvm.org/focal/ llvm-toolchain-focal main" >> /etc/apt/sources.list.d/llvm2.list \
    && sudo apt-get update \
    && sudo apt-get install -y libclang-dev llvm-dev lldb \
    && sudo update-alternatives --install "/usr/bin/ld" "ld" "/usr/bin/lld" 50

RUN set -ex \
  && sudo mkdir ${DPATH} \
  && sudo curl -fsS https://dlang.org/install.sh | bash -s ${D_VERSION} -p ${DPATH} \
  && sudo chmod 755 -R ${DPATH} \
  && sudo ln -s ${DPATH}/${D_VERSION} ${DPATH}/dc \
  && sudo ls ${DPATH}

ENV PATH="${DPATH}/${D_VERSION}/bin:${PATH}"
ENV LIBRARY_PATH="${DPATH}/${D_VERSION}/lib:${LIBRARY_PATH}"
ENV LD_LIBRARY_PATH="${DPATH}/${D_VERSION}/lib:${LD_LIBRARY_PATH}"

RUN sudo git clone https://github.com/Tencent/rapidjson.git deps/rapidjson \
  && sudo cp -r deps/rapidjson/include/rapidjson include
