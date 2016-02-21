#pragma once

#include <string>
#include <vector>
#include <iostream>
#include <fstream>
#include <map>
#include "tokenizer.h"

namespace summer_lang
{
	class ast
	{
	public:
		virtual ~ast()
		{
		}
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
	};

	class binary_expression_ast
		: public ast
	{
		char op_;
		ast * left_, *right_;
	public:
		binary_expression_ast(char op, ast * left, ast * right)
			: op_(op)
			, left_(left)
			, right_(right)
		{
		}
	};

	class call_expression_ast
		: public ast
	{
		std::string callee_;
		std::vector<ast *> args_;
	public:
		call_expression_ast(const std::string & callee, std::vector<ast *> & args)
			: callee_(callee)
			, args_(args_)
		{
		}
	};

	class prototype_ast
		: public ast
	{
		std::string name_;
		std::vector<std::string> args_;
	public:
		prototype_ast(const std::string & name, std::vector<std::string> & args)
			: name_(name)
			, args_(args)
		{
		}
	};

	class function_ast
		: public ast
	{
		prototype_ast * prototype_;
		ast * body_;
	public:
		function_ast(prototype_ast * prototype, ast * body)
			: prototype_(prototype)
			, body_(body)
		{
		}
	};

	class parser
	{
		std::map<char, int> precedence_;
		token current_token_;
		tokenizer * p_tokenizer_;

		ast * error_(const std::string & message);

		int get_op_precedence_();
		void get_next_token_();

		ast * parse_number_();
		ast * parse_parenthesis_();
		ast * parse_identifier_();
		ast * parse_primary_();
		ast * parse_expression_();
		ast * parse_bin_op_right_();

		prototype_ast * parse_prototype();
		function_ast * parse_function();
		prototype_ast * parse_extern();
		function_ast * parse_top_level_expr();

		using op = char;
		using identifier = std::string;
		using number = double;
	public:
		parser()
		{
		}

		void parse(const std::string & file_name);
	};
}
