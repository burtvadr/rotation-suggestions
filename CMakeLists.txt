# SPDX-FileCopyrightText: 2026 Rotation Suggestions contributors
# SPDX-License-Identifier: GPL-2.0-or-later

cmake_minimum_required(VERSION 3.20)
project(rotation_suggestions VERSION 0.2.0 LANGUAGES CXX)

include(CTest)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)

set(KDE_INSTALL_USE_QT_SYS_PATHS ON CACHE BOOL "" FORCE)

find_package(ECM 6.0 REQUIRED NO_MODULE)
set(CMAKE_MODULE_PATH ${ECM_MODULE_PATH})
include(KDEInstallDirs)

find_package(Qt6 6.5 REQUIRED COMPONENTS Core Qml DBus)

set(ROTATION_BACKEND "kscreen" CACHE STRING "Rotation backend: kscreen or l4t")
set_property(CACHE ROTATION_BACKEND PROPERTY STRINGS kscreen l4t)
message(STATUS "Rotation backend: ${ROTATION_BACKEND}")

set(ROTATION_SOURCES
    src/rotationplugin.cpp
    src/rotationplugin.h
    src/rotationutil.cpp
    src/rotationutil.h
    src/rotationbackend.h
    src/rotationmapping.cpp
    src/rotationmapping.h
    src/sensorproxy.cpp
    src/sensorproxy.h
)

if(ROTATION_BACKEND STREQUAL "l4t")
    list(APPEND ROTATION_SOURCES
        src/backends/l4t/l4tbackend.cpp
        src/backends/l4t/l4tbackend.h
    )
    set(ROTATION_BACKEND_DEFINE ROTATION_BACKEND_L4T)
elseif(ROTATION_BACKEND STREQUAL "kscreen")
    find_package(KF6Screen 6.7 REQUIRED)
    list(APPEND ROTATION_SOURCES
        src/backends/kscreen/kscreenbackend.cpp
        src/backends/kscreen/kscreenbackend.h
    )
    set(ROTATION_BACKEND_DEFINE ROTATION_BACKEND_KSCREEN)
else()
    message(FATAL_ERROR "ROTATION_BACKEND must be 'kscreen' or 'l4t', not '${ROTATION_BACKEND}'")
endif()

add_library(rotationplugin MODULE ${ROTATION_SOURCES})
target_include_directories(rotationplugin PRIVATE src)
target_compile_definitions(rotationplugin PRIVATE ${ROTATION_BACKEND_DEFINE})
target_link_libraries(rotationplugin PRIVATE
    Qt6::Core
    Qt6::Qml
    Qt6::DBus
)

if(ROTATION_BACKEND STREQUAL "kscreen")
    target_link_libraries(rotationplugin PRIVATE KF6::Screen)
endif()

if(BUILD_TESTING)
    find_package(Qt6 6.5 REQUIRED COMPONENTS Test)
    add_executable(rotationmappingtest
        tests/rotationmappingtest.cpp
        src/rotationmapping.cpp
    )
    target_include_directories(rotationmappingtest PRIVATE src)
    target_link_libraries(rotationmappingtest PRIVATE Qt6::Test)
    add_test(NAME rotationmappingtest COMMAND rotationmappingtest)
endif()

install(TARGETS rotationplugin
    LIBRARY DESTINATION ${KDE_INSTALL_QMLDIR}/org/kde/plasma/private/rotation
)
install(FILES src/qmldir
    DESTINATION ${KDE_INSTALL_QMLDIR}/org/kde/plasma/private/rotation
)
install(DIRECTORY packages/org.kde.plasma.rotationsuggestion
    DESTINATION ${KDE_INSTALL_DATADIR}/plasma/plasmoids
)
install(DIRECTORY packages/org.kde.plasma.screenrotation
    DESTINATION ${KDE_INSTALL_DATADIR}/plasma/plasmoids
)
