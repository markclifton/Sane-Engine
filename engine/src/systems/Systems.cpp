#include <ge/systems/Systems.hpp>

namespace GE
{
    namespace Sys
    {
        System::System(const std::string& name)
            : _name(name)
        {}

        void System::RegisterSystem(std::function<void(void)> onAttach, std::function<void(void)> onDetach)
        {
            _onAttachFns.push_back(onAttach);
            _onDetachFns.push_back(onDetach);
        }

        void System::Update(int64_t tsMicroseconds) {}
        void System::RenderScene() {}
        void System::RenderGui() {}

        void System::OnAttach() 
        {
            for (auto& onAttach : _onAttachFns) {
                onAttach();
            }
        }
        
        void System::OnDetach() 
        {
            for (auto& onDetach : _onDetachFns) {
                onDetach();
            }
        }

        void System::Begin() {}
        void System::End() {}
    }
}