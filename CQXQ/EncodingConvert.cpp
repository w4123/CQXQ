#include <string>
#include "iconv.h"
#include "EncodingConvert.h"

std::string UTF8toGB18030(const std::string& strUTF8)
{
	return ConvertEncoding<char>(strUTF8, "utf-8", "gb18030");
}

std::string GB18030toUTF8(const std::string& strGB18030)
{
	return ConvertEncoding<char>(strGB18030, "gb18030", "utf-8");
}

std::wstring GB18030toUTF16(const std::string& strGB18030)
{
	return ConvertEncoding<wchar_t>(strGB18030, "gb18030", "utf-16le");
}

std::string UTF16toGB18030(const std::wstring& strUTF16)
{
	return ConvertEncoding<char>(strUTF16, "utf-16le", "gb18030");
}

