#pragma once
#include <gtest/gtest.h>
#include "Camera.hpp"

/**
 * @class CameraTest
 * @brief Test fixture for the Camera class.
 *
 * Initializes a Camera instance for each test.
 */
class CameraTest : public ::testing::Test
{
protected:
    Camera camera;

    void SetUp() override
    {
        // The camera is already initialized by its default constructor
    }

    void TearDown() override
    {
        // Nothing particular to clean up
    }
};
