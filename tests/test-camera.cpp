#include <gtest/gtest.h>
#include "Camera.hpp"
#include <glm/glm.hpp>
#include <glm/gtc/epsilon.hpp>

class CameraTest : public ::testing::Test {
protected:
    Camera camera;
};

TEST_F(CameraTest, DefaultPositionIsOrigin) {
    glm::vec3 pos = camera.GetPosition();
    EXPECT_TRUE(glm::all(glm::epsilonEqual(pos, glm::vec3(0.0f), 1e-6f)));
}

TEST_F(CameraTest, DirectionVectorsAreNormalized) {
    EXPECT_NEAR(glm::length(camera.GetForward()), 1.0f, 1e-6f);
    EXPECT_NEAR(glm::length(camera.GetRight()), 1.0f, 1e-6f);
    EXPECT_NEAR(glm::length(camera.GetUp()), 1.0f, 1e-6f);
}

TEST_F(CameraTest, MoveUpdatesPosition) {
    camera.Move(glm::vec3(1.0f, 0.0f, 0.0f), 5.0f);
    glm::vec3 pos = camera.GetPosition();
    EXPECT_TRUE(glm::all(glm::epsilonEqual(pos, glm::vec3(5.0f, 0.0f, 0.0f), 1e-6f)));
}

TEST_F(CameraTest, MoveToUpdatesPosition) {
    camera.MoveTo(glm::vec3(1.0f, 2.0f, 3.0f));
    glm::vec3 pos = camera.GetPosition();
    EXPECT_TRUE(glm::all(glm::epsilonEqual(pos, glm::vec3(1.0f, 2.0f, 3.0f), 1e-6f)));
}

TEST_F(CameraTest, TurnToFacesTarget) {
    camera.MoveTo(glm::vec3(0.0f));
    glm::vec3 target(0.0f, 0.0f, -1.0f);
    camera.TurnTo(target);

    glm::vec3 forward = camera.GetForward();
    EXPECT_NEAR(forward.x, 0.0f, 1e-6f);
    EXPECT_NEAR(forward.y, 0.0f, 1e-6f);
    EXPECT_NEAR(forward.z, -1.0f, 1e-6f);
}

TEST_F(CameraTest, TurnToDoesNothingWhenAtTarget) {
    camera.MoveTo(glm::vec3(1.0f, 1.0f, 1.0f));
    glm::vec3 posBefore = camera.GetPosition();
    camera.TurnTo(posBefore);
    glm::vec3 posAfter = camera.GetPosition();
    EXPECT_TRUE(glm::all(glm::epsilonEqual(posBefore, posAfter, 1e-6f)));
}
