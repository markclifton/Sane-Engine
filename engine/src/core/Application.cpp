#include <ge/core/Application.hpp>

#include <chrono>

#include <ge/core/Global.hpp>
#include <ge/systems/Systems.hpp>
#include <ge/systems/ResourceSystem.hpp>

namespace GE
{
	Application::Application(const char*)
	{
		Global::Get().Initialize();
		GE::Sys::ResourceSystem::Get().OnAttach();
	}

	Application::~Application()
	{
		GE::Sys::ResourceSystem::Get().OnDetach();
		Global::Get().Release();
	}

	void Application::Run()
	{
		Create();

		std::chrono::high_resolution_clock::time_point tpPrevious = std::chrono::high_resolution_clock::now();
		std::chrono::high_resolution_clock::time_point tpCurrent = std::chrono::high_resolution_clock::now();

		long long tsMicroseconds = std::chrono::duration_cast<std::chrono::microseconds>(tpCurrent - tpPrevious).count();

		while ((_running = Update(tsMicroseconds)))
		{
			tpCurrent = std::chrono::high_resolution_clock::now();
			tsMicroseconds = std::chrono::duration_cast<std::chrono::microseconds>(tpCurrent - tpPrevious).count();
			tpPrevious = tpCurrent;

			GE::UpdateGlobalDispatcher();
			GE::Sys::ResourceSystem::Get().Update(tsMicroseconds);

			for (auto& system : _systemsStack)
			{
				system->Update(tsMicroseconds);
			}

			if (_systemsStack._gfxSystem)
			{
				_systemsStack._gfxSystem->Begin();

				for (auto& system : _systemsStack)
				{
					system->RenderScene();
				}

				if (_systemsStack._imguiSystem) 
				{
					_systemsStack._imguiSystem->Update(tsMicroseconds);
					_systemsStack._imguiSystem->Begin();

					for (auto& system : _systemsStack)
					{
						system->RenderGui();
					}

					_systemsStack._imguiSystem->End();
				}

				_systemsStack._gfxSystem->End();
			}
		}

		Destroy();
	}

	void Application::PushSystem(Sys::System* layer)
	{
		_systemsStack.PushSystem(layer);
	}

	void Application::PopSystem(Sys::System* layer)
	{
		_systemsStack.PopSystem(layer);
	}

	void Application::PushImgui(Sys::System* imgui)
	{
		_systemsStack.PushImgui(imgui);
	}
	
	void Application::PushGraphicsSystem(Sys::System* sys)
	{
		_systemsStack.PushGraphicsSystem(sys);
	}

	GfxApplication::GfxApplication(const char* name)
		: Application(name)
	{
		_window.Create();
		PushGraphicsSystem(&_window);

		_imgui.Configure();
		_imgui.Create();
		PushImgui(&_imgui);
	}

	GfxApplication::~GfxApplication()
	{
		_imgui.Destroy();
		Application::~Application();
		_window.Destroy();
	}

	void GfxApplication::WaitForWindowIdle()
	{
		vkDeviceWaitIdle(*Gfx::VulkanCore::Get().device);
	}

	bool GfxApplication::ShouldClose()
	{
		return _window.ShouldClose();
	}
}