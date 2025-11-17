cmake_minimum_required(VERSION 3.10)
project(erosion)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Find required packages
find_package(OpenGL REQUIRED)
find_package(GLEW REQUIRED)
find_package(glfw3 REQUIRED)
find_package(glm REQUIRED)

# Collect all source files from src/
file(GLOB SOURCE_FILES src/*.cpp)

# Create the executable
add_executable(${PROJECT_NAME} ${SOURCE_FILES})

# Include header directories
target_include_directories(${PROJECT_NAME}
    PRIVATE
        ${PROJECT_SOURCE_DIR}/src
        ${GLEW_INCLUDE_DIRS}
        ${GLFW_INCLUDE_DIRS}
        ${GLM_INCLUDE_DIRS}
)

# Define GLEW_STATIC if needed
add_definitions(-DGLEW_STATIC)

# Link the libraries
target_link_libraries(${PROJECT_NAME}
    PRIVATE
        OpenGL::GL
        GLEW::GLEW
        glfw
)

# --------------------------------------------------------------------
# GOOGLE TEST
# --------------------------------------------------------------------
include(FetchContent)

FetchContent_Declare(
    googletest
    URL https://github.com/google/googletest/archive/refs/tags/v1.14.0.zip
)

FetchContent_MakeAvailable(googletest)
enable_testing()
add_subdirectory(tests)
