# Collect all test source files
file(GLOB TEST_SOURCES
    test-runner.cpp
    test-shader.cpp
    test-camera.cpp
)

# Create the test executable including the source files from ../src
add_executable(erosion_tests
    ${TEST_SOURCES}
    ../src/Shader.cpp
    ../src/Camera.cpp
)

# Include the source directory for header files
target_include_directories(erosion_tests PRIVATE ../src)

# Link the necessary libraries
target_link_libraries(erosion_tests
    PRIVATE
        gtest
        gtest_main
        OpenGL::GL
        GLEW::GLEW
        glfw
        glm::glm
)

# Register the test with CTest
add_test(NAME erosion_tests COMMAND erosion_tests)
