# CMakeLists.txt for Wind Tunnel Project
# Dependencies include ImGui, SDL3, OpenGL, and wjwwood's Serial library
cmake_minimum_required(VERSION 3.10)
project(WindTunnel CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

cmake_path(SET IMGUI_DIR "${CMAKE_CURRENT_SOURCE_DIR}/vendor/imgui")
cmake_path(SET IMPLOT_DIR "${CMAKE_CURRENT_SOURCE_DIR}/vendor/implot")
cmake_path(SET SERIAL_DIR "${CMAKE_CURRENT_SOURCE_DIR}/vendor/serial")
cmake_path(SET SDL_DIR "${CMAKE_CURRENT_SOURCE_DIR}/vendor/SDL")

find_package(OpenGL REQUIRED)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/$<CONFIGURATION>")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/$<CONFIGURATION>")

add_subdirectory(vendor/SDL EXCLUDE_FROM_ALL)

## Sources for Serial
set(serial_SRCS
    ${SERIAL_DIR}/src/serial.cc
)
if(APPLE)
    # If OSX
    list(APPEND serial_SRCS ${SERIAL_DIR}/src/impl/unix.cc)
    list(APPEND serial_SRCS ${SERIAL_DIR}/src/impl/list_ports/list_ports_osx.cc)
elseif(UNIX)
    # If unix
    list(APPEND serial_SRCS ${SERIAL_DIR}/src/impl/unix.cc)
    list(APPEND serial_SRCS ${SERIAL_DIR}/src/impl/list_ports/list_ports_linux.cc)
else()
    # If windows
    list(APPEND serial_SRCS ${SERIAL_DIR}/src/impl/win.cc)
    list(APPEND serial_SRCS ${SERIAL_DIR}/src/impl/list_ports/list_ports_win.cc)
endif()

## Add serial library
add_library(serial_lib STATIC ${serial_SRCS}) # Create a new target for the serial library
target_include_directories(serial_lib PUBLIC ${SERIAL_DIR}/include)
target_link_libraries(serial_lib PUBLIC ${SERIAL_LIBS})

add_executable(main
  main.cpp

  ${IMGUI_DIR}/imgui.cpp
  ${IMGUI_DIR}/imgui_demo.cpp
  ${IMGUI_DIR}/imgui_draw.cpp
  ${IMGUI_DIR}/imgui_tables.cpp
  ${IMGUI_DIR}/imgui_widgets.cpp
  ${IMGUI_DIR}/backends/imgui_impl_sdl3.cpp
  ${IMGUI_DIR}/backends/imgui_impl_opengl3.cpp

  ${IMPLOT_DIR}/implot.cpp
  ${IMPLOT_DIR}/implot_demo.cpp
  ${IMPLOT_DIR}/implot_items.cpp
  ${IMPLOT_DIR}/implot_internal.h
  ${IMPLOT_DIR}/implot.h
)

target_include_directories(main PRIVATE
  ${IMGUI_DIR}
  ${IMGUI_DIR}/backends
  ${IMPLOT_DIR}
)

if(APPLE)
  find_library(COCOA_LIB Cocoa REQUIRED)
  find_library(OPENGL_LIB OpenGL REQUIRED)

  target_link_libraries(main PRIVATE ${COCOA_LIB} ${OPENGL_LIB})
endif()

target_link_libraries(main PRIVATE 
  SDL3::SDL3
  serial_lib
)

if(APPLE)
    # Ensure IOKit and Foundation are linked to the executable
    find_library(IOKIT_LIB IOKit REQUIRED)
    find_library(FOUNDATION_LIB Foundation REQUIRED)
    target_link_libraries(main PRIVATE ${IOKIT_LIB} ${FOUNDATION_LIB})
endif()

target_compile_options(main PRIVATE
  -g  
  -Wall
  -Wextra
  -Wformat
)
