cmake_minimum_required(VERSION 3.19)
project(gdextension-gdtune VERSION 0.0.1)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)
set(CMAKE_VERBOSE_MAKEFILE 1)             # ビルド時に詳細な情報を表示する makeのコマンドにVERBOSE=1を付けるのと同じ

add_compile_options(/std:c++23)
add_compile_options(/Zc:preprocessor)       # マクロでの VA_OPT の使用には '/Zc:preprocessor' が必要です (VisualStudio warning C5109)
add_definitions(-D_HAS_CXX23=1)

#set(CMAKE_CXX_EXTENSIONS OFF)
# Enable on Release build ----------------------------------
#set(CMAKE_BUILD_TYPE Release)
#set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} /MD")
 # _ITERATOR_DEBUG_LEVELを統一
#add_compile_definitions(
#    $<$<CONFIG:Debug>:_ITERATOR_DEBUG_LEVEL=2>
#    $<$<CONFIG:Release>:_ITERATOR_DEBUG_LEVEL=0>
#)
#set(MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>DLL")

#----------------------------------------------------------
# ASIO対応のための定義　コンパイラにはちゃんと設定が通っているのになぜか効果がない
#set(JUCE_ASIO TRUE)
#add_definitions(-DJUCE_ASIO=1)
#add_compile_definitions

#---------------------------------------------------------
# Godot-cppのソースコードをダウンロードして使用可能にする
include(FetchContent)
FetchContent_Declare(
    GDExtension
    GIT_REPOSITORY https://github.com/godotengine/godot-cpp.git
    GIT_TAG godot-4.3-stable
)
FetchContent_MakeAvailable(GDExtension)

#---------------------------------------------------------
# JUCEのソースコードをダウンロードして使用可能にする
#FetchContent_Declare(
#    JUCE
#    GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
#    GIT_TAG 8.0.2
#)
#FetchContent_MakeAvailable(JUCE)

# FetchContentを使わないで自分でJUCEをダウンロードする場合は上を削除して以下を有効にする
#add_subdirectory(
#    "C:/MyData/Works40s/Programing/juce-8.0.2-windows/JUCE"
#    "${CMAKE_BINARY_DIR}/JUCE_build"
#)
#---------------------------------------------------------


add_subdirectory(gdtune)