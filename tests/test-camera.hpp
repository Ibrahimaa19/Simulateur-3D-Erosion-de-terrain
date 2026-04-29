#pragma once
#include "Camera.hpp"
#include <gtest/gtest.h>

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
