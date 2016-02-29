#include "tokenizer.h"
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
				|| str == "else")
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

		auto result = summer_lang::token(static_cast<char>(last_char));
		last_char = source_code_.get();
		return result;
	}
}