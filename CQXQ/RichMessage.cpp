#include <string>
#include "RichMessage.h"
using namespace std;

string constructXMLShareMsg(const string& url,
	const string& title,
	const string& content,
	const string& picUrl)
{
	std::string ret = R"(<?xml version='1.0' encoding='UTF-8' standalone='yes'?><msg templateID="12345" )";
	ret += R"(url=")" + url + R"(" serviceID="1" action="web" actionData="" a_actionData="" i_actionData="" )";
	ret += R"(brief=")" + (title.empty() ? "[·ÖÏí]" : title) + R"(" flag="0">)";
	ret += R"(<item layout="2">)";
	if (!picUrl.empty())
	{
		ret += R"(<picture cover=")" + picUrl + R"("/>)";
	}
	if (!title.empty())
	{
		ret += R"(<title>)" + title + R"(</title>)";
	}
	if (!content.empty())
	{
		ret += R"(<summary>)" + content + R"(</summary>)";
	}
	ret += R"(</item></msg>)";
	return ret;
}

