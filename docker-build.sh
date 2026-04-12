#!/bin/bash
# Build and test inside Docker (agodio/itba-so-multiarch:3.1)
# Usage:
#   ./docker-build.sh          # clean build + tests
#   ./docker-build.sh build    # only build (master + players)
#   ./docker-build.sh test     # only tests
#   ./docker-build.sh all      # build + tests (default)

set -e

DOCKER_IMAGE="agodio/itba-so-multiarch:3.1"
PROJECT_DIR="$(cd "$(dirname "$0")" && pwd)"

case "${1:-all}" in
    build)
        CMD="make clean && make build/master players"
        ;;
    test)
        CMD="make clean && make test"
        ;;
    all|"")
        CMD="make clean && make build/master players && make test"
        ;;
    *)
        CMD="$*"
        ;;
esac

docker run --rm \
    -v "${PROJECT_DIR}:/root/ccso" \
    -w /root/ccso \
    "${DOCKER_IMAGE}" \
    bash -c "${CMD}"
