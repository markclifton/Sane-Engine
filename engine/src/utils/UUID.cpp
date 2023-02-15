#include <ge/utils/UUID.hpp>

#include <random>

namespace
{
	uint64_t GetUUID()
	{
		static std::random_device S_RANDOMDEVICE;
		static std::mt19937_64 S_ENGINE(S_RANDOMDEVICE());
		std::uniform_int_distribution<uint64_t> uniformDistribution;

		return uniformDistribution(S_ENGINE);
	}
}

namespace GE
{
	namespace Utils
	{
		UUID::UUID()
			: _id(GetUUID())
		{
		}

		UUID::UUID(uint64_t uuid)
			: _id(uuid)
		{
		}

		UUID::UUID(int64_t uuid)
			: _id(uuid)
		{
		}
	}
}