#pragma once

#include <functional>
#include <string>
#include <vector>

#define REGISTER_SYSTEM() RegisterSystem([this](){Attach();},[this](){Detach();}); 

namespace GE
{
    namespace Sys
    {
        class System
        {
        public:
            System(const std::string& name);
            virtual void RegisterSystem(std::function<void(void)> onAttach, std::function<void(void)> onDetach) final;

            virtual void Update(int64_t tsMicroseconds);
            virtual void RenderScene();
            virtual void RenderGui();

            virtual void OnAttach() final;
            virtual void OnDetach() final;

            virtual void Begin();
            virtual void End();

            inline const char* Name() { return _name.c_str(); }
        
        private:
            std::string _name;

            std::vector<std::function<void(void)>> _onAttachFns;
            std::vector<std::function<void(void)>> _onDetachFns;
        };
    }
}