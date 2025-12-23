cmake_minimum_required(VERSION 3.5)

project(AfpViewerEx VERSION 0.1.0 LANGUAGES CXX)

find_package(PNG REQUIRED)
find_package(spdlog CONFIG REQUIRED)
find_package(safetyhook CONFIG REQUIRED)

add_library(${PROJECT_NAME} SHARED)

target_sources(${PROJECT_NAME}
  PRIVATE
    src/memory.cc
    src/modules.cc
    src/afp_scene.cc
    src/entrypoint.cc
    src/frame_capture.cc
    src/frame_exporter.cc
    src/capture_controller.cc
)

target_compile_features(${PROJECT_NAME}
  PRIVATE
    cxx_std_23
)

set_target_properties(${PROJECT_NAME}
  PROPERTIES
    PREFIX ""
)

target_link_libraries(${PROJECT_NAME}
  PRIVATE
    d3dx9
    PNG::PNG
    spdlog::spdlog
    safetyhook::safetyhook
)