#include "ge/gfx/Model.hpp"

#include <fstream>

#include <ge/core/Common.hpp>

#define TINYOBJLOADER_IMPLEMENTATION
#include "tiny_obj_loader.h"

#include <ge/utils/FileLoading.hpp>

namespace
{
	// From: https://stackoverflow.com/questions/8815164/c-wrapping-vectorchar-with-istream
	template<typename CharT, typename TraitsT = std::char_traits<CharT> >
	class vectorwrapbuf : public std::basic_streambuf<CharT, TraitsT> {
	public:
		vectorwrapbuf(std::vector<CharT>& vec) {
			std::streambuf::setg(vec.data(), vec.data(), vec.data() + vec.size());
		}
	};
}

namespace GE
{
	namespace Gfx
	{
		void Model::Load()
		{
			Load(_dataFromStorage);
		}

		void Model::LoadFromStorage()
		{
			_dataFromStorage = Utils::LoadFile(_path.c_str());
			if (!_mtlPath.empty())
				_mtl_data = Utils::LoadFile(_mtlPath.c_str());
		}

		void Model::Load(std::vector<char>& data)
		{
			if (_isLoaded)
				return;

			vectorwrapbuf<char> databuf(data);
			std::istream is(&databuf);

			tinyobj::attrib_t attrib;
			std::vector<tinyobj::shape_t> shapes;

			std::string warn;
			std::string err;
			bool result;

			if (!_mtlPath.empty())
			{
				vectorwrapbuf<char> mtl_databuf(_mtl_data);
				std::istream mtl_is(&mtl_databuf);
				tinyobj::MaterialStreamReader mtl_ss(mtl_is);
				result = tinyobj::LoadObj(&attrib, &shapes, &materials, &warn, &err, &is, &mtl_ss);
			}
			else
			{
				result = tinyobj::LoadObj(&attrib, &shapes, &materials, &warn, &err, &is, nullptr);
			}
			GE_ASSERT(result, "Failed to load model: {}", _path.c_str());
			GE_UNUSED(result);

			for (size_t s = 0; s < shapes.size(); s++) {
				size_t index_offset = 0;
				ModelObject object;

				for (size_t f = 0; f < shapes[s].mesh.num_face_vertices.size(); f++) {
					size_t fv = size_t(shapes[s].mesh.num_face_vertices[f]);

					object.centerOfMass = glm::vec3{ 0 };
					for (size_t v = 0; v < fv; v++) {
						tinyobj::index_t idx = shapes[s].mesh.indices[index_offset + v];
						tinyobj::real_t vx = attrib.vertices[3 * size_t(idx.vertex_index) + 0];
						tinyobj::real_t vy = attrib.vertices[3 * size_t(idx.vertex_index) + 1];
						tinyobj::real_t vz = attrib.vertices[3 * size_t(idx.vertex_index) + 2];

						object.vertices.push_back({ vx, vy, vz });
						object.texture_id.push_back(static_cast<float>(shapes[s].mesh.material_ids[f]));

						object.centerOfMass += object.vertices.back();

						if (idx.normal_index >= 0) {
							tinyobj::real_t nx = attrib.normals[3 * size_t(idx.normal_index) + 0];
							tinyobj::real_t ny = attrib.normals[3 * size_t(idx.normal_index) + 1];
							tinyobj::real_t nz = attrib.normals[3 * size_t(idx.normal_index) + 2];

							object.normals.push_back({ nx, ny, nz });
						}

						if (idx.texcoord_index >= 0) {
							tinyobj::real_t tx = attrib.texcoords[2 * size_t(idx.texcoord_index) + 0];
							tinyobj::real_t ty = attrib.texcoords[2 * size_t(idx.texcoord_index) + 1];

							object.texCoords.push_back({ tx, ty });
						}
					}

					index_offset += fv;
				}
				
				object.centerOfMass /= object.vertices.size();

				objects.push_back(object);
			}

			_isLoaded = true;
			_dataFromStorage.clear();
			_mtlPath.clear();
		}

		void Model::Unload()
		{
			_isLoaded = false;

			objects.clear();
			materials.clear();
		}

	}
}