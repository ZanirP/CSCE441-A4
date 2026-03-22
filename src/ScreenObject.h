#pragma once
#include "Shape.h"

#include <vector>
#include <memory>
#include <glm/glm.hpp>

struct SceneObject {
    std::shared_ptr<Shape> mesh;
    glm::vec3 pos;
    glm::vec3 rot;
    glm::vec3 scl;
    glm::mat4 shear;
    glm::vec3 color;
    float minY;
};