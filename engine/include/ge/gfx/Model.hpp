#pragma once

#include <vector>

#include <glm/glm.hpp>
#include <tiny_obj_loader.h>

#include <ge/components/CenterOfMass.hpp>
#include <ge/systems/ResourceSystem.hpp>

namespace GE
{
	namespace Gfx
	{
		struct ModelObject
		{
			std::vector<glm::vec3> vertices;
			std::vector<glm::vec2> texCoords;
			std::vector<glm::vec3> normals;
			std::vector<glm::vec3> colors;

			CenterOfMass centerOfMass;

			std::vector<float> texture_id;
		};

		class Model : public Sys::Resource
		{
		public:
			Model()
				: Resource({})
			{}

			Model(Sys::ResourceData* data)
				: Resource(data)
			{
				_path = data->path + ".obj";
				std::string mtlFilePath = data->path + ".mtl";
				if (Utils::FileExist(mtlFilePath.c_str()))
					_mtlPath = mtlFilePath;
			}

			virtual void Load() override;
			virtual void LoadFromStorage() override;
			void Load(std::vector<char>& data);
			virtual void Unload() override;

			virtual bool LimitToMainThread() override { return false; }

			std::vector<ModelObject> objects;
			std::vector<tinyobj::material_t> materials;
			std::string _path;
			std::string _mtlPath;

			std::vector<char> _dataFromStorage;
			std::vector<char> _mtl_data;
		};
	}
}
