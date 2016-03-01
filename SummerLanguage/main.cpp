#include "parser.h"
#include <fstream>
#include <vector>
#include <cstdlib>

using namespace std;
using namespace summer_lang;

int main(int argc, char * argv[])
{
	if (argc != 2)
	{
		cerr << "Illegal format of input" << endl;
		exit(EXIT_FAILURE);
	}

	parser global_parser;
	global_parser.parse(argv[1]);
	return 0;
}