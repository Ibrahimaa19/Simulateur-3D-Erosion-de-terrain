cmake_minimum_required (VERSION 3.0)
project (Erosion)

find_package(OpenGL REQUIRED)

add_subdirectory (external)

set(ALL_LIBS
	${OPENGL_LIBRARY}
	glfw
	GLEW_1130
)

include_directories(
	external/AntTweakBar-1.16/include/
	external/glfw-3.1.2/include/
	external/glm-0.9.7.1/
	external/glew-1.13.0/include/
	external/assimp-3.0.1270/include/
	external/bullet-2.81-rev2613/src/
	.
)

add_executable(erosion 
    src/main.cpp 
    shaders/SimpleFragmentShader.fragmentshader
    shaders/SimpleVertexShader.vertexshader)

target_include_directories(erosion PUBLIC ${PROJECT_SOURCE_DIR}/include)
target_link_libraries(erosion ${OPENGL_LIBRARY} glfw GLEW_1130)
