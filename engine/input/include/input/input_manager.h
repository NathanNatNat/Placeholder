#pragma once

#include "input_types.h"

#include <glm/vec2.hpp>

#include <string>
#include <unordered_map>
#include <vector>

namespace placeholder::core { class Config; }
namespace placeholder::platform { class Window; }

namespace placeholder::input
{

/// Action-mapping input system.
///
/// Game code queries named actions instead of raw keys. Bindings are loaded
/// from JSON config and can be changed at runtime.
class InputManager
{
public:
    /// Load bindings from config and install GLFW callbacks on the window.
    void initialize(const core::Config& config, platform::Window& window);

    /// Poll input state. Call once per frame before game logic.
    void update(platform::Window& window);

    /// @return true on the frame the action's key was first pressed.
    bool isTriggered(const std::string& action) const;

    /// @return true every frame the action's key is held.
    bool isHeld(const std::string& action) const;

    /// @return mouse delta for an axis-type action (pixels this frame).
    glm::vec2 getAxis(const std::string& action) const;

    /// @return accumulated scroll wheel delta since last update.
    glm::vec2 getScrollDelta() const { return m_scrollDelta; }

    /// Register an action with a default binding. Config overrides this.
    void addBinding(const ActionBinding& binding);

private:
    void loadBindings(const core::Config& config);

    struct ActionState
    {
        ActionBinding binding;
        bool pressedThisFrame = false;
        bool held = false;
        bool wasHeld = false;
    };

    std::unordered_map<std::string, ActionState> m_actions;

    glm::vec2 m_mouseDelta{0.0f};
    glm::vec2 m_scrollDelta{0.0f};
    glm::vec2 m_scrollAccum{0.0f};
    double m_lastCursorX = 0.0;
    double m_lastCursorY = 0.0;
    bool m_firstMouse = true;
};

} // namespace placeholder::input
