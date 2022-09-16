#include <iostream>
#include <sys/stat.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <cstdio>
#include <set>
#include <string>
#include <vector>
#include <algorithm>
#include <iterator>
#include <string_view>
#include <thread>
#include <unistd.h>

static const size_t MAX_LETTER_COUNT = 50;
static const int BUFFER_SIZE = sysconf(_SC_PAGE_SIZE); 

void nullTheWord(char* word, size_t bound)
{
	for(int i = 0; i <= bound; ++i)
	{
		word[i] = '\0';
	}
}
class FileReader
{
public:
	FileReader(const char* fName);
	~FileReader();
	bool initialize();
	struct Chunk
	{
		std::set<std::string> words;
		std::string firstWord;
		std::string lastWord;
		int inst;
	};
	void readChunks();
	void handleChunk(int fd, int i);
private:
	const char* fileName;
	int fd;
	size_t fileSize = -1;
	std::vector<std::thread> threads;
	int numOfChunks;
};
FileReader::FileReader(const char* fName)
{
	fileName = fName;
}
FileReader::~FileReader()
{
	for(auto& t : threads) t.join();
}
bool FileReader::initialize()
{
	// open file for reading
	fd = open(fileName, O_RDONLY);
	if(fd == -1)
	{
		std::cout << "ERROR: could not open the file for reading!" << std::endl;
		return false;
	}
	// get file size
	struct stat status;
	if(fstat(fd, &status) == -1)
	{
		std::cout << "ERROR: could not fetch stat of the file!" << std::endl;
		return false;
	}
	fileSize = status.st_size;
	numOfChunks = fileSize / BUFFER_SIZE; 
	if(fileSize % BUFFER_SIZE) numOfChunks++;
	return true;
}
void FileReader::readChunks()
{
	for(int i = 0; i < numOfChunks; ++i)
	{
		threads.emplace_back(&FileReader::handleChunk, this, fd, i);
	}
}
void FileReader::handleChunk(int fd, int i)
{
	char* mapped = reinterpret_cast<char*>(mmap(nullptr, BUFFER_SIZE, PROT_READ, MAP_PRIVATE, fd, i*BUFFER_SIZE));
	std::string_view myStr(mapped, BUFFER_SIZE); 
	std::cout << "string view number " << i << ": " << myStr << std::endl;
	munmap(mapped, BUFFER_SIZE); 
}
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
		std::cout << "ERROR: could not initialize data!" << std::endl;
		return -1;
	}
	fileReader.readChunks();
	
	//std::copy(words.begin(), words.end(), std::ostream_iterator<std::string>(std::cout, " "));

	return 0;
}
