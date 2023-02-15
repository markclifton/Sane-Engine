#pragma once

#include <vector>

namespace GE
{
	namespace Sys
	{
        class System;

        class Stack
        {
        public:
            ~Stack();

            void PushSystem(System* layer);
            void PopSystem(System* layer);

            void PushImgui(System* imgui);
            void PushGraphicsSystem(System* sys);

            inline std::vector<System*>::iterator begin() { return systems_.begin(); }
            inline std::vector<System*>::iterator end() { return systems_.end(); }
            inline std::vector<System*>::reverse_iterator rbegin() { return systems_.rbegin(); }
            inline std::vector<System*>::reverse_iterator rend() { return systems_.rend(); }

            inline std::vector<System*>::const_iterator begin() const { return systems_.begin(); }
            inline std::vector<System*>::const_iterator end()	const { return systems_.end(); }
            inline std::vector<System*>::const_reverse_iterator rbegin() const { return systems_.rbegin(); }
            inline std::vector<System*>::const_reverse_iterator rend() const { return systems_.rend(); }

            System* _imguiSystem{ nullptr };
            System* _gfxSystem{ nullptr };

        private:
            std::vector<System*> systems_;
        };
	}
}