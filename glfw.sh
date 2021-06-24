#!/usr/local/bin/zsh

# This script downloads and builds glfw which used to be necessary due to a bug in the GLFW build process that targetted the wrong OSX versions, 
# but is not longer required. I have left it here in case something goes wrong in the future and we need to do it this way again.
set -x
set -e
mkdir -p glfw
curl -L https://github.com/glfw/glfw/releases/download/3.3.4/glfw-3.3.4.zip > glfw/glfw-3.3.4.zip
cd glfw
unzip glfw-3.3.4.zip
cd glfw-3.3.4
mkdir -p build
cd build
export MACOSX_DEPLOYMENT_TARGET=11.0
cmake -D CMAKE_BUILD_TYPE=Release -D GLFW_NATIVE_API=1 -D CMAKE_OSX_ARCHITECTURES="x86_64" -D BUILD_SHARED_LIBS=ON -D CMAKE_C_COMPILER=clang ../
make
