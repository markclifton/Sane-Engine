#include <ge/systems/InputSystem.hpp>

#include <set>
#include <vector>

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <ge/core/Application.hpp>
#include <ge/core/Global.hpp>

namespace {
	std::set<int> PRESSED_BUTTONS;
    std::map<int, int> BUTTON_MAP;

	void key_callback(GLFWwindow* window, int button, int scancode, int action, int mods)
	{
		int actualButton = button;
		if(BUTTON_MAP.count(actualButton) > 0) {
			actualButton = BUTTON_MAP[actualButton];
		}

		if (action != GLFW_RELEASE) {
			PRESSED_BUTTONS.insert(actualButton);
		} else {
			PRESSED_BUTTONS.erase(actualButton);
		}
	}
	
}

namespace GE
{
	namespace Sys
	{
        bool IsButtonPressed(ControllerState& state, int button)
        {
			return state.buttonsPressed.count(button) > 0;
        }

		InputSystem::InputSystem(GfxApplication* gfxApplication)
			: System("InputSystem")
		{
            REGISTER_SYSTEM();
			glfwSetKeyCallback(gfxApplication->_window._window._window, key_callback);
		}

		void InputSystem::Attach()
		{
		}
		
		void InputSystem::Detach() {}
		
		void InputSystem::Update(int64_t)
		{
			ControllerState state{PRESSED_BUTTONS};
			GE::GlobalDispatcher().trigger(state);
		}

        InputSystem& InputSystem::UpdateKeyBinding(Keyboard::Key key, Controller::Button button) 
        { 
            BUTTON_MAP[static_cast<int>(key)] = static_cast<int>(button); 
            return *this; 
        }
	}
}