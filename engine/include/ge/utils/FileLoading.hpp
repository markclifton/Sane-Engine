#pragma once

#include <vector>

namespace GE
{
	namespace Utils
	{
		std::vector<char> LoadFile(const char* path, int64_t offset, size_t size);
		std::vector<char> LoadFile(const char* path);
		bool FileExist(const char* path);
	}
}