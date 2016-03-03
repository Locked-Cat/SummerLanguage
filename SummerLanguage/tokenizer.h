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
		VOID,
		NUMBER,
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
		USER_DEFINED
	};

	class token
	{
		int row_no_;
	public:
		token(int row_no)
			: row_no_(row_no)
		{
		}

		int get_position() const
		{
			return row_no_;
		}

		virtual token_categories get_type() const = 0;

		virtual ~token()
		{
		}
	};

	template <typename T>
	typename T::value_type get_value(const std::unique_ptr<token> & p_token)
	{
		auto ptr = dynamic_cast<T *>(p_token.get());
		return ptr->value_;
	}

	class keyword
		: public token
	{
		keyword_categories value_;
	public:
		using value_type = keyword_categories;

		keyword(keyword_categories categories, int row_no)
			: token(row_no)
			, value_(categories)
		{
		}

		virtual token_categories get_type() const override
		{
			return token_categories::KEYWORD;
		}

		friend value_type get_value<keyword>(const std::unique_ptr<token> & p_token);
	};

	class type
		: public token
	{
		type_categories value_;
	public:
		using value_type = type_categories;

		type(type_categories categories, int row_no)
			: token(row_no)
			, value_(categories)
		{
		}

		virtual token_categories get_type() const override
		{
			return token_categories::TYPE;
		}

		friend value_type get_value<type>(const std::unique_ptr<token> & p_token);
	};

	class identifier
		: public token
	{
		std::string value_;
	public:
		using value_type = std::string;

		identifier(const std::string & name, int row_no)
			: token(row_no)
			, value_(name)
		{
		}

		virtual token_categories get_type() const override
		{
			return token_categories::IDENTIFIER;
		}

		friend value_type get_value<identifier>(const std::unique_ptr<token> & p_token);
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

		friend value_type get_value<literal_number>(const std::unique_ptr<token> & p_token);
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

		friend value_type get_value<literal_char>(const std::unique_ptr<token> & p_token);
	};

	class op
		: public token
	{
		operator_categories value_;
		std::string op_name_;
	public:
		using value_type = operator_categories;

		op(operator_categories op_type, const std::string & op_name, int row_no)
			: token(row_no)
			, value_(op_type)
			, op_name_(op_name)
		{
		}

		virtual token_categories get_type() const override
		{
			return token_categories::OPERATOR;
		}

		std::string get_op_name() const
		{
			return op_name_;
		}

		friend value_type get_value<op>(const std::unique_ptr<token> & p_token);
	};

	std::string get_op_name(const std::unique_ptr<token> & tok);

	class end
		: public token
	{
	public:
		end(int row_no)
			: token(row_no)
		{
		}

		virtual token_categories get_type() const override
		{
			return token_categories::END;
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

		std::unique_ptr<token> get_token();
	};
}
