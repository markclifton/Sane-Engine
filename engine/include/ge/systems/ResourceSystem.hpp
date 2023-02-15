#pragma once

#include <deque>
#include <iostream>
#include <map>
#include <string>
#include <unordered_map>

#include <ge/systems/Systems.hpp>
#include <ge/utils/Common.hpp>

namespace GE
{
	namespace Sys
	{
		struct ResourceData
		{
			ResourceData() {};
			ResourceData(std::string name, std::string path) : name(name), path(path) {}
			std::string name;
			std::string path;
		};

		class Resource
		{
		public:
			Resource(ResourceData* data) : _data(data) {}
			virtual ~Resource() {};

			bool _isLoaded{ false };
			bool _isLoadedFromStorage{ false };

			ResourceData* _data{ nullptr };
			
			std::string& GetPath() const { return _data->path; }

			virtual void Load() = 0;
			virtual void LoadFromStorage() { _isLoadedFromStorage = true; };
			virtual void Unload() = 0;

			virtual bool LimitToMainThread() = 0;
		};

		class ResourceSystem : public System
		{
			ResourceSystem(const char* resourceManifest);
		public:
			static ResourceSystem& Get(const char* ManifestFile = nullptr) {
				static ResourceSystem rs(ManifestFile ? ManifestFile : "Manifest.xml");
				return rs;
			}

			virtual void Update(int64_t tsMicroseconds) override;
			void Attach();
			void Detach();

			Utils::UUID LookupResource(const std::string& path);
			Resource* GetResource(Utils::UUID uuid);

			template<class T>
			Resource* LoadResource(Utils::UUID& uuid, bool lazyLoad = true)
			{
				LOCK(_mutex);
				auto data = _availableResources.find(uuid);
				if (data != _availableResources.end())
				{
					if (_resources.count(uuid) > 0)
						return _resources[uuid];

					_resourceRefCounts[uuid]++;
					T* resource = new T(&data->second);
					if (lazyLoad)
						_loadFromDiskResources.push_back(uuid);
					else
					{
						resource->LoadFromStorage();
						resource->Load();
					}

					_resources[uuid] = resource;
					return resource;
				}
				GE_ASSERT(0, "Failed to find resource with uuid: {}", uuid);
				return nullptr;
			}

			void UnloadResource(const Utils::UUID& uuid);

		private:
			const std::string _resourceManifest;
			std::unordered_map<Utils::UUID, ResourceData> _availableResources;
			std::unordered_map<Utils::UUID, Resource*> _resources;
			std::unordered_map<Utils::UUID, uint32_t> _resourceRefCounts;

			std::deque<Utils::UUID> _loadResources;
			std::deque<Utils::UUID> _loadFromDiskResources;
			std::deque<Utils::UUID> _unloadResources;

			std::mutex _mutex;

			int32_t _numProcessing = 0;
		};

		template <class T>
		class ResourceHandle
		{
		public:
			ResourceHandle() {}
			ResourceHandle(uint64_t uuid, bool lazyLoad = true) :_uuid(uuid) { data = (T*)ResourceSystem::Get().LoadResource<T>(_uuid, lazyLoad); }
			ResourceHandle(Utils::UUID objUUID, bool lazyLoad = true) : _uuid(objUUID) { data = (T*)ResourceSystem::Get().LoadResource<T>(_uuid, lazyLoad); }
			~ResourceHandle() { ResourceSystem::Get().UnloadResource(_uuid); }

			void Load() { data->Load(); }
			T& Get() { return *data; }
			operator T& () { return *data; }
			T* operator->() { return data; }

		private:
			T* data{ nullptr };
			Utils::UUID _uuid;
		};
	}
}