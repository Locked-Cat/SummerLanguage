#pragma once

#include <iostream>
#include <string>
#include <memory>
#include <stdexcept>
#include <exception>

namespace summer_lang
{
	enum class token_categories
	{
		KEYWORD,
		IDENTIFIER,
		LITERAL_NUMBER,
		LITERAL_CHAR,
		OPERATOR,
		TYPE,
		END
	};

	enum class keyword_categories
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
		VAR,
	};

	enum class type_categories
	{
		CHAR,
		NUMBER,
		VOID
	};

	enum class operator_categories
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
		COLON,
		ASSIGN,
		ARROW,
		UNKNOWN
	};

	class token
	{
		int row_no_;
	public:
		token(int row_no)
			: row_no_(row_no)
		{
		}

		virtual token_categories get_type() const = 0;

		virtual ~token()
		{
		}

		int get_position() const
		{
			return row_no_;
		}
	};

	class keyword
		: public token
	{
		keyword_categories keyword_;
	public:
		using value_type = keyword_categories;

		keyword(keyword_categories type, int row_no)
			: token(row_no)
			, keyword_(type)
		{
		}

		virtual token_categories get_type() const override
		{
			return token_categories::KEYWORD;
		}

		value_type get_value() const
		{
			return keyword_;
		}
	};

	class type
		: public token
	{
		type_categories type_;
	public:
		using value_type = type_categories;

		type(type_categories type, int row_no)
			: token(row_no)
			, type_(type)
		{
		}

		virtual token_categories get_type() const override
		{
			return token_categories::TYPE;
		}

		value_type get_value() const
		{
			return type_;
		}
	};

	class identifier
		: public token
	{
		std::string name_;
	public:
		using value_type = std::string;

		identifier(const std::string & name, int row_no)
			: token(row_no)
			, name_(name)
		{
		}

		virtual token_categories get_type() const override
		{
			return token_categories::IDENTIFIER;
		}

		value_type get_value() const
		{
			return name_;
		}
	};

	class literal_number
		: public token
	{
		double value_;
	public:
		using value_type = double;

		literal_number(double value, int row_no)
			: token(row_no)
			, value_(value)
		{
		}

		virtual token_categories get_type() const override
		{
			return token_categories::LITERAL_NUMBER;
		}

		value_type get_value() const
		{
			return value_;
		}
	};

	class literal_char
		: public token
	{
		char value_;
	public:
		using value_type = char;

		literal_char(char value, int row_no)
			: token(row_no)
			, value_(value)
		{
		}

		virtual token_categories get_type() const override
		{
			return token_categories::LITERAL_CHAR;
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
		operator_categories op_type_;
		std::string op_;
	public:
		using value_type = operator_categories;
		using op_type = std::string;

		op(operator_categories op_type, const std::string & op, int row_no)
			: token(row_no)
			, op_type_(op_type)
			, op_(op)
		{
		}

		virtual token_categories get_type() const override
		{
			return token_categories::OPERATOR;
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

		end(int row_no)
			: token(row_no)
		{
		}

		virtual token_categories get_type() const override
		{
			return token_categories::END;
		}
	};

	template <typename Object, typename Token>
	typename Object::value_type get_value(const std::unique_ptr<Token> & tok)
	{
		auto ptr = dynamic_cast<Object *>(tok.get());
		if (ptr != nullptr)
			return ptr->get_value();
	}

	template <typename Token>
	op::op_type get_op(const std::unique_ptr<Token> & tok)
	{
		auto ptr = dynamic_cast<op *>(tok.get());
		if (ptr != nullptr)
			return ptr->get_op();
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
