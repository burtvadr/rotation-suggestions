cmake_minimum_required(VERSION 3.20)
project(rotation_suggestions VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)

set(KDE_INSTALL_USE_QT_SYS_PATHS ON CACHE BOOL "" FORCE)

find_package(ECM 6.0 REQUIRED NO_MODULE)
set(CMAKE_MODULE_PATH ${ECM_MODULE_PATH})
include(KDEInstallDirs)

find_package(Qt6 6.5 REQUIRED COMPONENTS Core Qml DBus)

add_library(rotationplugin MODULE
    src/rotationplugin.cpp
    src/rotationplugin.h
    src/rotationutil.cpp
    src/rotationutil.h
)
target_link_libraries(rotationplugin PRIVATE
    Qt6::Core
    Qt6::Qml
    Qt6::DBus
)

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
