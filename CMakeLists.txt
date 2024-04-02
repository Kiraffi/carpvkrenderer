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


add_subdirectory("external/carpvk")
add_subdirectory("external/glfw")

set(GLFW_BUILD_DOCS FALSE)
set(GLFW_INSTALL FALSE)


target_link_libraries(carpvkrenderer PRIVATE
    carpvk
    glfw
)