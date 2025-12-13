cmake_minimum_required(VERSION 3.14) 
project(erosion)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Find required packages
find_package(OpenGL REQUIRED)
find_package(GLEW REQUIRED)
find_package(glfw3 REQUIRED)
find_package(glm REQUIRED)

# --------------------------------------------------------------------
# ### GESTION IMGUI VIA FETCHCONTENT
# --------------------------------------------------------------------
include(FetchContent)

message(STATUS "Téléchargement et configuration de ImGui...")

FetchContent_Declare(
    imgui
    GIT_REPOSITORY https://github.com/ocornut/imgui.git
    GIT_TAG        v1.90.4
)
FetchContent_MakeAvailable(imgui)

# Création de la librairie ImGui statique
add_library(imgui_lib STATIC
    ${imgui_SOURCE_DIR}/imgui.cpp
    ${imgui_SOURCE_DIR}/imgui_draw.cpp
    ${imgui_SOURCE_DIR}/imgui_tables.cpp
    ${imgui_SOURCE_DIR}/imgui_widgets.cpp
    ${imgui_SOURCE_DIR}/imgui_demo.cpp
    ${imgui_SOURCE_DIR}/backends/imgui_impl_glfw.cpp
    ${imgui_SOURCE_DIR}/backends/imgui_impl_opengl3.cpp
)

# Configuration des includes pour ImGui
target_include_directories(imgui_lib PUBLIC 
    ${imgui_SOURCE_DIR} 
    ${imgui_SOURCE_DIR}/backends
)

# Liens ImGui -> GLFW/OpenGL
target_link_libraries(imgui_lib PUBLIC 
    glfw 
    GLEW::GLEW 
    OpenGL::GL
)

# --------------------------------------------------------------------
# SOURCES DU PROJET 
# --------------------------------------------------------------------

# On inclut le dossier include globalement
include_directories(${PROJECT_SOURCE_DIR}/include)

# Liste des fichiers sources fournie par tes collègues
# Note : Gui.cpp est bien dedans, et mainGui.cpp est bien commenté (désactivé)
set(SOURCE_FILES
    ${PROJECT_SOURCE_DIR}/src/main.cpp 
    #${PROJECT_SOURCE_DIR}/src/mainGui.cpp
    ${PROJECT_SOURCE_DIR}/src/Camera.cpp
    ${PROJECT_SOURCE_DIR}/src/Shader.cpp
    ${PROJECT_SOURCE_DIR}/src/Terrain.cpp
    ${PROJECT_SOURCE_DIR}/src/ThermalErosion.cpp
    ${PROJECT_SOURCE_DIR}/src/Gui.cpp
    ${PROJECT_SOURCE_DIR}/src/TerrainApp.cpp
    ${PROJECT_SOURCE_DIR}/src/FaultFormationTerrain.cpp
    ${PROJECT_SOURCE_DIR}/src/MidpointDisplacement.cpp
    ${PROJECT_SOURCE_DIR}/src/PerlinNoiseTerrain.cpp
)

# Create the executable
# Note : On a retiré ${IMGUI_SOURCES} car c'est géré par la lib imgui_lib
add_executable(${PROJECT_NAME} ${SOURCE_FILES})

# Link the libraries
target_link_libraries(${PROJECT_NAME}
    PRIVATE
        imgui_lib      # <-- Notre lib ImGui automatique
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
        # Plus besoin des chemins ImGui manuels ici
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