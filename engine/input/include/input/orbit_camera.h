#pragma once

#include "camera.h"

#include <glm/vec3.hpp>

namespace placeholder::input
{

/// Third-person orbit camera that rotates around a target point.
///
/// Right-click drag to rotate, scroll wheel to zoom in/out.
class OrbitCamera : public Camera
{
public:
    OrbitCamera();

    void update(const InputManager& input, float deltaTime) override;
    glm::mat4 getViewMatrix() const override;
    glm::vec3 getPosition() const override;

    void setTarget(const glm::vec3& target) { m_target = target; }
    glm::vec3 getTarget() const { return m_target; }

    void setDistance(float distance) { m_distance = distance; }
    void setOrbitSensitivity(float sensitivity) { m_orbitSensitivity = sensitivity; }

private:
    glm::vec3 m_target{0.0f, 0.0f, 0.0f};
    float m_distance = 10.0f;
    float m_yaw = 0.0f;
    float m_pitch = 30.0f;

    float m_minDistance = 1.0f;
    float m_maxDistance = 200.0f;
    float m_orbitSensitivity = 0.2f;
    float m_zoomSpeed = 1.0f;
};

} // namespace placeholder::input
