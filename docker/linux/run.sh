#!/usr/bin/env bash

set -e
set -o pipefail

./docker/build.sh

docker build -t mapbox/gl-native:linux docker/linux

docker run \
    -i \
    -v `pwd`:/home/mapbox/build \
    -t mapbox/gl-native:linux \
    build/docker/linux/test.sh
