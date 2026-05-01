#!/bin/sh
# SPDX-License-Identifier: LGPL-2.1-or-later
podman build -t rcc .
podman run --rm -it rcc bash --login -i
