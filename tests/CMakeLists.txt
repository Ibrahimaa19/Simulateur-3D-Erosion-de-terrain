add_executable(erosion_core_tests
    test-core.cpp
)

target_link_libraries(erosion_core_tests
    PRIVATE
        erosion_core
)

add_test(NAME erosion_core_tests COMMAND erosion_core_tests)
