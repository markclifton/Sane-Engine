#include <ge/core/Global.hpp>

#include <ge/utils/Common.hpp>

namespace GE
{
	const int MAJOR_VERSION = 1;
	const int MINOR_VERSION = 2;
	const int PATCH_VERSION = 0;
	const char* ENGINE_NAME = "SaneEngine+";

	void UpdateGlobalDispatcher()
	{
		GlobalDispatcher().update();
	}

	entt::registry& GlobalRegistry()
	{
		return *Global::Get().registry;
	}

	entt::dispatcher& GlobalDispatcher()
	{
		return *Global::Get().dispatcher;
	}

	Utils::ThreadPool& GlobalThreadPool()
	{
		return Global::Get().pool;
	}

	Global& Global::Get()
	{
		static Global global;
		return global;
	}

	void Global::Initialize()
	{
		if (dispatcher || registry)
		{
			GE_DEBUG("GE::Global already initialized");
			return;
		}

		GE_INFO("GooseEngine+: Version: {}.{}.{}", GE::MAJOR_VERSION, GE::MINOR_VERSION, GE::PATCH_VERSION);

		dispatcher = std::make_unique<entt::dispatcher>();
		registry = std::make_unique<entt::registry>();
	}

	void Global::Release()
	{
		if (dispatcher)
		{
			dispatcher->clear();
			dispatcher.reset();
		}

		if (registry)
		{
			registry->clear();
			registry.reset();
		}
	}
}