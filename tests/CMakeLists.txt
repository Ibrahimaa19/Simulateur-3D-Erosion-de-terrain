# rassembler les fichiers de test
file(GLOB TEST_SOURCES
    test-runner.cpp
    test-shader.cpp
)

add_executable(erosion_tests ${TEST_SOURCES} ../src/Shader.cpp)

target_include_directories(erosion_tests PRIVATE ../src)

target_link_libraries(erosion_tests
    PRIVATE
        gtest
        gtest_main
        OpenGL::GL
        GLEW::GLEW
        glfw
        glm::glm
)

add_test(NAME erosion_tests COMMAND erosion_tests)
