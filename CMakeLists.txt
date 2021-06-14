cmake_minimum_required(VERSION 3.17)
project(FocusIPC)

set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_CXX_STANDARD 20)

set(PRE_CONFIGURE_FILE "version.h.in")
set(POST_CONFIGURE_FILE "version.h")
include(./cmake-git-version-tracking/git_watcher.cmake)

find_package(Qt5Widgets REQUIRED)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

include_directories(${CMAKE_BINARY_DIR})
include_directories(FTools/VbfEditor)
include_directories(FTools/ImgSectionParser)
include_directories(FTools/miniz)
include_directories(FTools/EifViewer)
include_directories(FTools/CRCpp/inc)

add_subdirectory(FTools)

add_executable(FocusIPC main.cpp mainwindow.cpp mainwindow.ui HeaderObjectsModel.cpp)
add_dependencies(FocusIPC check_git)

target_link_libraries(${CMAKE_PROJECT_NAME} Qt5::Widgets Qt5::Core)
target_link_libraries(${CMAKE_PROJECT_NAME} vbf imgsec eif miniz)
if ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")
    add_compile_options(-Werror=vla)
#    target_link_libraries(${CMAKE_PROJECT_NAME} pthread)
endif()
if(WIN32)
    set_property(TARGET FocusIPC PROPERTY WIN32_EXECUTABLE true)
endif()

install(TARGETS FocusIPC)