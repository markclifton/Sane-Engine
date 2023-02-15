#pragma once

#include <memory>

#include <ge/utils/Threadpool.hpp>

#pragma warning( push )
#pragma warning( disable : 4307)
#include <entt/entt.hpp>
#pragma warning( pop )

namespace GE
{
	extern const int MAJOR_VERSION;
	extern const int MINOR_VERSION;
	extern const int PATCH_VERSION;
	extern const char* ENGINE_NAME;

	void UpdateGlobalDispatcher();
	entt::registry& GlobalRegistry();
	entt::dispatcher& GlobalDispatcher();
	Utils::ThreadPool& GlobalThreadPool();

	class Global
	{
		friend entt::registry& GlobalRegistry();
		friend entt::dispatcher& GlobalDispatcher();
		friend Utils::ThreadPool& GlobalThreadPool();
	public:
		static Global& Get();

		void Initialize();
		void Release();

	private:
		std::unique_ptr<entt::dispatcher> dispatcher{ nullptr };
		std::unique_ptr<entt::registry> registry{ nullptr };
		Utils::ThreadPool pool{ 8 };
	};
}