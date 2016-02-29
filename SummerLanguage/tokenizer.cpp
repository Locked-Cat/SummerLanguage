#include "tokenizer.h"
#include "error.h"
#include <cctype>
#include <cstdlib>

namespace summer_lang
{
	summer_lang::token_type token::get_type() const
	{
		return type_;
	}

	token tokenizer::get_token()
	{
		static int last_char = ' ';

		if (last_char == EOF)
			return token(summer_lang::token(summer_lang::token_type::END));

		if (std::isspace(last_char))
		{
            while ((last_char = source_code_.get()) != EOF && std::isspace(last_char));
            return get_token();   
        }
            
        if(last_char == '#')
        {
             while((last_char = source_code_.get()) != EOF && (last_char != '\r' || last_char != '\n'));
            return get_token();    
        }

		if (std::isalpha(last_char))
		{
			auto str = std::string();
			str += last_char;

			while ((last_char = source_code_.get()) != EOF && (isalnum(last_char) || last_char == '_'))
				str += last_char;

			auto type = summer_lang::token_type::IDENTIFIER;
			if (str == "function"
				|| str == "extern"
				|| str == "if"
				|| str == "then"
				|| str == "else"
				|| str == "for"
				|| str == "in")
				type = summer_lang::token_type::KEYWORD;

			return summer_lang::token(type, str);
		}

		if (std::isdigit(last_char) || last_char == '.')
		{
			auto num_str = std::string();
			num_str += last_char;

			while ((last_char = source_code_.get()) != EOF && (std::isdigit(last_char) || last_char == '.'))
				num_str += last_char;

			auto num = std::strtod(num_str.c_str(), nullptr);
			return summer_lang::token(num);
		}

		if (last_char == '\'')
		{
			auto ch_str = std::string();

			while ((last_char = source_code_.get()) != EOF && last_char != '\'')
				ch_str += last_char;

			if (last_char == '\'')
				last_char = source_code_.get();
			else
				exit(ILLEAGL_CHAR);

			double ch;
			if (ch_str.length() == 1)
			{
				ch = static_cast<double>(ch_str[0]);
				return summer_lang::token(ch);
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
							ch = static_cast<double>('\n');
							return token(ch);
						default:
							break;
						}
					}
				}
			}

			exit(ILLEAGL_CHAR);
		}

		auto result = summer_lang::token(static_cast<char>(last_char));
		last_char = source_code_.get();
		return result;
	}
}