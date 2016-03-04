#include "tokenizer.h"
#include "error.h"
#include <cctype>
#include <cstdlib>

namespace summer_lang
{
	std::unique_ptr<token> tokenizer::get_token()
	{
		static int last_char = ' ';
		static int row_no = 1;

		if (last_char == EOF)
			return std::make_unique<end>(row_no);

		if (std::isspace(last_char))
		{
			while ((last_char = source_code_.get()) != EOF && std::isspace(last_char))
				if (last_char == '\r' || last_char == '\n')
					row_no++;
            return get_token();   
        }
            
        if(last_char == '#')
        {
            while((last_char = source_code_.get()) != EOF && (last_char != '\r' && last_char != '\n'));
			row_no++;
            return get_token();    
        }

		if (std::isalpha(last_char))
		{
			auto str = std::string();
			str += last_char;

			while ((last_char = source_code_.get()) != EOF && (isalnum(last_char) || last_char == '_'))
				str += last_char;

			if (str == "extern")
				return std::make_unique<keyword>(keyword_categories::EXTERN, row_no);

			if (str == "function")
				return std::make_unique<keyword>(keyword_categories::FUNCTION, row_no);

			if (str == "if")
				return std::make_unique<keyword>(keyword_categories::IF, row_no);

			if (str == "then")
				return std::make_unique<keyword>(keyword_categories::THEN, row_no);

			if (str == "else")
				return std::make_unique<keyword>(keyword_categories::ELSE, row_no);

			if (str == "for")
				return std::make_unique<keyword>(keyword_categories::FOR, row_no);

			if (str == "in")
				return std::make_unique<keyword>(keyword_categories::IN, row_no);

			if (str == "unary")
				return std::make_unique<keyword>(keyword_categories::UNARY, row_no);

			if (str == "binary")
				return std::make_unique<keyword>(keyword_categories::BINARY, row_no);

			if (str == "var")
				return std::make_unique<keyword>(keyword_categories::VAR, row_no);

			if (str == "begin")
				return std::make_unique<keyword>(keyword_categories::BEGIN, row_no);

			if (str == "end")
				return std::make_unique<keyword>(keyword_categories::END, row_no);

			if (str == "return")
				return std::make_unique<keyword>(keyword_categories::RETURN, row_no);

			if (str == "number")
				return std::make_unique<type>(type_categories::NUMBER, row_no);

			if (str == "void")
				return std::make_unique<type>(type_categories::VOID, row_no);

			if (str == "string")
				return std::make_unique<type>(type_categories::STRING, row_no);

			return std::make_unique<identifier>(str, row_no);
		}

		if (std::isdigit(last_char) || last_char == '.')
		{
			auto num_str = std::string();
			num_str += last_char;

			while ((last_char = source_code_.get()) != EOF && (std::isdigit(last_char) || last_char == '.'))
				num_str += last_char;

			auto num = std::strtod(num_str.c_str(), nullptr);
			return std::make_unique<literal_number>(num, row_no);
		}

		if (last_char == '\'')
		{
			auto ch_str = std::string();

			while ((last_char = source_code_.get()) != EOF && last_char != '\'')
				ch_str += last_char;

			if (last_char == '\'')
				last_char = source_code_.get();
			else
				throw lexical_error("Illegal format of character", row_no);

			
			char ch;
			if (ch_str.length() == 1)
			{
				ch = ch_str[0];
				return std::make_unique<literal_char>(ch, row_no);
			}
			else
			{
				if (ch_str.length() == 2)
				{
					if (ch_str[0] == '\\')
					{
						switch (ch_str[1])
						{
						case 'n':
							ch = '\n';
							break;
						case 'r':
							ch = '\r';
							break;
						case 't':
							ch = '\t';
							break;
						case '\\':
							ch = '\\';
							break;
						case '\'' :
							ch = '\'';
							break;
						default:
							throw lexical_error("Illegal format of character", row_no);
						}
						return std::make_unique<literal_char>(ch, row_no);
					}
				}
				throw lexical_error("Illegal format of character", row_no);
			}
		}

		if (last_char == '"')
		{
			auto str = std::string();

			while ((last_char = source_code_.get()) != EOF && last_char != '"')
			{
				auto ch = last_char;
				if (last_char == '\\')
				{
					last_char = source_code_.get();
					if(last_char == EOF)
						throw lexical_error("Illegal format of string", row_no);
					switch (last_char)
					{
					case 'n':
						ch = '\n';
						break;
					case 'r':
						ch = '\r';
						break;
					case 't':
						ch = '\t';
						break;
					case '\\':
						ch = '\\';
						break;
					case '\'':
						ch = '\'';
						break;
					default:
						throw lexical_error("Illegal format of character", row_no);
					}
				}
				str += ch;
			}

			if (last_char == '"')
				last_char = source_code_.get();
			else
				throw lexical_error("Illegal format of string", row_no);

			return std::make_unique<literal_string>(str, row_no);
		}

		operator_categories type;
		auto need_eat = true;
		auto operator_str = std::string();

		switch (last_char)
		{
		case ';':
			type = operator_categories::SEMI;
			operator_str += ';';
			break;
		case '<':
			type = operator_categories::LT;
			operator_str += '<';
			last_char = source_code_.get();
			if (last_char != '=' && last_char != '>')
				need_eat = false;
			else
			{
				if (last_char == '=')
				{
					operator_str += '=';
					type = operator_categories::LE;
				}
				else
				{
					operator_str += '>';
					type = operator_categories::NEQ;
				}
			}
			break;
		case '>':
			type = operator_categories::GT;
			operator_str += '>';
			last_char = source_code_.get();
			if (last_char != '=')
				need_eat = false;
			else
			{
				operator_str += '=';
				type = operator_categories::GE;
			}
			break;
		case '+':
			type = operator_categories::ADD;
			operator_str += '+';
			break;
		case ':':
			type = operator_categories::COLON;
			operator_str += ':';
			break;
		case '-':
			type = operator_categories::SUB;
			operator_str += '-';
			last_char = source_code_.get();
			if (last_char != '>')
				need_eat = false;
			else
			{
				operator_str += '>';
				type = operator_categories::ARROW;
			}
			break;
		case '*':
			type = operator_categories::MUL;
			operator_str += '*';
			break;
		case '/':
			type = operator_categories::DIV;
			operator_str += '/';
			break;
		case '(':
			type = operator_categories::LBRACKET;
			operator_str += '(';
			break;
		case ')':
			type = operator_categories::RBRACKET;
			operator_str += ')';
			break;
		case ',':
			type = operator_categories::COMM;
			operator_str += ',';
			break;
		case '=':
			type = operator_categories::ASSIGN;
			operator_str += '=';
			last_char = source_code_.get();
			if (last_char != '=')
				need_eat = false;
			else
			{
				operator_str += '=';
				type = operator_categories::EQ;
			}
			break;
		default:
			type = operator_categories::USER_DEFINED;
			operator_str += last_char;
			break;
		}

		if (need_eat)
			last_char = source_code_.get();

		return std::make_unique<op>(type, operator_str, row_no);
	}

	std::string get_op_name(const std::unique_ptr<token>& tok)
	{
		auto ptr = static_cast<op *>(tok.get());
		return ptr->get_op_name();
	}
}