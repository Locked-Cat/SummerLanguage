#include "tokenizer.h"
#include "error.h"
#include <cctype>
#include <cstdlib>

namespace summer_lang
{
	std::unique_ptr<token> tokenizer::get_token()
	{
		static int last_char = ' ';

		if (last_char == EOF)
			return std::make_unique<end>();

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

			if (str == "extern")
				return std::make_unique<keyword>(keyword_type::EXTERN);

			if (str == "function")
				return std::make_unique<keyword>(keyword_type::FUNCTION);

			if (str == "if")
				return std::make_unique<keyword>(keyword_type::IF);

			if (str == "then")
				return std::make_unique<keyword>(keyword_type::THEN);

			if (str == "for")
				return std::make_unique<keyword>(keyword_type::FOR);

			if (str == "in")
				return std::make_unique<keyword>(keyword_type::IN);

			if (str == "unary")
				return std::make_unique<keyword>(keyword_type::UNARY);

			if (str == "binary")
				return std::make_unique<keyword>(keyword_type::BINARY);

			return std::make_unique<identifier>(str);
		}

		if (std::isdigit(last_char) || last_char == '.')
		{
			auto num_str = std::string();
			num_str += last_char;

			while ((last_char = source_code_.get()) != EOF && (std::isdigit(last_char) || last_char == '.'))
				num_str += last_char;

			auto num = std::strtod(num_str.c_str(), nullptr);
			return std::make_unique<number>(num);
		}

		if (last_char == '\'')
		{
			auto ch_str = std::string();

			while ((last_char = source_code_.get()) != EOF && last_char != '\'')
				ch_str += last_char;

			if (last_char == '\'')
				last_char = source_code_.get();
			else
				return std::make_unique<unknown>();

			double ch;
			if (ch_str.length() == 1)
			{
				ch = static_cast<double>(ch_str[0]);
				return std::make_unique<number>(ch);
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
							return std::make_unique<number>(ch);
						default:
							break;
						}
					}
				}
				return std::make_unique<unknown>();
			}
		}

		auto result = std::make_unique<op>(last_char);
		last_char = source_code_.get();
		return std::move(result);
	}
}