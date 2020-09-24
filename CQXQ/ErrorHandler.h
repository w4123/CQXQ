#pragma once
#include <Windows.h>
#include <functional>

LONG WINAPI CQXQUnhandledExceptionFilter(
	LPEXCEPTION_POINTERS ExceptionInfo
);

template <typename T, typename... Args>
std::function<T(Args...)> __stdcall ExceptionWrapper(T(__stdcall* func)(Args...))
{
	return std::function<T(Args...)>([func](Args&&... args) -> T
	{
		__try
		{
			return func(std::forward<Args>(args)...);
		}
		__except (CQXQUnhandledExceptionFilter(GetExceptionInformation()))
		{
			;
		}
		return T();
	});
}