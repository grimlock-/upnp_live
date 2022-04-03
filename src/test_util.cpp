#include <string>
#include <sstream>
#include <iostream>
#include "util.h"
#include "Logging.h"

using namespace upnp_live;

void test_TrimString()
{
	std::cout << "================================\n";
	std::cout << "TrimString()\n";
	std::string test_string = "   umps  ,over, the \",,,laz,,y dog ,  ,,,\" \t ";
	std::cout << "[" << test_string << "]\n";
	std::string newstr = util::TrimString(test_string);
	std::cout << "[" << newstr << "]\n";
	if(!newstr.size())
	{
		std::cout << "X - got empty string\n";
		return;
	}
	if(newstr == test_string)
	{
		std::cout << "X - string same after trim\n";
		return;
	}
	if(newstr.front() != 'u')
		std::cout << "X - expected first character 'u', got " << newstr.front() << "\n";
	if(newstr.back() != '"')
		std::cout << "X - expected last character '\"', got " << newstr.back() << "\n";
}
void test_TrimString_char()
{
	std::cout << "================================\n";
	std::cout << "TrimString(char)\n";
	std::string test_string = ",,,umps  ,over, the \",,,laz,,y dog ,  ,,,\",,";
	std::cout << "[" << test_string << "]\n";
	std::string newstr = util::TrimString(test_string, ',');
	std::cout << "[" << newstr << "]\n";
	if(!newstr.size())
	{
		std::cout << "X - got empty string\n";
		return;
	}
	if(newstr == test_string)
	{
		std::cout << "X - string same after trim\n";
		return;
	}
	if(newstr.front() != 'u')
		std::cout << "X - expected first character 'u', got " << newstr.front() << "\n";
	if(newstr.back() != '"')
		std::cout << "X - expected last character 'u', got " << newstr.back() << "\n";
}

void test_SplitString()
{
	std::cout << "================================\n";
	std::cout << "SplitString()\n";


	std::string test_string = "the quick brown fox\" jumps over the \"lazy dog\"";
	std::cout << "[" << test_string << "]\n";
	auto result = util::SplitString(test_string, ' ');
	std::cout << "Args:\n";
	for(auto a : result)
		std::cout << a << "\n";
	if(result.size() != 9)
		std::cout << "X - expected arg count of 9, got " << result.size() << "\n";


	test_string = "  t  he   quick  fox brow wopeir  a ";
	std::cout << "[" << test_string << "]\n";
	result = util::SplitString(test_string, ' ');
	std::cout << "values:\n";
	for(auto a : result)
	{
		if(a.size())
			std::cout << a << "\n";
		else
			std::cout << "(empty string)\n";
	}
	if(result.size() != 7)
		std::cout << "X - expected arg count of 7, got " << result.size() << "\n";
}

void test_SplitArgString()
{
	std::cout << "================================\n";
	std::cout << "SplitArgString()\n";
	std::string test_string = "the quick \"brown fox\" jumps over the \"lazy dog\"";
	std::cout << "[" << test_string << "]\n";
	auto result = util::SplitArgString(test_string, ' ');
	std::cout << "values:\n";
	for(auto a : result)
	{
		if(a.size())
			std::cout << a << "\n";
		else
			std::cout << "(empty string)\n";
	}
	if(result.size() != 7)
		std::cout << "X - expected arg count of 7, got " << result.size() << "\n";
}

void test_LogFmt()
{
	std::cout << "================================\n";
	std::cout << "LogFmt()\n";
	std::stringstream ss;
	Logger logger(&ss, &ss);

	std::string test_string = "Some string with %d substituted %s";
	std::cout << "original:\n" << test_string << "\n";
	logger.Log_fmt(always, test_string.c_str(), 2, "arguments");
	std::cout << "logged:\n" << ss.str() << "\n";
	ss.str("");

	test_string = "Some string with %d substituted %s including a literal %%";
	std::cout << "original:\n" << test_string << "\n";
	logger.Log_fmt(always, test_string.c_str(), 2, "things");
	std::cout << "logged:\n" << ss.str() << "\n";
	ss.str("");

	test_string = "string with %d ending %";
	std::cout << "original:\n" << test_string << "\n";
	logger.Log_fmt(always, test_string.c_str(), 1);
	std::cout << "logged:\n" << ss.str() << "\n";
	ss.str("");

	test_string = "string with %d ending %%";
	std::cout << "original:\n" << test_string << "\n";
	logger.Log_fmt(always, test_string.c_str(), 2);
	std::cout << "logged:\n" << ss.str() << "\n";
	ss.str("");

	test_string = "string with %d ending %%%";
	std::cout << "original:\n" << test_string << "\n";
	logger.Log_fmt(always, test_string.c_str(), 3);
	std::cout << "logged:\n" << ss.str() << "\n";
}

void test_Url()
{
	std::cout << "================================\n";
	std::cout << "Uri()\n";


	std::string url = "http://example.com";
	std::cout << url << "\n";
	try
	{
		upnp_live::util::url_t u{url};
		if(u.protocol != "http")
			std::cout << "X - Bad protocol: " << u.protocol << "\n";
		if(u.hostname != "example.com")
			std::cout << "X - Bad hostname: " << u.hostname << "\n";
		if(u.port != 0)
			std::cout << "X - Bad port: " << u.port << "\n";
		if(u.path != "/")
			std::cout << "X - Bad path: " << u.path << "\n";
	}
	catch(InvalidUri e)
	{
		std::cout << "X - Constructor threw exception\n";
	}


	url = "http://www.example.com/";
	std::cout << url << "\n";
	try
	{
		upnp_live::util::url_t u{url};
		if(u.protocol != "http")
			std::cout << "X - Bad protocol: " << u.protocol << "\n";
		if(u.hostname != "www.example.com")
			std::cout << "X - Bad hostname: " << u.hostname << "\n";
		if(u.port != 0)
			std::cout << "X - Bad port: " << u.port << "\n";
		if(u.path != "/")
			std::cout << "X - Bad path: " << u.path << "\n";
	}
	catch(InvalidUri e)
	{
		std::cout << "X - Constructor threw exception\n";
	}


	url = "http://www.example.com:80/";
	std::cout << url << "\n";
	try
	{
		upnp_live::util::url_t u{url};
		if(u.protocol != "http")
			std::cout << "X - Bad protocol: " << u.protocol << "\n";
		if(u.hostname != "www.example.com")
			std::cout << "X - Bad hostname: " << u.hostname << "\n";
		if(u.port != 80)
			std::cout << "X - Bad port: " << u.port << "\n";
		if(u.path != "/")
			std::cout << "X - Bad path: " << u.path << "\n";
	}
	catch(InvalidUri e)
	{
		std::cout << "X - Constructor threw exception\n";
	}


	url = "rtmp://example.com/some/path/here.xyz";
	std::cout << url << "\n";
	try
	{
		upnp_live::util::url_t u{url};
		if(u.protocol != "rtmp")
			std::cout << "X - Bad protocol: " << u.protocol << "\n";
		if(u.hostname != "example.com")
			std::cout << "X - Bad hostname: " << u.hostname << "\n";
		if(u.port != 0)
			std::cout << "X - Bad port: " << u.port << "\n";
		if(u.path != "/some/path/here.xyz")
			std::cout << "X - Bad path: " << u.path << "\n";
	}
	catch(InvalidUri e)
	{
		std::cout << "X - Constructor threw exception\n";
	}


	url = "xyzq://localhost:492";
	std::cout << url << "\n";
	try
	{
		upnp_live::util::url_t u{url};
		if(u.protocol != "xyzq")
			std::cout << "X - Bad protocol: " << u.protocol << "\n";
		if(u.hostname != "localhost")
			std::cout << "X - Bad hostname: " << u.hostname << "\n";
		if(u.port != 492)
			std::cout << "X - Bad port: " << u.port << "\n";
		if(u.path != "/")
			std::cout << "X - Bad path: " << u.path << "\n";
	}
	catch(InvalidUri e)
	{
		std::cout << "X - Constructor threw exception\n";
	}
}

int main(int argc, char* argv[])
{
	test_TrimString();
	test_TrimString_char();
	test_SplitString();
	test_SplitArgString();
	test_LogFmt();
	test_Url();
	std::cout << std::flush;
}
