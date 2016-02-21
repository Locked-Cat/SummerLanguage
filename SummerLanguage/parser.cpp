#include "parser.h"
#include <cassert>

namespace summer_lang
{
	void parser::parse(const std::string & file_name)
	{
	}

	ast * parser::error_(const std::string & message)
	{
		std::cerr << message << std::endl;
		return nullptr;
	}

	void parser::get_next_token_()
	{
		current_token_ = p_tokenizer_->get_token();
	}

	ast * parser::parse_number_()
	{
		auto result = new number_ast(current_token_.get_value<number>());
		get_next_token_();
		return result;
	}

	ast * parser::parse_parenthesis_()
	{
		get_next_token_();

		auto result = parse_expression_();
		if (!result)
			return result;

		if (current_token_.get_value<op>() != ')')
			return error_("Expected a ')'");
		get_next_token_();
		return result;
	}

	ast * parser::parse_identifier_()
	{
		auto name = current_token_.get_value<identifier>();

		get_next_token_();
		if (current_token_.get_value<op>() != '(')
			return new variable_ast(name);

		get_next_token_();
		std::vector<ast *> args;
		while (true)
		{
			auto arg = parse_expression_();
			if (!arg)
				return nullptr;
			args.push_back(arg);

			if (current_token_.get_value<op>() == ')')
			{
				get_next_token_();
				return new call_expression_ast(name, args);
			}

			if (current_token_.get_value<op>() != ',')
				return error_("Expected a ')' or ',' in argument list");
			get_next_token_();
		}
	}
}