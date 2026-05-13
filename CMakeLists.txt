cmake_minimum_required(VERSION 3.15)
project(AudioAnalyzer CXX)

set(CMAKE_CXX_STANDARD 17)

add_compile_definitions(_USE_MATH_DEFINES NOMINMAX)

find_package(ffmpeg REQUIRED)
find_package(fftw3 REQUIRED)
find_package(Crow REQUIRED)
find_package(nlohmann_json REQUIRED)

add_executable(analyzer
    main.cpp
    src/core/audio_decoder.cpp
    src/core/fft_engine.cpp
    src/features/mfcc_extractor.cpp
)

target_include_directories(analyzer PRIVATE ${CMAKE_SOURCE_DIR})

target_link_libraries(analyzer PRIVATE
    ffmpeg::ffmpeg
    FFTW3::fftw3
    Crow::Crow
    nlohmann_json::nlohmann_json
)

if(MSVC)
    add_compile_options("/utf-8")
endif()

add_executable(test_features
    tests/test_features.cpp
    src/core/fft_engine.cpp
    src/features/mfcc_extractor.cpp
)

target_include_directories(test_features PRIVATE ${CMAKE_SOURCE_DIR})
target_link_libraries(test_features PRIVATE
    FFTW3::fftw3
)
