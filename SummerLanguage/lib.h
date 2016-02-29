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
			LLVMAddSymbol("putd", &lib::putd);
			LLVMAddSymbol("putc", &lib::putc);
		}

		static double putd(double d)
		{
			std::cout << d;
			std::cout.flush();
			return 0;
		}

		static double putc(double c)
		{
			std::cout << static_cast<char>(c);
			std::cout.flush();
			return 0;
		}
	};
}