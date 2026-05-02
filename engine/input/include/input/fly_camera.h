#pragma once

#include "camera.h"

#include <glm/vec3.hpp>
#include <glm/gtc/quaternion.hpp>

namespace placeholder::input
{

/// Free fly camera with WASD movement and mouse look.
///
/// Mouse look activates when right mouse button is held.
/// Uses quaternion orientation to avoid gimbal lock.
class FlyCamera : public Camera
{
public:
    FlyCamera();

    void update(const InputManager& input, float deltaTime) override;
    glm::mat4 getViewMatrix() const override;
    glm::vec3 getPosition() const override { return m_position; }

    void setPosition(const glm::vec3& pos) { m_position = pos; }
    void setMoveSpeed(float speed) { m_moveSpeed = speed; }
    void setLookSensitivity(float sensitivity) { m_lookSensitivity = sensitivity; }

private:
    glm::vec3 m_position{0.0f, 1.0f, 5.0f};
    float m_yaw = -90.0f;
    float m_pitch = 0.0f;
    glm::vec3 m_front{0.0f, 0.0f, -1.0f};
    glm::vec3 m_right{1.0f, 0.0f, 0.0f};
    glm::vec3 m_up{0.0f, 1.0f, 0.0f};

    float m_moveSpeed = 5.0f;
    float m_lookSensitivity = 0.1f;

    void updateVectors();
};

} // namespace placeholder::input
