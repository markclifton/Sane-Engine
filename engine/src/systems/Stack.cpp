#include <ge/systems/Stack.hpp>

#include <ge/systems/Systems.hpp>
#include <ge/utils/Common.hpp>

namespace GE
{
    namespace Sys
    {
        Stack::~Stack()
        {
#ifdef _DEBUG
            for (auto& system : systems_)
            {
                GE_ERROR("Failed to cleanup {} system", system->Name());
            }
#endif
        }

        void Stack::PushSystem(System* sys)
        {
            systems_.emplace(systems_.begin() + systems_.size(), sys);
            sys->OnAttach();
        }

        void Stack::PopSystem(System* sys)
        {
            auto it = std::find(systems_.begin(), systems_.end(), sys);
            GE_ASSERT(it != systems_.end(), "Failed to pop system {}", sys->Name());
            systems_.erase(it);
            sys->OnDetach();
        }

        void Stack::PushImgui(System* imgui)
        {
            _imguiSystem = imgui;
        }

        void Stack::PushGraphicsSystem(System* sys)
        {
            _gfxSystem = sys;
        }
    }
}