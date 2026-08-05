# cmake -B build -DCMAKE_PREFIX_PATH=$HOME/Qt/6.11.0/macos
# cmake --build build && cmake --install build
cmake_minimum_required(VERSION 3.16)
project(qtmultimedia-plugins-mdk VERSION 0.1 LANGUAGES CXX)

option(BUILD_TESTING "Build tests" OFF)
option(MDK_USE_QT_RHI_TEXTURE_POOL
    "Use Qt 6.8.2+ QVideoFrameTexturePool for independent RHI frame slots" OFF)

# Qt 6.8.0 introduced QHwVideoBuffer. Qt 6.8.2 adds the oldTextures handoff
# used by the optional QVideoFrameTexturePool path, while 6.8.0/6.8.1 use the
# pointer-only mapTextures() compatibility branch.
set(MDK_MIN_QT_VERSION 6.8.0)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(QT_NO_PRIVATE_MODULE_WARNING ON)

find_package(Qt6 ${MDK_MIN_QT_VERSION} REQUIRED COMPONENTS
    Core Gui GuiPrivate Multimedia OpenGL MultimediaPrivate)

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

if(MDK_USE_QT_RHI_TEXTURE_POOL)
    target_compile_definitions(mdkmediaplugin PRIVATE MDK_USE_QT_RHI_TEXTURE_POOL=1)
endif()

if(BUILD_TESTING)
    enable_testing()
    find_package(Qt6 REQUIRED COMPONENTS Test)

    qt_add_executable(tst_mdkplayercontrol
        tests/tst_mdkplayercontrol.cpp
        qt6/mdkplayercontrol.cpp
        qt6/mdkplayercontrol.h
        qt6/mdkrihiframe.cpp
        qt6/mdkrihiframe.h
    )
    target_include_directories(tst_mdkplayercontrol PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/qt6
    )
    target_link_libraries(tst_mdkplayercontrol PRIVATE
        Qt6::Core
        Qt6::Gui
        Qt6::GuiPrivate
        Qt6::Multimedia
        Qt6::MultimediaPrivate
        Qt6::OpenGL
        Qt6::Test
        mdk
    )
    if(MDK_USE_QT_RHI_TEXTURE_POOL)
        target_compile_definitions(tst_mdkplayercontrol PRIVATE MDK_USE_QT_RHI_TEXTURE_POOL=1)
    endif()
    add_test(NAME tst_mdkplayercontrol COMMAND tst_mdkplayercontrol)
    if(APPLE)
        set_tests_properties(tst_mdkplayercontrol PROPERTIES
            ENVIRONMENT "QT_MEDIA_BACKEND=darwin"
        )
    endif()
endif()

get_filename_component(_qt_prefix "${Qt6_DIR}/../../.." ABSOLUTE)
set(_qt_plugins_dir "${_qt_prefix}/plugins/multimedia")
set(_qt_lib_dir "${_qt_prefix}/lib")
if(WIN32)
    set(_qt_bin_dir "${_qt_prefix}/bin")
endif()

# Match the platform loader's relative-library syntax. The MDK runtime is
# installed in Qt's lib/ (or bin/ on Windows) as done by the rules below.
set(_mdk_install_rpath "")
if(APPLE)
    set(_mdk_install_rpath "@loader_path/../../lib")
elseif(UNIX)
    set(_mdk_install_rpath "$ORIGIN/../../lib")
endif()

if(WIN32)
    set(_mdk_runtime_install_dir "${_qt_bin_dir}")
else()
    set(_mdk_runtime_install_dir "${_qt_lib_dir}")
endif()

set_target_properties(mdkmediaplugin PROPERTIES
    BUILD_RPATH "${MDK_RUNTIME_DIR};${_qt_lib_dir}"
    INSTALL_RPATH "${_mdk_install_rpath}"
)

install(TARGETS mdkmediaplugin
    LIBRARY DESTINATION "${_qt_plugins_dir}"
    RUNTIME DESTINATION "${_qt_plugins_dir}"
    ARCHIVE DESTINATION "${_qt_plugins_dir}"
)

# Runtime: install FindMDK's framework or runtime files where the platform
# loader can resolve them from the installed plugin.
if(MDK_FRAMEWORK)
    install(DIRECTORY "${MDK_FRAMEWORK}"
        DESTINATION "${_qt_lib_dir}"
        USE_SOURCE_PERMISSIONS
    )
elseif(MDK_RUNTIMES)
    install(FILES ${MDK_RUNTIMES} DESTINATION "${_mdk_runtime_install_dir}")
endif()

message(STATUS "Qt6: ${Qt6_DIR}")
message(STATUS "Qt6 minimum version: ${MDK_MIN_QT_VERSION}")
message(STATUS "MDK_SDK=${MDK_SDK}")
message(STATUS "MDK_FRAMEWORK=${MDK_FRAMEWORK}")
message(STATUS "MDK_RUNTIMES=${MDK_RUNTIMES}")
message(STATUS "Plugin install dir: ${_qt_plugins_dir}")
message(STATUS "MDK runtime install dir: ${_mdk_runtime_install_dir}")
message(STATUS "Qt RHI texture pool: ${MDK_USE_QT_RHI_TEXTURE_POOL}")
