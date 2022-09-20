#include "file_reader.h"

#include <iostream>

int main(int argc, const char* argv[])
{
	if (argc != 2)
	{
		std::cout << "ERROR: bad arguments!" << std::endl;
		return -1;
	}
	FileReader fileReader(argv[1]);
	if (!fileReader.initialize())
	{
		std::cout << "ERROR: could not initialize the process!" << std::endl;
		return -1;
	}
	fileReader.readFile();
	fileReader.fixCorruptedWords();
	fileReader.mergeWords();
	fileReader.printWordsSize();

	return 0;
}
