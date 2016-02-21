#include "tokenizer.h"
#include <fstream>
#include <vector>

using namespace std;
using namespace summer_lang;

int main()
{
	ifstream source_code("example.sl");
	tokenizer global_tokenizer(source_code);
	vector<token> tokens;

	auto current_token = global_tokenizer.get_token();
	while (current_token.get_type() != token_type::END)
	{
		tokens.push_back(current_token);
		current_token = global_tokenizer.get_token();
	}

	return 0;
}