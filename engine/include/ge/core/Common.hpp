#pragma once

#include <string>

#include <ge/utils/UniqueName.hpp>

#ifdef _DEBUG
#define TIMED(x) GE::Timed PRIVATE_NAME(timed)(x);
#ifdef _WIN32
#define TRACE() GE::Trace PRIVATE_NAME(trace)(__FUNCTION__ );
#define TIMED_TRACE() GE::TraceWithTime PRIVATE_NAME(trace)(__FUNCTION__ );
#elif defined(NN_BUILD_TARGET_PLATFORM_NX)
#define TRACE() GE::Trace PRIVATE_NAME(trace)(__PRETTY_FUNCTION__ );
#define TIMED_TRACE() GE::TraceWithTime PRIVATE_NAME(trace)(__PRETTY_FUNCTION__ );
#endif
#else
#define TRACE() 
#define TIMED_TRACE() 
#define TIMED(x)
#endif

namespace GE
{
	class Trace
	{
	public:
		Trace(std::string name);
		~Trace();

	private:
		std::string _name;
	};

	class TraceWithTime
	{
	public:
		TraceWithTime(std::string name, int64_t* out = nullptr);
		~TraceWithTime();

	private:
		std::string _name;
		int64_t _time;
		int64_t* _out;
	};

	class Timed
	{
	public:
		Timed(int64_t* out = nullptr);
		~Timed();

		int64_t Tear();

	private:
		int64_t _time;
		int64_t* _out;
		int64_t _placeholderTime;
	};
}
