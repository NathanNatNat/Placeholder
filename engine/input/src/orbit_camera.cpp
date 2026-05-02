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
    // Shift+MMB: pan
    glm::vec2 pan = input.getAxis("Pan");
    if (pan.x != 0.0f || pan.y != 0.0f)
    {
        float yawRad = glm::radians(m_yaw);
        float pitchRad = glm::radians(m_pitch);

        glm::vec3 forward;
        forward.x = glm::cos(pitchRad) * glm::cos(yawRad);
        forward.y = glm::sin(pitchRad);
        forward.z = glm::cos(pitchRad) * glm::sin(yawRad);

        glm::vec3 right = glm::normalize(glm::cross(forward, glm::vec3(0.0f, 1.0f, 0.0f)));
        glm::vec3 up = glm::normalize(glm::cross(right, forward));

        float panScale = m_distance * m_panSensitivity;
        m_target -= right * pan.x * panScale;
        m_target += up * pan.y * panScale;
    }

    // MMB: orbit
    glm::vec2 look = input.getAxis("Look");
    if (look.x != 0.0f || look.y != 0.0f)
    {
        m_yaw += look.x * m_orbitSensitivity;
        m_pitch += look.y * m_orbitSensitivity;
        m_pitch = std::clamp(m_pitch, -89.0f, 89.0f);
    }

    // Scroll: zoom
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
