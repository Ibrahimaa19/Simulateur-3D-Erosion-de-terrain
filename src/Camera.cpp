#include <GL/freeglut_std.h>
#include <GL/freeglut_ext.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "Camera.hpp"

Camera::Camera()
{
    mPosition = glm::vec3(0.0f, 0.0f, 0.0f);
    mYaw = 0.0f;
    mPitch = 0.0f;
}

Camera::~Camera()
{
}

glm::vec3 Camera::GetPosition()
{
    return mPosition;
}

glm::vec3 Camera::GetForward()
{
    glm::vec4 forward = glm::inverse(GetRotationMatrix()) * glm::vec4(0.f, 0.f, -1.f, 1.f);
    return glm::normalize(glm::vec3(forward));
}

glm::vec3 Camera::GetRight()
{
    glm::vec4 right = glm::inverse(GetRotationMatrix()) * glm::vec4(1.f, 0.f, 0.f, 1.f);
    return glm::normalize(glm::vec3(right));
}

glm::vec3 Camera::GetUp()
{
    glm::vec4 up = glm::inverse(GetRotationMatrix()) * glm::vec4(0.f, 1.f, 0.f, 1.f);
    return glm::normalize(glm::vec3(up));
}

glm::mat4 Camera::GetViewMatrix()
{
    return GetRotationMatrix() * glm::translate(glm::mat4(1.f), -mPosition);
}

glm::mat4 Camera::GetRotationMatrix()
{
    glm::mat4 rotation(1.0f);
    rotation = glm::rotate(rotation, glm::radians(mPitch), glm::vec3(1.f, 0.f, 0.f));
    rotation = glm::rotate(rotation, glm::radians(mYaw), glm::vec3(0.f, 1.f, 0.f));
    return rotation;
}

void Camera::Move(glm::vec3 direction, float amount)
{
    mPosition = mPosition + direction * amount;
}

void Camera::Yaw(float angle)
{
    mYaw += angle;
    NormalizeYaw();
}

void Camera::Pitch(float angle)
{   
    mPitch += angle;
    NormalizePitch();
}

void Camera::TurnTo(glm::vec3 position)
{
    if(position == mPosition) return;

    glm::vec3 direction = glm::normalize(position - mPosition);
    mPitch = glm::degrees(asinf(-direction.y));
    NormalizePitch();

    mYaw = -glm::degrees(atan2f(-direction.x, -direction.z));
    NormalizeYaw();
}

void Camera::MoveTo(glm::vec3 position)
{
    mPosition = position;
}

void Camera::NormalizeYaw()
{
    mYaw = fmodf(mYaw, 360.f);
    if(mYaw < 0.f)
    {
        mYaw += 360.f;
    }
}

void Camera::NormalizePitch()
{
    if(mPitch > 87.f)
    {
        mPitch = 87.f;
    }
    if(mPitch < -87.f)
    {
        mPitch = -87.f;
    }
}