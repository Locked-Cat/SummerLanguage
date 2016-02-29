#pragma once

#include <llvm\ExecutionEngine\ExecutionEngine.h>
#include <llvm\ExecutionEngine\MCJIT.h>
#include <llvm\Analysis\Passes.h>
#include <llvm\ExecutionEngine\SectionMemoryManager.h>
#include <llvm\IR\DataLayout.h>
#include <llvm\IR\LLVMContext.h>
#include <llvm\IR\Module.h>
#include <llvm\IR\LegacyPassManager.h>
#include <llvm\IR\Verifier.h>
#include <vector>
#include <memory>

#include "error.h"

namespace summer_lang
{
	class MCJIT_helper
	{
		llvm::LLVMContext & context_;
		llvm::Module * open_module_;
		std::vector<llvm::Module *> modules_;
		std::vector<llvm::ExecutionEngine *> engines_;
	public:
		MCJIT_helper(llvm::LLVMContext & context)
			: context_(context)
			, open_module_(nullptr)
		{
		}
		~MCJIT_helper();

		llvm::Function * get_function(const std::string & name);
		llvm::Module * get_module_for_new_function();
		void * get_pointer_to_function(llvm::Function * function);
		void * get_symbol_address(const std::string & name);

		static std::string generate_function_name(const std::string & name);
	};
}