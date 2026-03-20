# Copyright (C) 2026 Keith Kyzivat <https://github.com/keithel>
# SPDX-License-Identifier: GPL-3.0-only

cmake_minimum_required(VERSION 3.21)
project(qt_resource_dumper LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_AUTOMOC ON)

find_package(Qt6 REQUIRED COMPONENTS Core)

add_library(qt_resource_dumper SHARED
    qt_resource_dumper.cpp
)

target_link_libraries(qt_resource_dumper PRIVATE Qt6::Core)
