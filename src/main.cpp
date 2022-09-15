#include <iostream>
#include <sys/stat.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <cstdio>
#include <set>
#include <string>
#include <algorithm>
#include <iterator>
#include <string_view>

static const size_t MAX_LETTER_COUNT = 50;

void nullTheWord(char* word, size_t bound)
{
	for(int i = 0; i <= bound; ++i)
	{
		word[i] = '\0';
	}
}

int main(int argc, const char* argv[])
{
	struct stat status;
	size_t size = -1;
	std::set<std::string> words;
	char* word = new char[MAX_LETTER_COUNT];
	char* pc=word;

	if (argc != 2)
	{
		std::cout << "error: bad arguments!" << std::endl;
		return -1;
	}

	const char* file_name = argv[1];
	int fd = open(file_name, O_RDONLY);

	// get file size
	fstat(fd, &status);
	size = status.st_size;

	size_t portion = size / 2;
	char* mapped = nullptr;
	mapped = reinterpret_cast<char*>(mmap(mapped, 10, PROT_READ, MAP_PRIVATE, fd, 0));
	//std::string_view myStr(mapped, size); 
	//std::cout << "string view: \n" << myStr << std::endl;
	std::cout << "First part: " << std::endl;

	for (int i = 0; i < 10; ++i)
	{
		char c = mapped[i];
		putchar(c);
		/*if(c != ' ')
		{
			*pc++ = c;
		}
		else
		{
			words.insert({word});
			nullTheWord(word, pc-word);
			pc = word;
		}*/
	}
	//words.insert({word});
	std::cout << "\nSecond part" << std::endl;
	char * new_mapped = reinterpret_cast<char*>(mmap(mapped, 10, PROT_READ, MAP_PRIVATE, fd, 1));
	for (int i = 0; i < 10; ++i)
	{
		char c = new_mapped[i];
		putchar(c);
	}
	std::cout << std::endl;
	std::copy(words.begin(), words.end(), std::ostream_iterator<std::string>(std::cout, " "));
	std::cout << "\nSize of set: " << words.size() << std::endl;
	return 0;
}
