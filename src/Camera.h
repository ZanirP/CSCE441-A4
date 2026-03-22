#pragma once

#include <memory>
#include <glm/glm.hpp>

class MatrixStack;

class Camera
{
public:
    Camera();
    virtual ~Camera();

    void mouseClicked(float x, float y, bool shift, bool ctrl, bool alt);
    void mouseMoved(float x, float y);

    void moveForward(float amount);
    void moveRight(float amount);
    void zoom(float amount);

    void setAspect(float a);
    void applyProjectionMatrix(std::shared_ptr<MatrixStack> P) const;
    void applyViewMatrix(std::shared_ptr<MatrixStack> MV) const;

private:
    float aspect;
    float fovy;
    float znear;
    float zfar;

    glm::vec3 position;
    float yaw;
    float pitch;

    glm::vec2 mousePrev;
    float mouseSensitivity;
    float moveSpeed;
};