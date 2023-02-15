#include <ge/core/Common.hpp>

#include <chrono>

#include <ge/utils/Log.hpp>

namespace GE
{
	Trace::Trace(std::string name)
		: _name(name)
	{
		GE_TRACE("Begin: {}", _name);
	}

	Trace::~Trace()
	{
		GE_TRACE("End  : {}", _name);
	}

	TraceWithTime::TraceWithTime(std::string name, int64_t* out)
		: _name(name)
		, _time(std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::high_resolution_clock::now().time_since_epoch()).count())
		, _out(out)
	{
		GE_TRACE("Begin: {}", _name);
	}

	TraceWithTime::~TraceWithTime()
	{
		int64_t finalTime = (std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::high_resolution_clock::now().time_since_epoch()).count() - _time) / 1000;
		GE_TRACE("End  : {} -- total time taken: {}ms", _name, finalTime);
		if (_out) *_out = finalTime;
	}

	Timed::Timed(int64_t* out)
		: _time(std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::high_resolution_clock::now().time_since_epoch()).count())
		, _out(out ? out : &_placeholderTime)
	{
	}

	Timed::~Timed()
	{
		int64_t finalTime = (std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::high_resolution_clock::now().time_since_epoch()).count() - _time) / 1000;
		*_out = finalTime;
	}

	int64_t Timed::Tear()
	{
		return (std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::high_resolution_clock::now().time_since_epoch()).count() - _time) / 1000;
	}
}