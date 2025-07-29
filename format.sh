#!/bin/bash

find . -regex '.*\.\(c\|cpp\|h\|hpp\)' -exec clang-format -i {} +