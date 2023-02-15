#include <ge/utils/Types.hpp>

uint32_t operator""_FLOAT(unsigned long long value)
{
	return static_cast<uint32_t>(value * sizeof(float));
}