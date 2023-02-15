#include <ge/utils/Common.hpp>

namespace GE
{
	namespace Utils
	{
		bool Contains(const std::string& a, const std::string& b)
		{
			return (a.find(b) != std::string::npos);
		}
	}
}