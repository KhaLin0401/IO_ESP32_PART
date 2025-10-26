cmake_minimum_required(VERSION 3.5)

# Thêm main vào danh sách components
set(EXTRA_COMPONENT_DIRS main)

include($ENV{IDF_PATH}/tools/cmake/project.cmake)
project(Module_IO)