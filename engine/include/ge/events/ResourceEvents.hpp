#pragma once

#include <ge/core/Global.hpp>
#include <ge/utils/UUID.hpp>
#include <ge/systems/Systems.hpp>

namespace GE
{

	struct ResourceLoaded
	{
		Utils::UUID uuid;
	};

	class ResourceSink : virtual public Sys::System
	{
	public:
		ResourceSink();

		void Attach();
		void Detach();

		void OnResourceUpdated(const ResourceLoaded& resource);
		void RegisterUUID(GE::Utils::UUID uuid);

		bool ResourcesUpdated = false;

	private:
		std::vector<Utils::UUID> _resources;
	};
}