#pragma once

#include "ge/core/Application.hpp"

#if !defined(_DEBUG)
#include <Windows.h>
#endif

extern std::unique_ptr<GE::Application> CreateApplication();

int APIENTRY WinMain(HINSTANCE, HINSTANCE, PSTR, int)
{
#if !defined(_DEBUG)
	FreeConsole();
#endif
	GE::CreateApplication()->Run();
	return 0;
}