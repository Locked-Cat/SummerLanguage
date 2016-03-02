#pragma once

#include <string>
#include <iostream>
#include <exception>
#include <sstream>

namespace summer_lang
{
	static std::string int2string(int value)
	{
		std::stringstream ss;
		ss << value;
		return ss.str();
	}

	class lexical_error
		: public std::exception
	{
	public:
		lexical_error(const std::string & message, int row_no)
			: std::exception((message + "At line: " + int2string(row_no)).c_str())
		{
		}
	};

	class syntax_error
		: public std::exception
	{
	public:
		syntax_error(const std::string & message, int row_no)
			: std::exception((message + "At line: " + int2string(row_no)).c_str())
		{
		}
	};
}