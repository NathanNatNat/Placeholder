#include "input/fly_camera.h"
#include "input/input_manager.h"

#include <glm/ext/matrix_transform.hpp>
#include <glm/trigonometric.hpp>

#include <algorithm>

namespace placeholder::input
{

FlyCamera::FlyCamera()
{
    updateVectors();
}

void FlyCamera::update(const InputManager& input, float deltaTime)
{
    glm::vec2 look = input.getAxis("Look");
    if (look.x != 0.0f || look.y != 0.0f)
    {
        m_yaw += look.x * m_lookSensitivity;
        m_pitch -= look.y * m_lookSensitivity;
        m_pitch = std::clamp(m_pitch, -89.0f, 89.0f);
        updateVectors();
    }

    float velocity = m_moveSpeed * deltaTime;

    if (input.isHeld("MoveForward"))  m_position += m_front * velocity;
    if (input.isHeld("MoveBack"))     m_position -= m_front * velocity;
    if (input.isHeld("MoveLeft"))     m_position -= m_right * velocity;
    if (input.isHeld("MoveRight"))    m_position += m_right * velocity;
    if (input.isHeld("MoveUp"))       m_position.y += velocity;
    if (input.isHeld("MoveDown"))     m_position.y -= velocity;

    glm::vec2 scroll = input.getScrollDelta();
    if (scroll.y != 0.0f)
    {
        m_moveSpeed = std::clamp(m_moveSpeed + scroll.y * 0.5f, 0.5f, 100.0f);
    }
}

glm::mat4 FlyCamera::getViewMatrix() const
{
    return glm::lookAt(m_position, m_position + m_front, m_up);
}

void FlyCamera::updateVectors()
{
    float yawRad = glm::radians(m_yaw);
    float pitchRad = glm::radians(m_pitch);

    m_front.x = glm::cos(yawRad) * glm::cos(pitchRad);
    m_front.y = glm::sin(pitchRad);
    m_front.z = glm::sin(yawRad) * glm::cos(pitchRad);
    m_front = glm::normalize(m_front);

    m_right = glm::normalize(glm::cross(m_front, glm::vec3(0.0f, 1.0f, 0.0f)));
    m_up = glm::normalize(glm::cross(m_right, m_front));
}

} // namespace placeholder::input
