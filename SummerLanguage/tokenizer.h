#pragma once

#include <iostream>
#include <string>
#include <memory>

namespace summer_lang
{
	enum token_type
	{
		KEYWORD,
		IDENTIFIER,
		NUMBER,
		OPERATOR,
		END,
		UNKNOWN
	};

	class token
	{
	private:
		token_type type_;

		std::string str_ = "";
		double num_ = 0;
		char op_ = '\0';
	public:
		token(token_type type)
			: type_(type)
		{
		}

		token(token_type type, const std::string & str)
			: type_(type)
			, str_(str)
		{
		}

		token(double num)
			: type_(token_type::NUMBER)
			, num_(num)
		{
		}

		token(char op)
			: type_(token_type::OPERATOR)
			, op_(op)
		{
		}

		token()
			: type_(token_type::UNKNOWN)
		{
		}

		token_type get_type() const;
		template <typename T>
		T get_value() const;

		template<>
		std::string get_value<std::string>() const
		{
			return str_;
		}

		template<>
		double get_value<double>() const
		{
			return num_;
		}

		template <>
		char get_value<char>() const
		{
			return op_;
		}
	};

	class tokenizer
	{
		std::istream & source_code_;
	public:
		tokenizer(const tokenizer &) = delete;
		tokenizer & operator=(const tokenizer &) = delete;

		tokenizer(std::istream & source_code)
			: source_code_(source_code)
		{
		}

		token get_token();
	};
}
