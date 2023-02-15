#include <ge/utils/FileLoading.hpp>

#include <fstream>
#include <string>

#include <ge/utils/Common.hpp>

namespace GE
{
	namespace Utils
	{
        std::vector<char> LoadFile(const char* path, int64_t offset, size_t size)
        {
            std::string full_path = std::string("resources/") + path;

            std::ifstream ifs(full_path.c_str(), std::ios_base::binary);
            GE_ASSERT(ifs.is_open(), "Failed to open file: {}", full_path.c_str());

            ifs.ignore(offset);

            std::vector<char> filebuffer;
            filebuffer.resize(size);

            ifs.read(filebuffer.data(), size);

            ifs.close();

            return filebuffer;
        }

		std::vector<char> LoadFile(const char* path)
		{
            std::string full_path = std::string("resources/") + path;

			std::ifstream file(full_path.c_str(), std::ios::binary | std::ios::ate);
            GE_ASSERT(file.is_open(), "Failed to open file: {}", full_path.c_str());

			size_t fileSize = (size_t)file.tellg();
			std::vector<char> filebuffer(fileSize);

			file.seekg(0);
			file.read(filebuffer.data(), fileSize);

			file.close();

			return filebuffer;
		}

        bool FileExist(const char* path)
        {
            std::string full_path = std::string("resources/") + path;
            std::ifstream file(full_path.c_str(), std::ios::binary | std::ios::ate);
            return file.is_open();
        }
	}
}