#include <gtest/gtest.h>

/**
 * @brief Main function for the GoogleTest framework.
 */
int main(int argc, char **argv)
{
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
