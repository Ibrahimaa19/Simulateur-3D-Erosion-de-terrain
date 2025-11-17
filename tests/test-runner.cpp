#include <gtest/gtest.h>

/**
 * @brief Fonction principale du framework GoogleTest.
 */
int main(int argc, char **argv)
{
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
