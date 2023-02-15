#pragma once

#include <memory>

#include <ge/gfx/Window.hpp>
#include <ge/systems/ImguiSystem.hpp>
#include <ge/systems/InputSystem.hpp>
#include <ge/systems/Stack.hpp>

namespace GE
{
	namespace Sys {
		class InputSystem;
	}

	class Application
	{
	public:
		Application(const char*);
		virtual ~Application();

		virtual void Create() = 0;
		virtual void Destroy() = 0;
		virtual bool Update(int64_t tsMicroseconds) = 0;

		void PushSystem(Sys::System* layer);
		void PopSystem(Sys::System* layer);

		void PushImgui(Sys::System* imgui);
		void PushGraphicsSystem(Sys::System* sys);

		void Run();

	protected:
		bool _running{ false };

	private:
		Sys::Stack _systemsStack;
	};

	class GfxApplication : public Application
	{
		friend class Sys::InputSystem;

	public:
		GfxApplication(const char* name);
		virtual ~GfxApplication();

		void WaitForWindowIdle();
		bool ShouldClose();

	protected:
		GE::Sys::ImguiSystem _imgui;
		GE::Gfx::VulkanWindow _window;
	};

	std::unique_ptr<Application> CreateApplication();
}