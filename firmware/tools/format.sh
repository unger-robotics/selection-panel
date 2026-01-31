#!/usr/bin/env bash
set -euo pipefail

clang-format -i \
  include/*.h \
  src/**/*.cpp
