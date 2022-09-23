#pragma once

#include <future>
#include <list>
#include <string>
#include <unordered_set>
#include <vector>

#include "thread_pool.h"

class FileReader
{
public:
	struct Chunk
	{
		std::unordered_set<std::string> words;
		std::string firstWord;
		std::string lastWord;
		int id;

		bool operator<(const Chunk& rhs) const { return id < rhs.id; }
	};
	using ListItr = std::list<Chunk>::iterator;

	FileReader(const char* fName);
	~FileReader();

	bool initialize();
	void readFile();
	void fixCorruptedWords();
	void mergeWords();
	void printUniqueWordsNumber();

private:
	Chunk readChunk(int i);
	ListItr recursiveMerge(ListItr begin, ListItr end);
	template<typename T>
	std::list<T> parallelQuickSort(std::list<T> chunks);

	const char* fileName;
	int fd;
	size_t fileSize;
	size_t bufferSize;
	unsigned int numOfChunks;
	std::unordered_set<std::string> finalWords;
	std::vector<std::future<Chunk>> futureChunks;
	std::list<Chunk> chunks;
	ThreadPool threadPool;
};
