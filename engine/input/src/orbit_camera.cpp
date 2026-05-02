#include "input/orbit_camera.h"
#include "input/input_manager.h"

#include <glm/ext/matrix_transform.hpp>
#include <glm/trigonometric.hpp>

#include <algorithm>

namespace placeholder::input
{

OrbitCamera::OrbitCamera() = default;

void OrbitCamera::update(const InputManager& input, float /*deltaTime*/)
{
    glm::vec2 look = input.getAxis("Look");
    if (look.x != 0.0f || look.y != 0.0f)
    {
        m_yaw += look.x * m_orbitSensitivity;
        m_pitch += look.y * m_orbitSensitivity;
        m_pitch = std::clamp(m_pitch, -89.0f, 89.0f);
    }

    glm::vec2 scroll = input.getScrollDelta();
    if (scroll.y != 0.0f)
    {
        float zoomFactor = 1.0f - scroll.y * m_zoomSpeed * 0.1f;
        m_distance *= zoomFactor;
        m_distance = std::clamp(m_distance, m_minDistance, m_maxDistance);
    }
}

glm::mat4 OrbitCamera::getViewMatrix() const
{
    return glm::lookAt(getPosition(), m_target, glm::vec3(0.0f, 1.0f, 0.0f));
}

glm::vec3 OrbitCamera::getPosition() const
{
    float yawRad = glm::radians(m_yaw);
    float pitchRad = glm::radians(m_pitch);

    glm::vec3 offset;
    offset.x = m_distance * glm::cos(pitchRad) * glm::cos(yawRad);
    offset.y = m_distance * glm::sin(pitchRad);
    offset.z = m_distance * glm::cos(pitchRad) * glm::sin(yawRad);

    return m_target + offset;
}

} // namespace placeholder::input
