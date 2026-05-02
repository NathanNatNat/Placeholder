#pragma once

#include <string>

namespace placeholder::input
{

/// How an action responds to input.
enum class ActionType
{
    Trigger, ///< Fires once on the frame the key is pressed.
    Hold,    ///< True every frame the key is held down.
    Axis     ///< 2D delta from mouse movement (requires a modifier button).
};

/// Maps a physical input to a named action.
struct ActionBinding
{
    std::string action;
    ActionType type = ActionType::Trigger;
    int key = -1;          ///< GLFW key code, or -1 if unbound.
    int mouseButton = -1;  ///< GLFW mouse button, or -1 if unbound.
};

} // namespace placeholder::input
