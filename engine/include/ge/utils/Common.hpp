#pragma once

#include <string>

#include <ge/utils/Assert.hpp>
#include <ge/utils/BlobParser.hpp>
#include <ge/utils/FileLoading.hpp>
#include <ge/utils/GuardedType.hpp>
#include <ge/utils/Log.hpp>
#include <ge/utils/Mutex.hpp>
#include <ge/utils/Threadpool.hpp>
#include <ge/utils/Types.hpp>
#include <ge/utils/UniqueName.hpp>
#include <ge/utils/UUID.hpp>
#include <ge/utils/XML.hpp>

#define GE_UNUSED(x) (void)(x)

namespace GE
{
	namespace Utils
	{
		bool Contains(const std::string& a, const std::string& b);
	}
}