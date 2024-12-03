cmake_minimum_required(VERSION 3.5.0)

project(COMMPORT_READER VERSION 0.1.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDART 17)
set(CMAKE_CXX_STANDART_REQUIRED True)

find_package(GLEW REQUIRED)
find_package(glfw3 CONFIG REQUIRED)
find_package(imgui CONFIG REQUIRED)

add_executable(COMMPORT_READER 
    main.cpp
    src/intercept_usb.cpp)

target_link_libraries(COMMPORT_READER 
    PRIVATE 
        imgui::imgui
        glfw
        GLEW::GLEW)

target_include_directories(COMMPORT_READER PRIVATE include)
