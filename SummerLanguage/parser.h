#pragma once

#include <llvm\ADT\STLExtras.h>
#include <llvm\IR\IRBuilder.h>
#include <llvm\IR\LLVMContext.h>
#include <llvm\IR\Module.h>
#include <llvm\IR\Verifier.h>
#include <llvm\Support\TargetSelect.h>
#include <llvm\Transforms\Scalar.h>
#include <llvm\IR\LegacyPassManager.h>
#include <llvm\Analysis\Passes.h>

#include <string>
#include <vector>
#include <iostream>
#include <fstream>
#include <map>
#include <memory>

#include "tokenizer.h"
#include "MCJIT_helper.h"
#include "error.h"

namespace summer_lang
{
	class ast
	{
	public:
		virtual ~ast()
		{
		}

		virtual llvm::Value * codegen() = 0;
	};

	class number_ast
		: public ast
	{
		double value_;
	public:
		number_ast(double value)
			: value_(value)
		{
		}

		virtual llvm::Value * codegen() override;
	};

	class variable_ast
		: public ast
	{
		std::string name_;
	public:
		variable_ast(const std::string & name)
			: name_(name)
		{
		}

		virtual llvm::Value * codegen() override;
	};

	class binary_expression_ast
		: public ast
	{
		char op_;
		std::unique_ptr<ast> left_, right_;
	public:
		binary_expression_ast(char op, std::unique_ptr<ast> left,  std::unique_ptr<ast> right)
			: op_(op)
			, left_(std::move(left))
			, right_(std::move(right))
		{
		}

		virtual llvm::Value * codegen()	override;
	};

	class call_expression_ast
		: public ast
	{
		std::string callee_;
		std::vector<std::unique_ptr<ast>> args_;
	public:
		call_expression_ast(const std::string & callee, std::vector<std::unique_ptr<ast>> args)
			: callee_(callee)
			, args_(std::move(args))
		{
		}

		virtual llvm::Value * codegen() override;
	};

	class if_expression_ast
		: public ast
	{
		std::unique_ptr<ast> cond_, then_part_, else_part_;
	public:
		if_expression_ast(std::unique_ptr<ast> cond, std::unique_ptr<ast> then_part, std::unique_ptr<ast> else_part)
			: cond_(std::move(cond))
			, then_part_(std::move(then_part))
			, else_part_(std::move(else_part))
		{
		}

		virtual llvm::Value * codegen() override;
	};

	class prototype_ast
	{
		std::string name_;
		std::vector<std::string> args_;
	public:
		prototype_ast(const std::string & name, std::vector<std::string> args)
			: name_(name)
			, args_(std::move(args))
		{
		}

		llvm::Function * codegen();
		const std::string & get_name() const
		{
			return name_;
		}
	};

	class function_ast
	{
		std::unique_ptr<prototype_ast> prototype_;
		std::unique_ptr<ast> body_;
	public:
		function_ast(std::unique_ptr<prototype_ast> prototype, std::unique_ptr<ast> body)
			: prototype_(std::move(prototype))
			, body_(std::move(body))
		{
		}

		llvm::Function * codegen();
	};

	static llvm::IRBuilder<> global_builder(llvm::getGlobalContext());
	static std::map<std::string, llvm::Value *> global_named_values;
	static std::unique_ptr<MCJIT_helper> JIT_helper;

	class parser
	{
		std::map<char, int> precedence_;
		token current_token_;
		std::unique_ptr<tokenizer> p_tokenizer_;

		int get_op_precedence_(char op);
		void get_next_token_();

		std::unique_ptr<ast> parse_number_();
		std::unique_ptr<ast> parse_parenthesis_();
		std::unique_ptr<ast> parse_identifier_();
		std::unique_ptr<ast> parse_primary_();
		std::unique_ptr<ast> parse_expression_();
		std::unique_ptr<ast> parse_bin_op_right_(int expr_precedence, std::unique_ptr<ast> left);
		std::unique_ptr<ast> parse_if_();

		std::unique_ptr<prototype_ast> parse_prototype_();
		std::unique_ptr<function_ast> parse_function_();
		std::unique_ptr<prototype_ast> parse_extern_();
		std::unique_ptr<function_ast> parse_top_level_expr_();

		void handle_extern();
		void handle_function();
		void handle_top_level_expr();

		using op = char;
		using identifier = std::string;
		using keyword = std::string;
		using number = double;
	public:
		parser(const parser &) = delete;
		parser & operator=(const parser &) = delete;

		parser();
		void parse(const std::string & file_name);
	};
}
