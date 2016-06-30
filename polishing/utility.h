//(c) 2013-2016 by Authors
//This file is a part of Ragout program.
//Released under the BSD license (see LICENSE file)

#pragma once

#include <iterator>
#include <utility>
#include <iostream>
#include <ctime>
#include <sstream>
#include <vector>

#ifdef _DEBUG_LOG
#define DEBUG_PRINT(x) do {std::cerr << timestamp() << " " << x << std::endl;} \
					   while(0)
#else
#define DEBUG_PRINT(x)
#endif

#ifdef _LOG
#define LOG_PRINT(x) do {std::cerr << timestamp() << " " << x << std::endl;} \
					   while(0)
#else
#define LOG_PRINT(x)
#endif

#define WARNING_PRINT(x) do {std::cerr << timestamp() << " [WARNING] " << x << std::endl;} \
					   	 while(0)

#define ERROR_PRINT(x) do {std::cerr << timestamp() << " [ERROR] " << x << std::endl;} \
					   	 while(0)


inline std::string timestamp(const char* format = "[%H:%M:%S]")
{
	std::time_t t = std::time(0);
	char cstr[128];
	std::strftime(cstr, sizeof(cstr), format, std::localtime(&t));
	return cstr;
}


inline std::vector<std::string> 
splitString(const std::string &s, char delim) 
{
	std::vector<std::string> elems;
	std::stringstream ss(s);
	std::string item;
	while (std::getline(ss, item, delim)) elems.push_back(item);
	return elems;
}

class ProgressPercent
{
public:
	ProgressPercent(int finalCount = 0):
		_finalCount(finalCount), _curCount(0), _prevPercent(-1),
		_stopped(false)
	{}

	void setFinalCount(int finalCount) {_finalCount = finalCount;}
	void setValue(int value)
	{
		this->advance(value - _curCount);
	}
	void setDone()
	{
		this->setValue(_finalCount);
	}
	void advance(int step = 1)
	{
		if (_stopped) return;

		_curCount += step;
		int percent = 10UL * _curCount / _finalCount;
		if (percent > _prevPercent)
		{
			std::cerr << percent * 10 << "% ";
			_prevPercent = percent;
		}

		if (_prevPercent >= 10)
		{
			std::cerr << std::endl;
			_stopped = true;
		}
	}

private:
	int _finalCount;
	int _curCount;
	int _prevPercent;
	bool _stopped;
};