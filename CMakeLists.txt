cmake_minimum_required(VERSION 3.10)

project(erosion)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Trouver les packages nécessaires
find_package(OpenGL REQUIRED)
find_package(GLEW REQUIRED)
find_package(glfw3 REQUIRED)
find_package(glm REQUIRED)

# Rassembler toutes les sources de src/
file(GLOB SOURCE_FILES src/*.cpp)

# Créer l'exécutable
add_executable(${PROJECT_NAME} ${SOURCE_FILES})

target_include_directories(${PROJECT_NAME}
    PRIVATE
        ${PROJECT_SOURCE_DIR}/src
)

target_link_libraries(${PROJECT_NAME}
    PRIVATE
        OpenGL::GL
        GLEW::GLEW
        glfw
        glm::glm
)
