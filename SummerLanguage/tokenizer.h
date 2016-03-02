#pragma once

#include <iostream>
#include <string>
#include <memory>
#include <stdexcept>
#include <exception>

namespace summer_lang
{
	enum class token_type
	{
		KEYWORD,
		IDENTIFIER,
		NUMBER,
		OPERATOR,
		END,
		UNKNOWN
	};

	enum class keyword_type
	{
		EXTERN,
		FUNCTION,
		IF,
		THEN,
		ELSE,
		FOR,
		IN,
		UNARY,
		BINARY,
		VAR
	};

	enum class operator_type
	{
		LT,
		LE,
		GT,
		GE,
		EQ,
		NEQ,
		ADD,
		SUB,
		MUL,
		DIV,
		LBRACKET,
		RBRACKET,
		COMM,
		ASSIGN,
		UNKNOWN
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
		operator_type op_type_;
		std::string op_;
	public:
		using value_type = operator_type;
		using op_type = std::string;

		op(operator_type op_type, const std::string & op)
			: op_type_(op_type)
			, op_(op)
		{
		}

		virtual token_type get_type() const override
		{
			return token_type::OPERATOR;
		}

		value_type get_value() const
		{
			return op_type_;
		}

		op_type get_op() const
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
	typename Object::value_type get_value(const std::unique_ptr<Token> & tok)
	{
		auto ptr = dynamic_cast<Object *>(tok.get());
		if (ptr != nullptr)
			return ptr->get_value();
		throw token_type_mismatch(typeid(Token).name(), typeid(Object).name());
	}

	template <typename Token, typename Object = op>
	typename Object::op_type get_op(const std::unique_ptr<Token> & tok)
	{
		auto ptr = dynamic_cast<op *>(tok.get());
		if (ptr != nullptr)
			return ptr->get_op();
		throw token_type_mismatch(typeid(Token).name(), typeid(op).name());
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
