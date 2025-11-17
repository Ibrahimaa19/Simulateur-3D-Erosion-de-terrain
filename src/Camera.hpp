#pragma once 

#include <GL/freeglut_std.h>
#include <GL/freeglut_ext.h>

#include <glm/glm.hpp>
#include <glm/ext.hpp>

/**
 * @class Camera
 * @brief Represents a 3D camera with position, orientation, and movement control.
 *
 * The camera can calculate forward, right, and up direction vectors,
 * as well as rotation and view matrices. It also provides functions
 * for moving, turning, and directly setting the camera position.
 */
class Camera
{
public:
    /**
     * @brief Default constructor for the camera.
     *
     * Initializes position at the origin and yaw/pitch angles to 0.
     */
    Camera();

    /**
     * @brief Destructor for the camera.
     */
    ~Camera();

    /**
     * @brief Gets the current position of the camera.
     * @return A 3D vector representing the position.
     */
    glm::vec3 GetPosition();

    /**
     * @brief Gets the camera's forward vector.
     *
     * The vector is calculated using the inverse rotation matrix.
     * @return A normalized 3D vector representing the forward direction.
     */
    glm::vec3 GetForward();

    /**
     * @brief Gets the camera's right vector.
     *
     * The vector is calculated using the inverse rotation matrix.
     * @return A normalized 3D vector representing the right direction.
     */
    glm::vec3 GetRight();

    /**
     * @brief Gets the camera's up vector.
     *
     * The vector is calculated using the inverse rotation matrix.
     * @return A normalized 3D vector representing the up direction.
     */
    glm::vec3 GetUp();

    /**
     * @brief Gets the camera's view matrix.
     *
     * The matrix combines rotation and translation to convert world
     * coordinates to view coordinates.
     * @return A 4x4 matrix representing the view transformation.
     */
    glm::mat4 GetViewMatrix();

    /**
     * @brief Gets the camera's rotation matrix.
     *
     * Rotation is applied first around the X axis (pitch), then around the Y axis (yaw).
     * @return A 4x4 matrix representing rotation only.
     */
    glm::mat4 GetRotationMatrix();

    /**
     * @brief Moves the camera in a given direction.
     * @param direction Direction vector.
     * @param amount Distance or magnitude of movement.
     */
    void Move(glm::vec3 direction, float amount);

    /**
     * @brief Applies a horizontal rotation (yaw) to the camera.
     * @param angle Angle in degrees to add to the current yaw.
     */
    void Yaw(float angle);

    /**
     * @brief Applies a vertical rotation (pitch) to the camera.
     * @param angle Angle in degrees to add to the current pitch.
     */
    void Pitch(float angle);

    /**
     * @brief Orients the camera to look at a specific point.
     * @param position Target position to look at.
     */
    void TurnTo(glm::vec3 position);

    /**
     * @brief Moves the camera directly to a target position.
     * @param position New position of the camera.
     */
    void MoveTo(glm::vec3 position);

private:
    /**
     * @brief Normalizes the yaw angle to stay within 0 to 360 degrees.
     */
    void NormalizeYaw();

    /**
     * @brief Clamps the pitch angle to stay between -87 and 87 degrees.
     */
    void NormalizePitch();

private:
    glm::vec3 mPosition; ///< Current position of the camera in space
    float mYaw;          ///< Yaw angle (horizontal rotation) in degrees
    float mPitch;        ///< Pitch angle (vertical rotation) in degrees
};
