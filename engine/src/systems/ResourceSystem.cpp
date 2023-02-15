#include <ge/systems/ResourceSystem.hpp>

#include <ge/core/Common.hpp>
#include <ge/core/Global.hpp>
#include <ge/events/ResourceEvents.hpp>

#if defined(NN_BUILD_TARGET_PLATFORM_NX)
#include <nn/oe.h>
#endif

namespace GE
{
	namespace Sys
	{
		ResourceSystem::ResourceSystem(const char* resourceManifest)
			: System("ResourceSystem")
			, _resourceManifest(resourceManifest)
		{
			REGISTER_SYSTEM();
		}

		void ResourceSystem::Update(int64_t tsMicroseconds)
		{
			int loadedFromStorage = 0;
			while (loadedFromStorage < 1 && !_loadFromDiskResources.empty())
			{
				auto uuid = _loadFromDiskResources.front();
				_resources[uuid]->LoadFromStorage();
				_loadResources.push_back(uuid); 

				LOCK(_mutex);
				_loadFromDiskResources.pop_front();
				loadedFromStorage++;
				
				return;
			}

			while (!_loadResources.empty() && _numProcessing < 4)
			{
				GE::Utils::UUID uuid;
				{
					LOCK(_mutex);
					uuid = _loadResources.front();
					_loadResources.pop_front();
					_numProcessing++;
				}

				if (_resources[uuid]->LimitToMainThread()) {
					_resources[uuid]->Load();
					ResourceLoaded res {uuid};
					GlobalDispatcher().trigger(res);
					{
						LOCK(_mutex);
						_numProcessing--;
					}
					return;
				}
				else {
					GlobalThreadPool().enqueue([this, uuid]() {
						_resources[uuid]->Load();

						LOCK(_mutex);
						_numProcessing--;

						ResourceLoaded res {uuid};
						GlobalDispatcher().trigger(res);
					});
				}
			}

			int newlyFreed = 0;
			while (newlyFreed < 6 && !_unloadResources.empty())
			{
				LOCK(_mutex);
				auto uuid = _unloadResources.front();
				if (_resourceRefCounts[_unloadResources.front()] == 0)
				{
					_resources[uuid]->Unload();
					delete _resources[uuid];
					_resources.erase(uuid);
					newlyFreed++;
				}
				_unloadResources.pop_front();
			}
		}

		void ResourceSystem::Attach()
		{
			LOCK(_mutex);

			auto data = Utils::LoadFile(_resourceManifest.c_str());
			tinyxml2::XMLDocument doc;
			doc.Parse(data.data(), data.size());
			GE_ASSERT(!doc.Error(), "Failed to parse Manifest!");

			tinyxml2::XMLNode* pRoot = doc.RootElement();
			if (pRoot != nullptr)
			{
				auto category = pRoot->FirstChild();
				if (category)
				{
					while (category != nullptr)
					{
						auto resource = category->FirstChildElement();
						while (resource)
						{
							_availableResources[resource->Unsigned64Attribute("uuid")] = ResourceData(resource->Attribute("name"), resource->Attribute("path"));
							_resourceRefCounts[resource->Unsigned64Attribute("uuid")] = 0;
							resource = resource->NextSiblingElement();
						}
						category = category->NextSibling();
					}
				}
			}
		}

		void ResourceSystem::Detach()
		{
			while (_numProcessing != 0); //Wait for threads to finish

			LOCK(_mutex);
			_availableResources.clear();
			_resourceRefCounts.clear();
			_loadFromDiskResources.clear();
			_loadResources.clear();
			_unloadResources.clear();
			for (auto& resource : _resources)
			{
				resource.second->Unload();
				delete resource.second;
			}
			_resources.clear();
		}

		Utils::UUID ResourceSystem::LookupResource(const std::string& path)
		{
			for (auto& [uuid, data] : _availableResources)
			{
				if (data.path == path) {
					return uuid; 
				}
			}

			return Utils::UUID((uint64_t)0);
		}

		Resource* ResourceSystem::GetResource(Utils::UUID uuid)
		{
			auto resource = _resources[uuid];
			return resource->_isLoaded ? resource : nullptr;
		}

		void ResourceSystem::UnloadResource(const Utils::UUID& uuid)
		{
			LOCK(_mutex);
			_resourceRefCounts[uuid]--;
			if (_resourceRefCounts[uuid] == 0 && _resources[uuid]->_isLoaded)
				_unloadResources.push_back(uuid);
		}
	}
}