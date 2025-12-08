cmake_minimum_required(VERSION 3.10)
project(erosion)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Find required packages
find_package(OpenGL REQUIRED)
find_package(GLEW REQUIRED)
find_package(glfw3 REQUIRED)
find_package(glm REQUIRED)

# --------------------------------------------------------------------
# ### AJOUT IMGUI 
# --------------------------------------------------------------------
# On définit le chemin vers le dossier que tu as créé
set(IMGUI_DIR ${CMAKE_SOURCE_DIR}/external/imgui)

# On liste manuellement les fichiers sources d'ImGui nécessaires
# Note : On inclut "impl_glfw" car le projet utilise GLFW
# Note : On inclut "impl_opengl3" car le projet utilise OpenGL moderne
set(IMGUI_SOURCES
    ${IMGUI_DIR}/imgui.cpp
    ${IMGUI_DIR}/imgui_demo.cpp
    ${IMGUI_DIR}/imgui_draw.cpp
    ${IMGUI_DIR}/imgui_tables.cpp
    ${IMGUI_DIR}/imgui_widgets.cpp
    ${IMGUI_DIR}/backends/imgui_impl_glfw.cpp
    ${IMGUI_DIR}/backends/imgui_impl_opengl3.cpp
)

include_directories(${PROJECT_SOURCE_DIR}/include)

set(SOURCE_FILES
    ${PROJECT_SOURCE_DIR}/src/main.cpp # Commenter et decommenter pour échanger de main
    #${PROJECT_SOURCE_DIR}/src/mainGui.cpp
    ${PROJECT_SOURCE_DIR}/src/Camera.cpp
    ${PROJECT_SOURCE_DIR}/src/Shader.cpp
    ${PROJECT_SOURCE_DIR}/src/Terrain.cpp
    ${PROJECT_SOURCE_DIR}/src/ThermalErosion.cpp
    ${PROJECT_SOURCE_DIR}/src/Gui.cpp
    ${PROJECT_SOURCE_DIR}/src/TerrainApp.cpp
)
# Create the executable
# ### MODIFICATION : On ajoute ${IMGUI_SOURCES} à la liste
add_executable(${PROJECT_NAME} ${SOURCE_FILES} ${IMGUI_SOURCES})

# Link the libraries
target_link_libraries(${PROJECT_NAME}
    PRIVATE
        OpenGL::GL
        GLEW::GLEW
        glfw
        
)

# Include header directories
target_include_directories(${PROJECT_NAME}
    PRIVATE
        ${PROJECT_SOURCE_DIR}/src
        ${PROJECT_SOURCE_DIR}/include
        ${GLEW_INCLUDE_DIRS}
        ${GLFW_INCLUDE_DIRS}
        ${GLM_INCLUDE_DIRS}
        ${IMGUI_DIR}
        ${IMGUI_DIR}/backends
)

# Define GLEW_STATIC if needed
add_definitions(-DGLEW_STATIC)




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