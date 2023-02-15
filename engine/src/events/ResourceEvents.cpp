#include <ge/events/ResourceEvents.hpp>

namespace GE
{
	ResourceSink::ResourceSink()
		: GE::Sys::System("ResourceSink")
	{
		REGISTER_SYSTEM();
	}

	void ResourceSink::Attach() {
		GlobalDispatcher().sink<ResourceLoaded>().connect<&ResourceSink::OnResourceUpdated>(this);
	}

	void ResourceSink::Detach() {
		GlobalDispatcher().sink<ResourceLoaded>().disconnect<&ResourceSink::OnResourceUpdated>(this);
	}

	void ResourceSink::OnResourceUpdated(const ResourceLoaded& resource) {
		for (auto& uuid : _resources) {
			if (uuid == resource.uuid) {
				ResourcesUpdated = true;
				return;
			}
		}
	}

	void ResourceSink::RegisterUUID(GE::Utils::UUID uuid) {
		_resources.push_back(uuid);
	}
}