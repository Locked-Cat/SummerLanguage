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
#include <llvm\IR\Instruction.h>

#include <string>
#include <vector>
#include <iostream>
#include <fstream>
#include <map>
#include <memory>
#include <utility>

#include "tokenizer.h"
#include "MCJIT_helper.h"
#include "error.h"
#include "lib.h"

namespace summer_lang
{
	class ast
	{
		int start_row_no_;
	public:
		ast(const ast &) = delete;
		ast & operator=(const ast &) = delete;

		ast(int start_row_no)
			: start_row_no_(start_row_no)
		{
		}

		virtual ~ast()
		{
		}

		virtual llvm::Value * codegen() = 0;

		int get_position() const
		{
			return start_row_no_;
		}
	};

	class number_ast
		: public ast
	{
		double value_;
	public:
		number_ast(double value, int start_row_no)
			: ast(start_row_no)
			, value_(value)
		{
		}

		virtual llvm::Value * codegen() override;
	};

	class string_ast
		: public ast
	{
		std::string value_;
	public:
		string_ast(const std::string & value, int start_row_no)
			: ast(start_row_no)
			, value_(value)
		{
		}

		virtual llvm::Value * codegen() override;
	};

	class variable_ast
		: public ast
	{
		std::string name_;
	public:
		variable_ast(const std::string & name, int start_row_no)
			: ast(start_row_no)
			, name_(name)
		{
		}

		std::string get_name() const
		{
			return name_;
		}

		virtual llvm::Value * codegen() override;
	};

	class var_ast
		: public ast
	{
		std::vector<std::tuple<std::string, llvm::Type *, std::unique_ptr<ast>>> vars_;
		std::unique_ptr<ast> body_;
	public:
		var_ast(decltype(vars_) vars, std::unique_ptr<ast> body, int start_row_no)
			: ast(start_row_no)
			, vars_(std::move(vars))
			, body_(std::move(body))
		{
		}

		virtual llvm::Value * codegen() override;
	};

	class binary_expression_ast
		: public ast
	{
		std::string op_name_;
		operator_categories op_type_;
		std::unique_ptr<ast> left_, right_;
	public:
		binary_expression_ast(std::string op_name, operator_categories op_type, std::unique_ptr<ast> left, std::unique_ptr<ast> right, int start_row_no)
			: ast(start_row_no)
			, op_name_(op_name)
			, op_type_(op_type)
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
		call_expression_ast(const std::string & callee, decltype(args_) args, int start_row_no)
			: ast(start_row_no)
			, callee_(callee)
			, args_(std::move(args))
		{
		}

		virtual llvm::Value * codegen() override;
	};

	class empty_ast :
		public ast
	{
	public:
		empty_ast(int start_row_no)
			: ast(start_row_no)
		{
		}

		virtual llvm::Value * codegen() override;
	};

	class block_ast
		: public ast
	{
		std::vector<std::unique_ptr<ast>> exprs_;
	public:
		block_ast(std::vector<std::unique_ptr<ast>> exprs, int start_row_no)
			: ast(start_row_no)
			, exprs_(std::move(exprs))
		{
		}

		virtual llvm::Value * codegen() override;
	};

	class return_ast
		: public ast
	{
		std::unique_ptr<ast> ret_;
	public:
		return_ast(std::unique_ptr<ast> ret, int start_row_no)
			: ast(start_row_no)
			, ret_(std::move(ret))
		{
		}

		virtual llvm::Value	* codegen() override;
	};

	class for_expression_ast
		: public ast
	{
		std::string var_name_;
		llvm::Type * var_type_;
		std::unique_ptr<ast> start_, end_, step_, body_;

	public:
		for_expression_ast(const std::string & var_name, llvm::Type * var_type, std::unique_ptr<ast> start, std::unique_ptr<ast> end, std::unique_ptr<ast> step, std::unique_ptr<ast> body, int start_row_no)
			: ast(start_row_no)
			, var_name_(var_name)
			, var_type_(var_type)
			, start_(std::move(start))
			, end_(std::move(end))
			, step_(std::move(step))
			, body_(std::move(body))
		{
		}

		virtual llvm::Value * codegen() override;
	};

	class if_expression_ast
		: public ast
	{
		std::unique_ptr<ast> cond_, then_part_, else_part_;
	public:
		if_expression_ast(std::unique_ptr<ast> cond, std::unique_ptr<ast> then_part, std::unique_ptr<ast> else_part, int start_row_no)
			: ast(start_row_no)
			, cond_(std::move(cond))
			, then_part_(std::move(then_part))
			, else_part_(std::move(else_part))
		{
		}

		virtual llvm::Value * codegen() override;
	};

	class unary_expression_ast
		: public ast
	{
		std::string op_name_;
		std::unique_ptr<ast> expr_;
	public:
		unary_expression_ast(std::string op_name, std::unique_ptr<ast> expr, int start_row_no)
			: ast(start_row_no)
			, op_name_(std::move(op_name))
			, expr_(std::move(expr))
		{
		}

		virtual llvm::Value * codegen() override;
	};

	class prototype_ast
	{
		std::string name_;
		std::vector<std::pair<std::string, llvm::Type *>> args_;
		llvm::Type * ret_type_;

		bool is_operator_;
		int precedence_;

		int start_row_no_;
	public:
		prototype_ast(const std::string & name, decltype(args_) args, llvm::Type * ret_type, bool is_operator, int precedence, int start_row_no)
			: name_(name)
			, args_(std::move(args))
			, ret_type_(ret_type)
			, is_operator_(is_operator)
			, precedence_(precedence)
			, start_row_no_(start_row_no)
		{
		}

		llvm::Function * codegen();

		const std::string get_name() const
		{
			return name_;
		}

		bool is_unary_op() const
		{
			return is_operator_ && args_.size() == 1;
		}

		bool is_binary_op() const
		{
			return is_operator_ && args_.size() == 2;
		}

		std::string get_operator_name() const
		{
			assert(is_unary_op() || is_binary_op());
			return name_.substr(name_.size() - 1);
		}

		int get_binary_op_precedence() const
		{
			return precedence_;
		}

		int get_position() const
		{
			return start_row_no_;
		}
	};

	class function_ast
	{
		std::unique_ptr<prototype_ast> prototype_;
		std::unique_ptr<ast> body_;

		int start_row_no_;
	public:
		function_ast(std::unique_ptr<prototype_ast> prototype, std::unique_ptr<ast> body, int start_row_no)
			: prototype_(std::move(prototype))
			, body_(std::move(body))
			, start_row_no_(start_row_no)
		{
		}

		llvm::Function * codegen();

		int get_position() const
		{
			return start_row_no_;
		}
	};

	static llvm::IRBuilder<> global_builder(llvm::getGlobalContext());
	static std::map<std::string, std::pair<llvm::AllocaInst *, llvm::Type *>> global_named_values;
	static std::unique_ptr<MCJIT_helper> global_JIT_helper;
	static std::map<std::string, int> global_op_precedence;

	static int get_op_precedence(const std::string & op_name);
	static void set_op_precedence(const std::string & op_name, int precedence);
	static llvm::AllocaInst * global_create_alloca(llvm::Function * function, const std::string & name, llvm::Type * type);

	class parser
	{
		std::unique_ptr<token> current_token_;
		std::unique_ptr<tokenizer> p_tokenizer_;

		void get_next_token_();

		std::unique_ptr<ast> parse_number_();
		std::unique_ptr<ast> parse_string_();
		std::unique_ptr<ast> parse_parenthesis_();
		std::unique_ptr<ast> parse_identifier_();
		std::unique_ptr<ast> parse_primary_();
		std::unique_ptr<ast> parse_expression_();
		std::unique_ptr<ast> parse_bin_op_right_(int expr_precedence, std::unique_ptr<ast> left, int start_row);
		std::unique_ptr<ast> parse_if_();
		std::unique_ptr<ast> parse_for_();
		std::unique_ptr<ast> parse_unary_();
		std::unique_ptr<ast> parse_var_();
		std::unique_ptr<ast> parse_block_();
		std::unique_ptr<ast> parse_return_();
		std::unique_ptr<ast> parse_empty_();

		std::unique_ptr<prototype_ast> parse_prototype_();
		std::unique_ptr<function_ast> parse_function_();
		std::unique_ptr<prototype_ast> parse_extern_();
		std::unique_ptr<function_ast> parse_top_level_expr_();

		void handle_extern();
		void handle_function();
		void handle_top_level_expr();
	public:
		parser(const parser &) = delete;
		parser & operator=(const parser &) = delete;

		parser();
		void parse(const std::string & file_name);
	};
}
