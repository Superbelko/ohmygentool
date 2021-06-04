FROM gitpod/workspace-full

ARG D_VERSION=ldc-1.26.0
ARG DPATH=/dlang

# cmake, lld, clang, clangd, etc already installed
# See: https://github.com/gitpod-io/workspace-images/blob/master/full/Dockerfile
RUN apt-get install -y libclang-dev llvm-dev \
  && update-alternatives --install "/usr/bin/ld" "ld" "/usr/bin/lld"

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
