#pragma once
#include <iostream>
#include <llvm-c\Support.h>

namespace summer_lang
{
	class lib
	{
	public:
		static void import()
		{
			LLVMAddSymbol("print_number", &lib::print_number);
		}

		static void print_number(double d)
		{
			std::cout << d << std::flush;
		}
	};
}