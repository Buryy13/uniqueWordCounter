#pragma once

#include <set>
#include <unordered_set>
#include <string>
#include <vector>
#include <future>

#include "threadsafe_queue.h"

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
	void printWordsSize();
private:
	const char* fileName;
	int fd;
	size_t fileSize = -1;
	size_t bufferSize;
	int numOfChunks;
	std::set<Chunk, std::function<bool(const Chunk&, const Chunk&)>> chunks;
	std::unordered_set<std::string> finalWords;
	std::vector<std::future<Chunk>> futureChunks;
};
