#!/bin/sh
DOCKER_BUILDKIT=1 docker build --build-arg VERSION=5.6.1 -t quisp .