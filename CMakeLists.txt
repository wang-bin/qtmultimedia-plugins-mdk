# cmake -B build -DCMAKE_PREFIX_PATH=$HOME/Qt/6.11.0/macos
# cmake --build build && cmake --install build
cmake_minimum_required(VERSION 3.16)
project(qtmultimedia-plugins-mdk VERSION 0.1 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(QT_NO_PRIVATE_MODULE_WARNING ON)

find_package(Qt6 REQUIRED COMPONENTS Core Gui GuiPrivate Multimedia OpenGL MultimediaPrivate)

# MDK_SDK defaults to ./mdk-sdk in the project root
if(NOT MDK_SDK)
    set(MDK_SDK "${CMAKE_CURRENT_SOURCE_DIR}/mdk-sdk")
endif()
if(NOT EXISTS "${MDK_SDK}/lib/cmake/FindMDK.cmake")
    message(FATAL_ERROR "MDK_SDK not found at ${MDK_SDK} (expected lib/cmake/FindMDK.cmake). Extract mdk-sdk into the project root or set -DMDK_SDK=...")
endif()
include("${MDK_SDK}/lib/cmake/FindMDK.cmake")
if(NOT MDK_FOUND AND NOT TARGET mdk)
    message(FATAL_ERROR "FindMDK.cmake did not create target mdk (MDK_SDK=${MDK_SDK})")
endif()

set(MDK_PLUGIN_SOURCES
    qt6/mdkplatformmediaplugin.cpp
    qt6/mdkplatformmediaplugin.h
    qt6/mdkmediaintegration.cpp
    qt6/mdkmediaintegration.h
    qt6/mdkplayercontrol.cpp
    qt6/mdkplayercontrol.h
    qt6/mdkrihiframe.cpp
    qt6/mdkrihiframe.h
    qt6/mdkvideosink.cpp
    qt6/mdkvideosink.h
    qt6/qtcompat.h
    qt6/mdk.json
)

qt_add_plugin(mdkmediaplugin
    CLASS_NAME MDKMediaPlugin
    PLUGIN_TYPE multimedia
    OUTPUT_NAME mdkmediaplugin
    ${MDK_PLUGIN_SOURCES}
)

target_include_directories(mdkmediaplugin PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/qt6
)

target_link_libraries(mdkmediaplugin PRIVATE
    Qt6::Core
    Qt6::Gui
    Qt6::GuiPrivate
    Qt6::Multimedia
    Qt6::MultimediaPrivate
    Qt6::OpenGL
    mdk
)

get_filename_component(_qt_prefix "${Qt6_DIR}/../../.." ABSOLUTE)
set(_qt_plugins_dir "${_qt_prefix}/plugins/multimedia")
set(_qt_lib_dir "${_qt_prefix}/lib")
if(WIN32)
    set(_qt_bin_dir "${_qt_prefix}/bin")
endif()

# Match other Qt multimedia plugins: resolve @rpath deps from Qt's lib/.
# MDK runtime must be installed there too (see install rules below).
set_target_properties(mdkmediaplugin PROPERTIES
    BUILD_RPATH "${MDK_RUNTIME_DIR};${_qt_lib_dir}"
    INSTALL_RPATH "@loader_path/../../lib"
)

install(TARGETS mdkmediaplugin
    LIBRARY DESTINATION "${_qt_plugins_dir}"
    RUNTIME DESTINATION "${_qt_plugins_dir}"
    ARCHIVE DESTINATION "${_qt_plugins_dir}"
)

# Runtime: plugin links @rpath/mdk — install FindMDK's framework or runtime files next to Qt libs/bins.
if(MDK_FRAMEWORK)
    install(DIRECTORY "${MDK_FRAMEWORK}"
        DESTINATION "${_qt_lib_dir}"
        USE_SOURCE_PERMISSIONS
    )
elseif(MDK_RUNTIMES)
    if(WIN32)
        install(FILES ${MDK_RUNTIMES} DESTINATION "${_qt_bin_dir}")
    else()
        install(FILES ${MDK_RUNTIMES} DESTINATION "${_qt_lib_dir}")
    endif()
endif()

message(STATUS "Qt6: ${Qt6_DIR}")
message(STATUS "MDK_SDK=${MDK_SDK}")
message(STATUS "MDK_FRAMEWORK=${MDK_FRAMEWORK}")
message(STATUS "MDK_RUNTIMES=${MDK_RUNTIMES}")
message(STATUS "Plugin install dir: ${_qt_plugins_dir}")
message(STATUS "MDK runtime install dir: ${_qt_lib_dir}")
