# Build with the official NTS-3 toolchain, but avoid the weak fallback header
# injected by sdk/common/_unit_base.c, which duplicates .unit_header.
PROJECT_ROOT := $(realpath $(dir $(lastword $(MAKEFILE_LIST))))
COMMON_SRC_PATH := $(PROJECT_ROOT)/sdk_common
include ../dummy-genericfx/Makefile
