#pragma once
#include <iostream>
#include <cstring>
#include <llvm-c\Support.h>

namespace summer_lang
{
	class lib
	{
	public:
		static void import()
		{
			LLVMAddSymbol("print_number", &lib::print_number);
			LLVMAddSymbol("print_string", &lib::print_string);
			LLVMAddSymbol("str_cat", &lib::str_cat);
		}

		static void print_number(double d)
		{
			std::cout << d << std::flush;
		}

		static void print_string(char *s)
		{
			std::cout << s << std::flush;
		}

		static char * str_cat(char * left, char * right)
		{
			auto len = std::strlen(left) + std::strlen(right) + 1;
			auto new_str = new char[len];

			for (unsigned i = 0; i < std::strlen(left); ++i)
				new_str[i] = left[i];

			for (unsigned i = 0; i < std::strlen(right); ++i)
				new_str[i + strlen(left)] = right[i];

			new_str[len - 1] = '\0';
			return new_str;
		}
	};
}