#define _USE_MATH_DEFINES
#include <cmath>
#include <algorithm>
#include <glm/gtc/matrix_transform.hpp>
#include "Camera.h"
#include "MatrixStack.h"

Camera::Camera() :
    aspect(1.0f),
    fovy((float)(45.0 * M_PI / 180.0)),
    znear(0.1f),
    zfar(1000.0f),
    position(0.0f, 1.0f, 5.0f),
    yaw((float)(-M_PI / 2.0f)),
    pitch(0.0f),
    mouseSensitivity(0.005f),
    moveSpeed(0.2f)
{
}

Camera::~Camera()
{
}

void Camera::mouseClicked(float x, float y, bool shift, bool ctrl, bool alt)
{
    mousePrev = glm::vec2(x, y);
}

void Camera::mouseMoved(float x, float y)
{
    glm::vec2 mouseCurr(x, y);
    glm::vec2 dv = mouseCurr - mousePrev;

    yaw   += mouseSensitivity * dv.x;
    pitch -= mouseSensitivity * dv.y;

    float maxPitch = (float)(60.0 * M_PI / 180.0);
    pitch = std::max(-maxPitch, std::min(maxPitch, pitch));

    mousePrev = mouseCurr;
}

void Camera::moveForward(float amount)
{
    glm::vec3 forward(
        cos(yaw),
        0.0f,
        sin(yaw)
    );
    position += amount * glm::normalize(forward);
}

void Camera::moveRight(float amount)
{
    glm::vec3 forward(
        cos(yaw),
        0.0f,
        sin(yaw)
    );
    forward = glm::normalize(forward);

    glm::vec3 up(0.0f, 1.0f, 0.0f);
    glm::vec3 right = glm::normalize(glm::cross(forward, up));

    position += amount * right;
}

void Camera::zoom(float amount)
{
    fovy += amount;

    float minFovy = (float)(5.0  * M_PI / 180.0);
    float maxFovy = (float)(80.0 * M_PI / 180.0);
    fovy = std::max(minFovy, std::min(maxFovy, fovy));
}

void Camera::setAspect(float a)
{
    aspect = a;
}

void Camera::applyProjectionMatrix(std::shared_ptr<MatrixStack> P) const
{
    P->multMatrix(glm::perspective(fovy, aspect, znear, zfar));
}

void Camera::applyViewMatrix(std::shared_ptr<MatrixStack> MV) const
{
    glm::vec3 forward;
    forward.x = cos(pitch) * cos(yaw);
    forward.y = sin(pitch);
    forward.z = cos(pitch) * sin(yaw);
    forward = glm::normalize(forward);

    glm::vec3 up(0.0f, 1.0f, 0.0f);
    glm::vec3 target = position + forward;

    MV->multMatrix(glm::lookAt(position, target, up));
}