#include <thread>

#include "file_reader.h"

int main(int argc, const char* argv[])
{
	if (argc != 2)
	{
		std::cout << "ERROR: bad arguments!" << std::endl;
		return -1;
	}
	FileReader fileReader(argv[1]);
	if(!fileReader.initialize())
	{
		std::cout << "ERROR: could not initialize the process!" << std::endl;
		return -1;
	}
	fileReader.readFile();
	std::this_thread::sleep_for(std::chrono::milliseconds(1000));
	fileReader.fixCorruptedWords();
    fileReader.printWordsSize();
	//std::copy(words.begin(), words.end(), std::ostream_iterator<std::string>(std::cout, " "));

	return 0;
}
