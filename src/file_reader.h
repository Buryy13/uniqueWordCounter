#pragma once

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

#include <fstream>
#include "threadsafe_queue.h"

static const int BUFFER_SIZE = sysconf(_SC_PAGE_SIZE); 

class FileReader
{
public:
	FileReader(const char* fName);
	~FileReader();
	bool initialize();
	void readFile();
	void readChunk(int fd, int i);
	void fixCorruptedWords();
    void printWordsSize(){
		std::cout << "\nSize of words: " << words.size() << std::endl;
	}
	struct Chunk
	{
		std::set<std::string> words;
		std::string firstWord;
		std::string lastWord;
		int id;
	};
private:
	const char* fileName;
	int fd;
	size_t fileSize = -1;
	std::vector<std::thread> threads;
	int numOfChunks;
	ThreadSafeQueue<Chunk> chunks;
    std::set<std::string> words;
};
FileReader::FileReader(const char* fName)
{
	fileName = fName;
}
FileReader::~FileReader()
{
	for(auto& t : threads) t.join();
    close(fd);
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
void FileReader::readFile()
{
	for(int i = 0; i < numOfChunks; ++i)
	{
		threads.emplace_back(&FileReader::readChunk, this, fd, i);
	}
}
void FileReader::readChunk(int fd, int i)
{
	char* mapped = reinterpret_cast<char*>(mmap(nullptr, BUFFER_SIZE, PROT_READ, MAP_PRIVATE, fd, i*BUFFER_SIZE));
	std::string strChunk(mapped, BUFFER_SIZE); 
	if(mapped == MAP_FAILED) std::cout << "ERROR: mapping failed!" << std::endl;
	Chunk chunk;
    size_t firstSpaceIndx = strChunk.find_first_of(' ');
    size_t lastSpaceIndx = strChunk.find_last_of(' ');

    chunk.firstWord = strChunk.substr(0, firstSpaceIndx);
    chunk.lastWord = strChunk.substr(lastSpaceIndx+1);
	// erase '\n' from the very last word
    if(chunk.lastWord.find("\n") != std::string::npos)
    {
        chunk.lastWord.erase(chunk.lastWord.find_first_of('\n'));
    }
	chunk.id = i;

    std::string stringWords = strChunk.substr(firstSpaceIndx, lastSpaceIndx-firstSpaceIndx+1); // string of words formed from the chunk without first and last words (starting and ending with space)
    while(stringWords.compare(" ") != 0)
    {
        stringWords.erase(0, 1); // rid off space
        std::string word = stringWords.substr(0, stringWords.find_first_of(' '));
        chunk.words.insert(word);
        stringWords.erase(0, stringWords.find_first_of(' ')); // rid off word
    }
	chunks.push(chunk);
	munmap(mapped, BUFFER_SIZE); 
}
void FileReader::fixCorruptedWords()
{
    auto cmp = [](Chunk a, Chunk b){return a.id < b.id;};
    std::set<Chunk, decltype(cmp)> chunksToFix(cmp);
    std::ofstream out("output.txt", std::ios::out);
    // sort chunks by id
	for(int i = 0; i < numOfChunks; ++i)
	{
		Chunk chunk;
		chunks.try_pop(chunk);
        chunksToFix.insert(chunk); 
        // store chunk words into main set of words
        for(auto word : chunk.words) 
        {
            words.insert(std::move(word));
        }
	}
    // adding first and last words
    auto it = chunksToFix.begin();
    auto it2 = std::next(it, 1);
    words.insert(it->firstWord);
    while(it2 != chunksToFix.end())
    {
        std::string word = it->lastWord + it2->firstWord;
        words.insert(word);
        it++;
        it2++;
    }
    words.insert(it->lastWord);
	std::copy(words.begin(), words.end(), std::ostream_iterator<std::string>(out, "\n"));
}
