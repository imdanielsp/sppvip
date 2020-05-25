cmake_minimum_required (VERSION 3.13)

project(portaudio-download NONE)

include(ExternalProject)
ExternalProject_Add(portaudio_project
    URL http://www.portaudio.com/archives/pa_stable_v190600_20161030.tgz
    DOWNLOAD_DIR portaudio
    SOURCE_DIR ${CMAKE_BINARY_DIR}/portaudio-src
    BINARY_DIR ${CMAKE_BINARY_DIR}/portaudio-build
    LOG_DOWNLOAD ON)
