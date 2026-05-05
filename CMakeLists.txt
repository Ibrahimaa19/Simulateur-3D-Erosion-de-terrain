cmake_minimum_required(VERSION 3.16)
project(erosion LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

option(EROSION_ENABLE_RENDERING "Build the OpenGL/GLFW/ImGui renderer" ON)
option(EROSION_ENABLE_MPI "Build MPI execution mode" ON)
option(EROSION_ENABLE_TESTS "Build dependency-free core tests" ON)
option(EROSION_NATIVE_ARCH "Enable -march=native for local benchmarking" OFF)
set(EROSION_MODE "5" CACHE STRING "MPI erosion kernel mode selected in include/Mpi.hpp")

find_package(OpenMP REQUIRED)

add_library(erosion_core STATIC
    src/Terrain.cpp
    src/ThermalErosion.cpp
    src/FaultFormationTerrain.cpp
    src/MidpointDisplacement.cpp
    src/PerlinNoiseTerrain.cpp
    src/ValidationTest.cpp
    src/stb_impl.cpp
)

target_include_directories(erosion_core
    PUBLIC
        ${PROJECT_SOURCE_DIR}/include
)

target_link_libraries(erosion_core
    PUBLIC
        OpenMP::OpenMP_CXX
        stdc++fs
)

target_compile_options(erosion_core
    PRIVATE
        $<$<CONFIG:Release>:-O3>
        $<$<CONFIG:RelWithDebInfo>:-O3>
)

if(EROSION_NATIVE_ARCH)
    target_compile_options(erosion_core PRIVATE -march=native)
endif()

add_executable(${PROJECT_NAME} src/main.cpp)

target_link_libraries(${PROJECT_NAME}
    PRIVATE
        erosion_core
)

target_compile_definitions(${PROJECT_NAME}
    PRIVATE
        EROSION_ENABLE_RENDERING=$<BOOL:${EROSION_ENABLE_RENDERING}>
        EROSION_ENABLE_MPI=$<BOOL:${EROSION_ENABLE_MPI}>
        EROSION_MODE=${EROSION_MODE}
)

if(EROSION_ENABLE_MPI)
    find_program(EROSION_MPI_CXX_WRAPPER NAMES mpicxx mpic++ mpiCC)
    if(EROSION_MPI_CXX_WRAPPER)
        set(MPI_CXX_COMPILER "${EROSION_MPI_CXX_WRAPPER}" CACHE FILEPATH "MPI C++ compiler")

        execute_process(
            COMMAND "${EROSION_MPI_CXX_WRAPPER}" --showme:compile
            OUTPUT_VARIABLE EROSION_MPI_COMPILE_FLAGS
            ERROR_QUIET
            OUTPUT_STRIP_TRAILING_WHITESPACE
        )
        string(REGEX MATCHALL "-I[^ \n\t]+" EROSION_MPI_INCLUDE_FLAGS "${EROSION_MPI_COMPILE_FLAGS}")
        set(EROSION_MPI_INCLUDE_DIRS "")
        foreach(EROSION_MPI_INCLUDE_FLAG IN LISTS EROSION_MPI_INCLUDE_FLAGS)
            string(REGEX REPLACE "^-I" "" EROSION_MPI_INCLUDE_DIR "${EROSION_MPI_INCLUDE_FLAG}")
            if(IS_DIRECTORY "${EROSION_MPI_INCLUDE_DIR}")
                list(APPEND EROSION_MPI_INCLUDE_DIRS "${EROSION_MPI_INCLUDE_DIR}")
            endif()
        endforeach()
        if(EROSION_MPI_INCLUDE_DIRS)
            list(REMOVE_DUPLICATES EROSION_MPI_INCLUDE_DIRS)
            set(MPI_CXX_COMPILER_INCLUDE_DIRS "${EROSION_MPI_INCLUDE_DIRS}" CACHE STRING "MPI C++ include dirs" FORCE)
        endif()
    endif()

    find_package(MPI REQUIRED COMPONENTS CXX)

    target_link_libraries(${PROJECT_NAME}
        PRIVATE
            MPI::MPI_CXX
    )
endif()

if(EROSION_ENABLE_RENDERING)
    find_package(OpenGL REQUIRED)
    find_package(GLEW REQUIRED)
    find_package(glfw3 REQUIRED)
    find_path(GLM_INCLUDE_DIR glm/glm.hpp REQUIRED)

    include(FetchContent)

    message(STATUS "Configuration de ImGui pour le rendu interactif")
    FetchContent_Declare(
        imgui
        GIT_REPOSITORY https://github.com/ocornut/imgui.git
        GIT_TAG        v1.90.4
    )
    FetchContent_MakeAvailable(imgui)

    add_library(imgui_lib STATIC
        ${imgui_SOURCE_DIR}/imgui.cpp
        ${imgui_SOURCE_DIR}/imgui_draw.cpp
        ${imgui_SOURCE_DIR}/imgui_tables.cpp
        ${imgui_SOURCE_DIR}/imgui_widgets.cpp
        ${imgui_SOURCE_DIR}/imgui_demo.cpp
        ${imgui_SOURCE_DIR}/backends/imgui_impl_glfw.cpp
        ${imgui_SOURCE_DIR}/backends/imgui_impl_opengl3.cpp
    )

    target_include_directories(imgui_lib
        PUBLIC
            ${imgui_SOURCE_DIR}
            ${imgui_SOURCE_DIR}/backends
    )

    target_link_libraries(imgui_lib
        PUBLIC
            glfw
            GLEW::GLEW
            OpenGL::GL
    )

    target_sources(${PROJECT_NAME}
        PRIVATE
            src/Camera.cpp
            src/Shader.cpp
            src/Gui.cpp
            src/TerrainApp.cpp
            src/Frustrum.cpp
            src/Patch.cpp
            src/RendererManager.cpp
            src/Texture.cpp
    )

    target_link_libraries(${PROJECT_NAME}
        PRIVATE
            imgui_lib
            OpenGL::GL
            GLEW::GLEW
            glfw
    )

    target_include_directories(${PROJECT_NAME} PRIVATE ${GLM_INCLUDE_DIR})

    target_compile_definitions(${PROJECT_NAME} PRIVATE GLEW_STATIC)
endif()

if(EROSION_ENABLE_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()

find_program(CLANG_FORMAT_EXE NAMES clang-format)
if(CLANG_FORMAT_EXE)
    file(GLOB_RECURSE EROSION_FORMAT_FILES CONFIGURE_DEPENDS
        ${PROJECT_SOURCE_DIR}/include/*.hpp
        ${PROJECT_SOURCE_DIR}/src/*.cpp
        ${PROJECT_SOURCE_DIR}/tests/*.cpp
        ${PROJECT_SOURCE_DIR}/tests/*.hpp
    )
    list(FILTER EROSION_FORMAT_FILES EXCLUDE REGEX ".*/include/stb_image\\.hpp$")

    add_custom_target(format
        COMMAND ${CLANG_FORMAT_EXE} -i ${EROSION_FORMAT_FILES}
        COMMENT "Formatting C++ sources with clang-format"
    )

    add_custom_target(format-check
        COMMAND ${CLANG_FORMAT_EXE} --dry-run --Werror ${EROSION_FORMAT_FILES}
        COMMENT "Checking C++ formatting with clang-format"
    )
endif()
