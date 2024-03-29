cmake_minimum_required(VERSION 3.21)
project(carpvkrenderer)

set(CMAKE_CXX_STANDARD 20)

#set(CMAKE_CXX_FLAGS_RELEASE "-O2 -fno-inline-functions -DNDEBUG")
#set(CMAKE_CXX_FLAGS_RELEASE "-O2 -fno-inline-functions -ftime-report-details")

#add_compile_options($<$<CXX_COMPILER_ID:clang>:-ftime-trace>)

# adding multiprocessor compilation, in case generating visual studio solution separately
add_compile_options($<$<CXX_COMPILER_ID:MSVC>:/MP>)
#add_compile_options($<$<CXX_COMPILER_ID:MSVC>-ftime-ftrace)
if (WIN32)
    ADD_DEFINITIONS(-DWIN32_LEAN_AND_MEAN=1)
endif (WIN32)


add_executable(carpvkrenderer src/main.cpp "src/mymemory.h" "src/mymemory.cpp")

set(SDL_STATIC ON)
set(SDL_SHARED OFF)

set(SDL_CAMERA OFF)
set(SDL_POWER OFF)
set(SDL_SENSOR OFF)
set(SDL_HAPTIC OFF)
set(SDL_RENDER OFF)
set(SDL_HIDAPI ON)


set(SDL_LEAN_AND_MEAN ON)

if (WIN32)
set(SDL_LIBC OFF)

set(SDL_TEST_LIBRARY OFF)
set(SDL_DISKAUDIO OFF)
set(SDL_DUMMYAUDIO OFF)
set(SDL_DUMMYVIDEO OFF)
set(SDL_DUMMYCAMERA OFF)

set(SDL_DIRECTX OFF)
set(SDL_OFFSCREEN OFF)
set(SDL_OPENGL OFF)
set(SDL_OPENGLES OFF)

set(SDL_ASSEMBLY OFF)
set(SDL_MMX OFF)

set(SDL_KMSDRM OFF)
set(SDL_KMSDRM_SHARED OFF)

set(SDL_LIBUDEV OFF)
set(SDL_HIDAPI_LIBUSB OFF)
set(SDL_HIDAPI_LIBUSB_SHARED OFF)
set(SDL_STATIC_VCRT OFF)
endif (WIN32)

add_subdirectory("external/carpvk")
add_subdirectory("external/SDL")



target_link_libraries(carpvkrenderer PRIVATE
    carpvk
    SDL3::SDL3
)