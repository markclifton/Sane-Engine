#pragma once

#include <cstdint>
#include <functional>

namespace GE
{
	namespace Utils
	{
		class UUID
		{
		public:
			UUID();
			UUID(uint64_t uuid);
			UUID(int64_t uuid);
			UUID(const UUID&) = default;

			operator uint64_t() const { return _id; }

		private:
			uint64_t _id;
		};
	}
}

namespace std {
	template <> struct hash<GE::Utils::UUID>
	{
		size_t operator()(const GE::Utils::UUID& uuid) const
		{
			return hash<uint64_t>()((uint64_t)uuid);
		}
	};
}
