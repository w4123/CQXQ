#pragma once
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

LONG WINAPI CQXQUnhandledExceptionFilter(
	LPEXCEPTION_POINTERS ExceptionInfo
);