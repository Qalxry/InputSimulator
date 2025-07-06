cmake_minimum_required(VERSION 3.10)
project(MouseClickSimulator)

# Set C++ standard
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Set build type to Release if not specified
if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release CACHE STRING "Choose the type of build." FORCE)
endif()

# Add executable as a console application (not using WIN32)
add_executable(MouseClickSimulator MouseClickSimulator.cpp)

# # 关键修改：声明为WIN32应用程序（不创建控制台窗口）
# if(WIN32)
#     add_executable(MouseClickSimulator WIN32 MouseClickSimulator.cpp)
#     if(MSVC)
#         target_link_options(MouseClickSimulator PRIVATE "/ENTRY:mainCRTStartup")
#     endif()
# else()
#     add_executable(MouseClickSimulator MouseClickSimulator.cpp)
# endif()

# Link Windows libraries
target_link_libraries(MouseClickSimulator user32)

# Set output directory
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

# Set optimization flags for Release build
if(CMAKE_BUILD_TYPE STREQUAL "Release")
  if(MSVC)
    # Visual Studio compiler flags
    set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} /O2")
  else()
    # GCC/Clang compiler flags
    set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -O3")
  endif()
endif()