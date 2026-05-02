#include "input/input_manager.h"
#include "core/config.h"
#include "platform/window.h"
#include "core/logging.h"

#include <GLFW/glfw3.h>

namespace placeholder::input
{

// --- Key name <-> GLFW code mapping ---

static const std::unordered_map<std::string, int> KEY_NAME_TO_GLFW = {
    {"W", GLFW_KEY_W}, {"A", GLFW_KEY_A}, {"S", GLFW_KEY_S}, {"D", GLFW_KEY_D},
    {"Q", GLFW_KEY_Q}, {"E", GLFW_KEY_E}, {"R", GLFW_KEY_R}, {"F", GLFW_KEY_F},
    {"Space", GLFW_KEY_SPACE}, {"LeftShift", GLFW_KEY_LEFT_SHIFT},
    {"LeftControl", GLFW_KEY_LEFT_CONTROL}, {"LeftAlt", GLFW_KEY_LEFT_ALT},
    {"Tab", GLFW_KEY_TAB}, {"Escape", GLFW_KEY_ESCAPE},
    {"F1", GLFW_KEY_F1}, {"F2", GLFW_KEY_F2}, {"F3", GLFW_KEY_F3},
    {"F4", GLFW_KEY_F4}, {"F5", GLFW_KEY_F5}, {"F6", GLFW_KEY_F6},
    {"F7", GLFW_KEY_F7}, {"F8", GLFW_KEY_F8}, {"F9", GLFW_KEY_F9},
    {"F10", GLFW_KEY_F10}, {"F11", GLFW_KEY_F11}, {"F12", GLFW_KEY_F12},
    {"1", GLFW_KEY_1}, {"2", GLFW_KEY_2}, {"3", GLFW_KEY_3},
};

static const std::unordered_map<std::string, int> MOUSE_NAME_TO_GLFW = {
    {"Left", GLFW_MOUSE_BUTTON_LEFT},
    {"Right", GLFW_MOUSE_BUTTON_RIGHT},
    {"Middle", GLFW_MOUSE_BUTTON_MIDDLE},
};

static ActionType parseActionType(const std::string& str)
{
    if (str == "hold") return ActionType::Hold;
    if (str == "axis") return ActionType::Axis;
    return ActionType::Trigger;
}

// --- InputManager ---

void InputManager::initialize(const core::Config& config, platform::Window& window)
{
    auto logger = core::getLogger("input");
    logger->info("Initializing input manager");

    window.setScrollCallback([this](double xOffset, double yOffset)
    {
        m_scrollAccum.x += static_cast<float>(xOffset);
        m_scrollAccum.y += static_cast<float>(yOffset);
    });

    window.getCursorPos(m_lastCursorX, m_lastCursorY);
    m_firstMouse = true;

    loadBindings(config);

    logger->info("Loaded {} input bindings", m_actions.size());
}

void InputManager::update(platform::Window& window)
{
    double cursorX = 0.0;
    double cursorY = 0.0;
    window.getCursorPos(cursorX, cursorY);

    if (m_firstMouse)
    {
        m_lastCursorX = cursorX;
        m_lastCursorY = cursorY;
        m_firstMouse = false;
    }

    m_mouseDelta.x = static_cast<float>(cursorX - m_lastCursorX);
    m_mouseDelta.y = static_cast<float>(cursorY - m_lastCursorY);
    m_lastCursorX = cursorX;
    m_lastCursorY = cursorY;

    m_scrollDelta = m_scrollAccum;
    m_scrollAccum = glm::vec2{0.0f};

    for (auto& [name, state] : m_actions)
    {
        bool down = false;

        if (state.binding.key >= 0)
        {
            down = window.isKeyDown(state.binding.key);
        }

        if (!down && state.binding.mouseButton >= 0)
        {
            down = window.isMouseButtonDown(state.binding.mouseButton);
        }

        state.pressedThisFrame = down && !state.wasHeld;
        state.held = down;
        state.wasHeld = down;
    }
}

bool InputManager::isTriggered(const std::string& action) const
{
    auto it = m_actions.find(action);
    if (it == m_actions.end()) return false;
    return it->second.pressedThisFrame;
}

bool InputManager::isHeld(const std::string& action) const
{
    auto it = m_actions.find(action);
    if (it == m_actions.end()) return false;
    return it->second.held;
}

glm::vec2 InputManager::getAxis(const std::string& action) const
{
    auto it = m_actions.find(action);
    if (it == m_actions.end()) return glm::vec2{0.0f};

    const auto& state = it->second;
    if (state.binding.type != ActionType::Axis) return glm::vec2{0.0f};

    bool modifierHeld = false;
    if (state.binding.mouseButton >= 0)
    {
        modifierHeld = state.held;
    }
    else
    {
        modifierHeld = true;
    }

    return modifierHeld ? m_mouseDelta : glm::vec2{0.0f};
}

void InputManager::addBinding(const ActionBinding& binding)
{
    ActionState state;
    state.binding = binding;
    m_actions[binding.action] = state;
}

void InputManager::loadBindings(const core::Config& config)
{
    const auto& raw = config.raw();
    if (!raw.contains("input") || !raw["input"].contains("bindings"))
    {
        return;
    }

    for (const auto& entry : raw["input"]["bindings"])
    {
        ActionBinding binding;
        binding.action = entry.value("action", "");
        if (binding.action.empty()) continue;

        binding.type = parseActionType(entry.value("type", "trigger"));

        if (entry.contains("key"))
        {
            std::string keyName = entry["key"].get<std::string>();
            auto it = KEY_NAME_TO_GLFW.find(keyName);
            if (it != KEY_NAME_TO_GLFW.end())
            {
                binding.key = it->second;
            }
            else
            {
                core::getLogger("input")->warn("Unknown key name: {}", keyName);
            }
        }

        if (entry.contains("mouseButton"))
        {
            std::string btnName = entry["mouseButton"].get<std::string>();
            auto it = MOUSE_NAME_TO_GLFW.find(btnName);
            if (it != MOUSE_NAME_TO_GLFW.end())
            {
                binding.mouseButton = it->second;
            }
            else
            {
                core::getLogger("input")->warn("Unknown mouse button: {}", btnName);
            }
        }

        addBinding(binding);
    }
}

} // namespace placeholder::input
