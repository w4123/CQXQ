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

