#pragma once

#include <iostream>
#include <string>
#include <memory>
#include <stdexcept>
#include <exception>

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

	enum keyword_type
	{
		EXTERN,
		FUNCTION,
		IF,
		THEN,
		ELSE,
		FOR,
		IN,
		UNARY,
		BINARY
	};


	class token
	{
	public:
		virtual token_type get_type() const = 0;
		virtual ~token()
		{
		}
	};

	class keyword
		: public token
	{
		keyword_type keyword_;
	public:
		using value_type = keyword_type;

		keyword(keyword_type type)
			: keyword_(type)
		{
		}

		virtual token_type get_type() const override
		{
			return token_type::KEYWORD;
		}

		value_type get_value() const
		{
			return keyword_;
		}
	};

	class identifier
		: public token
	{
	private:
		std::string name_;
	public:
		using value_type = std::string;

		identifier(const std::string & name)
			: name_(name)
		{
		}

		virtual token_type get_type() const override
		{
			return token_type::IDENTIFIER;
		}

		value_type get_value() const
		{
			return name_;
		}
	};

	class number
		: public token
	{
	private:
		double value_;
	public:
		using value_type = double;

		number(double value)
			: value_(value)
		{
		}

		virtual token_type get_type() const override
		{
			return token_type::NUMBER;
		}

		value_type get_value() const
		{
			return value_;
		}
	};

	class op
		: public token
	{
	private:
		char op_;
	public:
		using value_type = char;

		op(char op_name)
			: op_(op_name)
		{
		}

		virtual token_type get_type() const override
		{
			return token_type::OPERATOR;
		}

		value_type get_value() const
		{
			return op_;
		}
	};

	class end
		: public token
	{
	public:
		using value_type = void;

		end()
		{
		}

		virtual token_type get_type() const override
		{
			return token_type::END;
		}
	};

	class unknown
		: public token
	{
	public:
		unknown()
		{
		}

		virtual token_type get_type() const override
		{
			return token_type::UNKNOWN;
		}
	};

	class token_type_mismatch
		: public std::exception
	{
	public:
		token_type_mismatch(const std::string & src, const std::string & des)
			: std::exception((src + " to " + des).c_str())
		{
		}
	};

	template <typename Object, typename Token>
	typename Object::value_type get_value(std::unique_ptr<Token> & tok)
	{
		auto ptr = dynamic_cast<Object *>(tok.get());
		if (ptr != nullptr)
			return ptr->get_value();
		throw token_type_mismatch(typeid(Token).name(), typeid(Object).name());
	}

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

		std::unique_ptr<token> get_token();
	};
}
