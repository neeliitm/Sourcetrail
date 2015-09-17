#include "TestStorage.h"

#include "utility/text/TextAccess.h"
#include "data/parser/cxx/CxxParser.h"
#include "TestFileManager.h"

TestStorage::TestStorage()
	: Storage("data/test.sqlite")
{
}

void TestStorage::parseCxxCode(std::string code)
{
	clear();

	TestFileManager fm;
	CxxParser parser(this, &fm);
	parser.parseFile(TextAccess::createFromString(code), Parser::Arguments());
}
