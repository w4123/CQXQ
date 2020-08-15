#include <string>
#include "iconv.h"
#include "EncodingConvert.h"

std::string ConvertEncoding(const std::string& in, const std::string& InEnc, const std::string& OutEnc, const double CapFac)
{
	const auto cd = iconv_open(OutEnc.c_str(), InEnc.c_str());
	if (cd == (iconv_t)-1)
	{
		return "";
	}
	size_t in_len = in.size();
	size_t out_len = size_t(in_len * CapFac + 1);
	const char* in_ptr = in.c_str();
	char* out_ptr = new char[out_len];

	// As out_ptr would be modified by iconv(), store a copy of it pointing to the beginning of the array
	char* out_ptr_copy = out_ptr;
	if (iconv(cd, &in_ptr, &in_len, &out_ptr, &out_len) == (size_t)-1)
	{
		delete[] out_ptr_copy;
		iconv_close(cd);
		return "";
	}
	*out_ptr = '\0';
	std::string ret(out_ptr_copy);
	delete[] out_ptr_copy;
	iconv_close(cd);
	return ret;
}

std::string UTF8toGB18030(const std::string& strUTF8)
{
	return ConvertEncoding(strUTF8, "utf-8", "gb18030");
}