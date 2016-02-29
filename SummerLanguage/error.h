#pragma once

#include <string>
#include <iostream>

#define ILLEAGL_NUMBER 0
#define ILLEAGL_CHAR 1

namespace summer_lang
{
	template <typename T = void *>
	T print_error(const std::string & message, std::ostream & os = std::cerr)
	{
		os << message << std::endl;
		return nullptr;
	}
}