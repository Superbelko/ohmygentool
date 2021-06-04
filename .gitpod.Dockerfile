# Test locally with:
# $ docker build -f .gitpod.Dockerfile -t gitpod-dockerfile-test .
FROM gitpod/workspace-full

ARG D_VERSION=ldc-1.26.0
ARG DPATH=/dlang

# cmake, ninja, lld, clang, clangd, etc already installed
# See: https://github.com/gitpod-io/workspace-images/blob/master/full/Dockerfile
# If you run "sudo apt-get update" first, it'll install LLVM 13 tools, but these seem to break somehow
RUN sudo apt-get install -y lldb \
  && sudo update-alternatives --install "/usr/bin/ld" "ld" "/usr/bin/lld" 50

RUN set -ex \
  && sudo mkdir ${DPATH} \
  && sudo curl -fsS https://dlang.org/install.sh | sudo bash -s ${D_VERSION} -p ${DPATH} \
  && sudo chmod 755 -R ${DPATH} \
  && sudo ln -s ${DPATH}/${D_VERSION} ${DPATH}/dc \
  && sudo ls ${DPATH}

ENV PATH="${DPATH}/${D_VERSION}/bin:${PATH}"
ENV LIBRARY_PATH="${DPATH}/${D_VERSION}/lib:${LIBRARY_PATH}"
ENV LD_LIBRARY_PATH="${DPATH}/${D_VERSION}/lib:${LD_LIBRARY_PATH}"
