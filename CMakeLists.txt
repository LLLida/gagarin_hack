cmake_minimum_required(VERSION 3.10)
project(harakiri VERSION 0.1)

add_executable(harakiri "main.cpp"
  "imgui.cpp"
  "imgui_demo.cpp"
  "imgui_draw.cpp"
  "imgui_impl_sdl2.cpp"
  "imgui_impl_sdlrenderer2.cpp"
  "imgui_tables.cpp"
  "imgui_widgets.cpp"
  "implot.cpp"
  "implot_demo.cpp"
  "implot_items.cpp")

target_include_directories(${PROJECT_NAME} PUBLIC
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>)

set_target_properties(${PROJECT_NAME} PROPERTIES
  C_STANDARD 11
  C_STANDARD_REQUIRED ON
  C_EXTENSIONS OFF
  CXX_STANDARD 11
  CXX_STANDARD_REQUIRED ON
  CXX_EXTENSIONS OFF
)

find_package(SDL2 REQUIRED)

find_package(PkgConfig REQUIRED)
pkg_check_modules(LIBAV REQUIRED IMPORTED_TARGET
    libavdevice
    libavfilter
    libavformat
    libavcodec
    libavutil
)

target_link_libraries(${PROJECT_NAME} PRIVATE
  PkgConfig::LIBAV
  SDL2::SDL2
  m
)
