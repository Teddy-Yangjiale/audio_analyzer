cmake_minimum_required(VERSION 3.15)
project(AudioAnalyzer CXX)

set(CMAKE_CXX_STANDARD 17)

find_package(ffmpeg REQUIRED)
find_package(fftw3 REQUIRED)
find_package(Crow REQUIRED)
find_package(nlohmann_json REQUIRED)


add_executable(analyzer main.cpp)

target_link_libraries(analyzer PRIVATE 
    ffmpeg::ffmpeg 
    FFTW3::fftw3 
    Crow::Crow 
    nlohmann_json::nlohmann_json
)

# 解决 MSVC 环境下中文注释导致的 C4819 警告
if(MSVC)
    add_compile_options("/utf-8")
endif()

