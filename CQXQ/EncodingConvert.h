#pragma once
#include <string>

std::string ConvertEncoding(const std::string& in, const std::string& InEnc, const std::string& OutEnc, const double CapFac = 2.0);

std::string UTF8toGB18030(const std::string& strUTF8);