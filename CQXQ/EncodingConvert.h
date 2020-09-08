#pragma once
#include <string>
#include "iconv.h"

template <typename T, typename Q>
std::basic_string<T> ConvertEncoding(const std::basic_string<Q>& in, const std::string& InEnc, const std::string& OutEnc, const double CapFac = 2.0)
{
	const auto cd = iconv_open(OutEnc.c_str(), InEnc.c_str());
	if (cd == (iconv_t)-1)
	{
		return std::basic_string<T>();
	}
	size_t in_len = in.size() * sizeof(Q);
	size_t out_len = size_t(in_len * CapFac + sizeof(T));
	const char* in_ptr = reinterpret_cast<const char*> (in.c_str());
	char* out_ptr = new char[out_len]();

	// As out_ptr would be modified by iconv(), store a copy of it pointing to the beginning of the array
	char* out_ptr_copy = out_ptr;
	if (iconv(cd, &in_ptr, &in_len, &out_ptr, &out_len) == (size_t)-1)
	{
		delete[] out_ptr_copy;
		iconv_close(cd);
		return std::basic_string<T>();
	}
	memset(out_ptr, 0, sizeof(T));
	std::basic_string<T> ret(reinterpret_cast<T*>(out_ptr_copy));
	delete[] out_ptr_copy;
	iconv_close(cd);
	return ret;
}

std::string UTF8toGB18030(const std::string& strUTF8);

std::string GB18030toUTF8(const std::string& strGB18030);

std::wstring GB18030toUTF16(const std::string& strGB18030);

std::string UTF16toGB18030(const std::wstring& strUTF16);