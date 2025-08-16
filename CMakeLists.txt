# CMakeLists.txt for Wind Tunnel Project
# Dependencies include ImGui, SDL3, OpenGL, and wjwwood's Serial library
cmake_minimum_required(VERSION 3.10)
project(WindTunnel CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

cmake_path(SET IMGUI_DIR "${CMAKE_CURRENT_SOURCE_DIR}/vendor/imgui")
cmake_path(SET SERIAL_DIR "${CMAKE_CURRENT_SOURCE_DIR}/vendor/serial")
cmake_path(SET SDL_DIR "${CMAKE_CURRENT_SOURCE_DIR}/vendor/SDL")

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/$<CONFIGURATION>")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/$<CONFIGURATION>")

add_subdirectory(vendor/SDL EXCLUDE_FROM_ALL)

add_executable(main
  main.cpp

  ${IMGUI_DIR}/imgui.cpp
  ${IMGUI_DIR}/imgui_demo.cpp
  ${IMGUI_DIR}/imgui_draw.cpp
  ${IMGUI_DIR}/imgui_tables.cpp
  ${IMGUI_DIR}/imgui_widgets.cpp
  ${IMGUI_DIR}/backends/imgui_impl_sdl3.cpp
  ${IMGUI_DIR}/backends/imgui_impl_opengl3.cpp
)

target_include_directories(main PRIVATE
  ${IMGUI_DIR}
  ${IMGUI_DIR}/backends
  ${SERIAL_DIR}/include
)

if(APPLE)
  find_library(COCOA_LIB Cocoa REQUIRED)
  find_library(OPENGL_LIB OpenGL REQUIRED)

  target_link_libraries(main PRIVATE ${COCOA_LIB} ${OPENGL_LIB})
endif()

target_link_libraries(main PRIVATE 
  SDL3::SDL3
)

target_compile_options(main PRIVATE
  -g  
  -Wall
  -Wextra
  -Wformat
)
