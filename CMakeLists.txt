cmake_minimum_required(VERSION 3.14)
project(SDR_GPS_Receiver VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)


if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()


set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -Wpedantic")
set(CMAKE_CXX_FLAGS_DEBUG "-g -O0 -DDEBUG")
set(CMAKE_CXX_FLAGS_RELEASE "-O3 -march=native -mavx2 -mfma -DNDEBUG")


find_package(Threads REQUIRED)
find_package(PkgConfig REQUIRED)


pkg_check_modules(RTLSDR REQUIRED librtlsdr)


find_package(Gnuradio)


include_directories(
    ${PROJECT_SOURCE_DIR}/include
    ${RTLSDR_INCLUDE_DIRS}
)

set(SOURCES
    src/main.cpp
    src/acquisition/sdr_receiver.cpp
    src/acquisition/signal_acquisition.cpp
    src/tracking/gps_tracker.cpp
    src/tracking/correlator.cpp
    src/decoding/nav_decoder.cpp
    src/decoding/ephemeris_parser.cpp
    src/utils/gps_constants.cpp
    src/utils/prn_generator.cpp
    src/utils/fft_processor.cpp
)

add_executable(gps_receiver ${SOURCES})


target_link_libraries(gps_receiver
    ${CMAKE_THREAD_LIBS_INIT}
    ${RTLSDR_LIBRARIES}
    m  
)


target_compile_options(gps_receiver PRIVATE
    $<$<CONFIG:Release>:-mavx2 -mfma>
)


enable_testing()
find_package(GTest)

if(GTest_FOUND)
    add_subdirectory(tests)
endif()

install(TARGETS gps_receiver
    RUNTIME DESTINATION bin
)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)