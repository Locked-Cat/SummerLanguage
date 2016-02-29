#include "parser.h"
#include <fstream>
#include <vector>

using namespace std;
using namespace summer_lang;

double print(double v)
{
	cout << char(v) << endl;
	return 0;
}

int main()
{

	parser global_parser;
	global_parser.parse("example.sl");
	return 0;
}