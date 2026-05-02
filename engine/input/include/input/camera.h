#pragma once

#include <glm/mat4x4.hpp>
#include <glm/vec3.hpp>
#include <glm/ext/matrix_clip_space.hpp>

namespace placeholder::input
{

class InputManager;

/// Base class for cameras that produce view and projection matrices.
class Camera
{
public:
    virtual ~Camera() = default;

    /// Update camera state from input. Called once per frame.
    virtual void update(const InputManager& input, float deltaTime) = 0;

    /// @return The camera's view matrix (world-to-eye).
    virtual glm::mat4 getViewMatrix() const = 0;

    /// @return The camera's world-space position.
    virtual glm::vec3 getPosition() const = 0;

    /// Compute perspective projection matrix for the given aspect ratio.
    glm::mat4 getProjectionMatrix(float aspectRatio) const
    {
        return glm::perspective(glm::radians(m_fov), aspectRatio, m_nearPlane, m_farPlane);
    }

    void setFov(float degrees) { m_fov = degrees; }
    float getFov() const { return m_fov; }

    void setClipPlanes(float nearPlane, float farPlane)
    {
        m_nearPlane = nearPlane;
        m_farPlane = farPlane;
    }

protected:
    float m_fov = 60.0f;
    float m_nearPlane = 0.1f;
    float m_farPlane = 1000.0f;
};

} // namespace placeholder::input
