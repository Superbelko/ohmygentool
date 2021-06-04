# Test locally with:
# $ docker build -f .gitpod.Dockerfile -t gitpod-dockerfile-test .
FROM gitpod/workspace-full

# Downloads LLVM. The archive is a folder that has "/bin", "/lib", "/include" etc
# So lets just unpack it into "/usr" and be done
ARG LLVM_GITHUB_RELEASE_TAG=llvmorg-12.0.1-rc1
ARG LLVM_GITHUB_RELEASE_FILENAME=clang+llvm-12.0.1-rc1-x86_64-linux-gnu-ubuntu-21.04
RUN set -ex \
  && sudo apt-get install xz-utils \
  && sudo wget -O llvm-clang.tar.xz https://github.com/llvm/llvm-project/releases/download/${LLVM_GITHUB_RELEASE_TAG}/${LLVM_GITHUB_RELEASE_FILENAME}.tar.xz \
  && sudo tar -xvf llvm-clang.tar.xz \
  && sudo cp -R ./${LLVM_GITHUB_RELEASE_FILENAME}/* /usr \
  && sudo rm ./llvm-clang.tar.xz \
  && sudo rm -rf ./${LLVM_GITHUB_RELEASE_FILENAME} \
  && sudo update-alternatives --install "/usr/bin/ld" "ld" "/usr/bin/lld" 50

# THE ABOVE DOES NOT HAVE WORKING BINARIES
# BUT: It does correctly configure some CMake nonsense in /usr/lib/cmake/clang, and a bunch of headers + libs
# We need to now do this, to get working binaries:
RUN sudo apt-get install clang lld

# Install LDC
ARG D_VERSION=ldc-1.26.0
ARG DPATH=/dlang
RUN set -ex \
  && sudo mkdir ${DPATH} \
  && sudo curl -fsS https://dlang.org/install.sh | sudo bash -s ${D_VERSION} -p ${DPATH} \
  && sudo chmod 755 -R ${DPATH} \
  && sudo ln -s ${DPATH}/${D_VERSION} ${DPATH}/dc \
  && sudo ls ${DPATH}

ENV PATH="${DPATH}/${D_VERSION}/bin:${PATH}"
ENV LIBRARY_PATH="${DPATH}/${D_VERSION}/lib:${LIBRARY_PATH}"
ENV LD_LIBRARY_PATH="${DPATH}/${D_VERSION}/lib:${LD_LIBRARY_PATH}"
