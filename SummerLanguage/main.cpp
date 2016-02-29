#include "parser.h"
#include <fstream>
#include <vector>

using namespace std;
using namespace summer_lang;

int main()
{												 
	parser global_parser;
	global_parser.parse("example.sl");
	return 0;
}