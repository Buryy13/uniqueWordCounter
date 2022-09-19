#pragma once

#include <iostream>
#include <sys/stat.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <cstdio>
#include <set>
#include <unordered_set>
#include <string>
#include <vector>
#include <algorithm>
#include <iterator>
#include <thread>
#include <unistd.h>
#include <future>

#include <fstream>
#include "threadsafe_queue.h"

static const int BUFFER_SIZE = sysconf(_SC_PAGE_SIZE);
class FileReader
{
public:
	struct Chunk
	{
		std::unordered_set<std::string> words;
		std::string firstWord;
		std::string lastWord;
		int id;
	};
	FileReader(const char* fName);
	~FileReader();
	bool initialize();
	void readFile();
	Chunk readChunk(int fd, int i);
	void fixCorruptedWords();
	void mergeWords();
	void printWordsSize() {
		std::cout << "\nSize of words: " << finalWords.size() << std::endl;
	}
private:
	const char* fileName;
	int fd;
	size_t fileSize = -1;
	int numOfChunks;
	std::set<Chunk, std::function<bool(const Chunk&, const Chunk&)>> chunks;
	std::unordered_set<std::string> finalWords;
	std::vector<std::future<Chunk>> futureChunks;
};
FileReader::FileReader(const char* fName) : chunks([this](const Chunk& lhs, const Chunk& rhs) {return lhs.id < rhs.id; })
{
	fileName = fName;
}
FileReader::~FileReader()
{
	close(fd);
}
bool FileReader::initialize()
{
	// open file for reading
	fd = open(fileName, O_RDONLY);
	if (fd == -1)
	{
		std::cout << "ERROR: could not open the file for reading!" << std::endl;
		return false;
	}
	// get file size
	struct stat status;
	if (fstat(fd, &status) == -1)
	{
		std::cout << "ERROR: could not fetch stat of the file!" << std::endl;
		return false;
	}
	fileSize = status.st_size;
	numOfChunks = fileSize / BUFFER_SIZE;
	if (fileSize % BUFFER_SIZE) numOfChunks++;
	return true;
}
void FileReader::readFile()
{
	for (int i = 0; i < numOfChunks; ++i)
	{
		//threads.emplace_back(&FileReader::readChunk, this, fd, i);
		futureChunks.emplace_back(std::async(&FileReader::readChunk, this, fd, i));
	}
}
FileReader::Chunk FileReader::readChunk(int fd, int i)
{
	void* mapped = mmap(nullptr, BUFFER_SIZE, PROT_READ, MAP_PRIVATE, fd, i * BUFFER_SIZE);
	if (mapped == MAP_FAILED)
	{
		std::cout << "ERROR: mapping failed!" << std::endl;
		// TODO: throw;
	}

	std::string strChunk(reinterpret_cast<char*>(mapped), BUFFER_SIZE);
	Chunk chunk;
	size_t firstSpaceIndx = strChunk.find_first_of(' ');
	size_t lastSpaceIndx = strChunk.find_last_of(' ');

	chunk.firstWord = strChunk.substr(0, firstSpaceIndx);
	chunk.lastWord = strChunk.substr(lastSpaceIndx + 1);
	// erase extra symbols from the very last word
	if (i == numOfChunks - 1)
	{
		auto itrNotAlpha = chunk.lastWord.find_first_not_of("abcdefghijklmnopqrstuvwxyz");
		if (itrNotAlpha != std::string::npos)
			chunk.lastWord.erase(itrNotAlpha);
	}
	chunk.id = i;
	// string of words formed from the chunk without first and last words (starting and ending with space)
	std::string stringWords = strChunk.substr(firstSpaceIndx, lastSpaceIndx - firstSpaceIndx + 1);
	while (stringWords.compare(" ") != 0)
	{
		stringWords.erase(0, 1); // rid off space
		std::string word = stringWords.substr(0, stringWords.find_first_of(' '));
		chunk.words.insert(word);
		stringWords.erase(0, stringWords.find_first_of(' ')); // rid off word
	}
	munmap(mapped, BUFFER_SIZE);
	return chunk;
}
void FileReader::fixCorruptedWords()
{
	//auto cmp = [](const Chunk& a, const Chunk& b){return a.id < b.id;};
	//std::set<Chunk, decltype(cmp)> sortedChunks(cmp);
	// sort chunks by id
	for (int i = 0; i < numOfChunks; ++i)
	{
		Chunk chunk = futureChunks[i].get();
		chunks.insert(chunk);
	}
	// adding first and last words
	auto it = chunks.begin();
	auto it2 = std::next(it, 1);
	finalWords.insert(it->firstWord);
	while (it2 != chunks.end())
	{
		std::string word = it->lastWord + it2->firstWord;
		finalWords.insert(word);
		it++; it2++;
	}
	finalWords.insert(it->lastWord);
}
void FileReader::mergeWords()
{
	//std::ofstream out("output.txt", std::ios::out);
// store chunk words into main set of words
//auto it = chunks.begin();
//auto it2 = std::next(it, 1);
	for (auto& chunk : chunks)
		for (auto word : chunk.words)
			finalWords.insert(std::move(word));
	//std::copy(finalWords.begin(), finalWords.end(), std::ostream_iterator<std::string>(out, "\n"));
}
