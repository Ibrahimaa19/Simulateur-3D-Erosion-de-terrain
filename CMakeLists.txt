cmake_minimum_required (VERSION 3.0)
project (Erosion)

find_package(OpenGL REQUIRED)

add_subdirectory (lib)

include_directories(
	lib/glfw-3.1.2/include/
	lib/glm-0.9.7.1/
	lib/glew-1.13.0/include/
)

add_executable(erosion 
    src/main.cpp 
    shaders/SimpleFragmentShader.fragmentshader
    shaders/SimpleVertexShader.vertexshader)

target_include_directories(erosion PUBLIC ${PROJECT_SOURCE_DIR}/include)
target_link_libraries(erosion ${OPENGL_LIBRARY} glfw GLEW_1130)
