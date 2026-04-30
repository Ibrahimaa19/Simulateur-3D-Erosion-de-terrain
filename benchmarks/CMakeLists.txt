add_executable(benchmark_render
    benchmark_render.cpp
    ${PROJECT_SOURCE_DIR}/src/Terrain.cpp
    ${PROJECT_SOURCE_DIR}/src/PerlinNoiseTerrain.cpp
    ${PROJECT_SOURCE_DIR}/src/Shader.cpp
)

target_include_directories(benchmark_render
    PRIVATE
        ${PROJECT_SOURCE_DIR}/include
        ${PROJECT_SOURCE_DIR}/src
        ${GLEW_INCLUDE_DIRS}
        ${GLFW_INCLUDE_DIRS}
        ${GLM_INCLUDE_DIRS}
)

target_link_libraries(benchmark_render
    PRIVATE
        OpenGL::GL
        GLEW::GLEW
        glfw
        glm::glm
)

target_compile_definitions(benchmark_render
    PRIVATE
        BENCHMARK_PROJECT_ROOT=\"${PROJECT_SOURCE_DIR}\"
)

set_target_properties(benchmark_render PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/benchmarks
)
