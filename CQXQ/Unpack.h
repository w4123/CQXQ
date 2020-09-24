#pragma once
#include <iostream>
#include <vector>
#include <string>

//打印内存数据
static void show(void* t, const int len)
{
	auto* const p = static_cast<unsigned char*>(t);
	std::cout << "{";
	for (auto i = 0; i < len; ++i)
	{
		std::cout << static_cast<int>(p[i]);
		if (i != len - 1)std::cout << ", ";
	}
	std::cout << "}" << std::endl;
}

//内存翻转
static unsigned char* Flip(unsigned char* const str, int len) noexcept
{
	auto f = 0;
	--len;
	while (f < len)
	{
		const auto p = str[len];
		str[len] = str[f];
		str[f] = p;
		++f;
		--len;
	}
	return str;
}

//到字节集...
//在原有的数据基础上操作
template <typename ClassType>
unsigned char* toBin(ClassType& i) noexcept
{
	return Flip(reinterpret_cast<unsigned char*>(&i), sizeof(ClassType));
}


class Unpack final
{
	std::vector<unsigned char> buff;
public:
	Unpack() noexcept;
	explicit Unpack(const char*);
	explicit Unpack(std::vector<unsigned char>) noexcept;
	explicit Unpack(const std::string&);

	Unpack& setData(const char* i, int len);
	Unpack& clear() noexcept;
	[[nodiscard]] int len() const noexcept;

	Unpack& add(int i); //添加一个整数
	int getInt() noexcept; //弹出一个整数

	Unpack& add(long long i); //添加一个长整数
	long long getLong() noexcept; //弹出一个长整数

	Unpack& add(short i); //添加一个短整数
	short getshort() noexcept; //弹出一个短整数

	Unpack& add(const unsigned char* i, short len); //添加一个字节集(请用add(std::string i);)
	std::vector<unsigned char> getchars() noexcept; //弹出一个字节集(请用getstring();)

	Unpack& add(std::string i); //添加一个字符串
	std::string getstring(); //弹出一个字符串

	Unpack& add(Unpack& i); //添加一个Unpack
	Unpack getUnpack() noexcept; //弹出一个Unpack

	std::string getAll() noexcept; //返回本包数据

	void show();
};
