#!/bin/sh
BUILD_DIR="build-rpm"
yum install -y centos-release-scl
yum-config-manager --enable rhel-server-rhscl-7-rpms
yum repolist
yum install -y git qt5-qtbase-devel cmake3 eigen3-devel devtoolset-7 freeglut-devel rpm-build
Run scl enable devtoolset-7 bash
mkdir -p "${BUILD_DIR}"
cd "${BUILD_DIR}" || exit 1
cmake3 .. -DCMAKE_BUILD_TYPE=Release
make -j 4
cpack3 -G RPM
