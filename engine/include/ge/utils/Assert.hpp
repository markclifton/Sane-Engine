#pragma once

#ifdef _DEBUG
#include <ge/utils/Log.hpp>

#pragma warning( disable : 4002 4003 )
#include <cassert>

#define FIRST_ARG_(N, ...) N
#define FIRST_ARG(args) FIRST_ARG_ args
#define REST_ARG_(N, ...) __VA_ARGS__
#define REST_ARG(args) REST_ARG_ args
#define GE_ASSERT(TEST, ...)\
	if(!(TEST)){\
		auto msg=std::string(FIRST_ARG((__VA_ARGS__)));\
		if(!msg.empty()) GE_ERROR(msg.c_str(),REST_ARG((__VA_ARGS__,)) 0);\
		assert(TEST);\
	}

#else
#define GE_ASSERT(...) 
#endif