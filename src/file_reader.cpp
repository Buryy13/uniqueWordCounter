#include "file_reader.h"

#include <iostream>
#include <algorithm>
#include <thread>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <cstdio>
#include <fstream>
#include <functional>

static const std::string ALPHABET{"abcdefghijklmnopqrstuvwxyz"};

FileReader::FileReader(const char* fName) :
  fileName(fName)
{
	fd = -1;
	fileSize = -1;
	bufferSize = -1;
	numOfChunks = 0;
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
  // TODO: optimize buffer size coresponding to file size and concurrency
  bufferSize = sysconf(_SC_PAGE_SIZE);
  numOfChunks = fileSize / bufferSize;
  if (fileSize % bufferSize) numOfChunks++;
	futureChunks.reserve(numOfChunks);
  return true;
}
void FileReader::readFile()
{
  for (int i = 0; i < numOfChunks; ++i)
  {
    //futureChunks.emplace_back(std::async(&FileReader::readChunk, this, i));
		futureChunks.push_back(threadPool.submit(std::bind(&FileReader::readChunk, this, i)));
  }
}
FileReader::Chunk FileReader::readChunk(int i)
{
  void* mapped = mmap(nullptr, bufferSize, PROT_READ, MAP_PRIVATE, fd, i * bufferSize);
  if (mapped == MAP_FAILED)
  {
    std::cout << "ERROR: mapping failed!" << std::endl;
    // TODO: throw;
  }

  std::string strChunk(reinterpret_cast<char*>(mapped), bufferSize);
  Chunk chunk;
  chunk.id = i;

  size_t firstSpaceIndx = strChunk.find_first_of(' ');
  size_t lastSpaceIndx = strChunk.find_last_of(' ');

  chunk.firstWord = strChunk.substr(0, firstSpaceIndx);
  chunk.lastWord = strChunk.substr(lastSpaceIndx + 1);
  // erase extra symbols from the very last word
  if (i == numOfChunks - 1)
  {
    auto itrNotAlpha = chunk.lastWord.find_first_not_of(ALPHABET);
    if (itrNotAlpha != std::string::npos)
      chunk.lastWord.erase(itrNotAlpha);
  }
	// parsing the chunk of words
  // string of words formed from the chunk without first and last words (starting and ending with space)
  std::string stringWords = strChunk.substr(firstSpaceIndx, lastSpaceIndx - firstSpaceIndx + 1);
  while (stringWords.compare(" ") != 0)
  {
    stringWords.erase(0, 1); // rid off space
    std::string word = stringWords.substr(0, stringWords.find_first_of(' '));
    chunk.words.insert(word);
    stringWords.erase(0, stringWords.find_first_of(' ')); // rid off word
  }
  munmap(mapped, bufferSize);
  return chunk;
}
void FileReader::fixCorruptedWords()
{
  // sort chunks by id
	for(auto& fu : futureChunks)
    chunks.push_back(fu.get());

	// sort list of chunks by id
	//chunks = parallelQuickSort(chunks);	// much slower than std::sort() of STL
	chunks.sort();

  // adding first and last words
  auto it = chunks.begin();
  auto it2 = std::next(it, 1);
  finalWords.insert(it->firstWord);
  while (it2 != chunks.end())
  {
    std::string word = it->lastWord + it2->firstWord;
    finalWords.insert(word);
    it++;
		it2++;
  }
  finalWords.insert(it->lastWord);
}
template<typename T>
std::list<T> FileReader::parallelQuickSort(std::list<T> chunks)
{
	if (chunks.empty()) return chunks;

	std::list<T> result;
	result.splice(result.begin(), chunks, chunks.begin()); // delete pivot from input

	const T& pivot = *result.begin();

	auto divide_point = std::partition(chunks.begin(), chunks.end(), [&pivot](const T& val) { return val < pivot; });

	std::list<T> lower;
	lower.splice(lower.begin(), chunks, chunks.begin(), divide_point);
	// lower for thread pool
	std::future<std::list<T>> lowerFuture = 
		threadPool.submit(std::bind(&FileReader::parallelQuickSort<T>, this, std::move(lower)));
	// higher for current thread
	auto new_higher(parallelQuickSort(std::move(chunks)));
	result.splice(result.end(), new_higher);
	while(lowerFuture.wait_for(std::chrono::seconds(0)) == std::future_status::timeout)
		threadPool.runPendingTask();
	result.splice(result.begin(), lowerFuture.get());

	return result;
}
void FileReader::mergeWords()
{
  // store chunk words into main set of words
  /*
	for (auto& chunk : chunks)
    for (auto&& word : chunk.words)
      finalWords.insert(std::move(word));
  */
	// merge all words into first chunk and return its iterator
	FileReader::ListItr it = recursiveMerge(chunks.begin(), chunks.end());
	finalWords.insert(it->words.begin(), it->words.end());
}
FileReader::ListItr FileReader::recursiveMerge(ListItr begin, ListItr end)
{
	if(std::distance(begin, end) == 1) return begin;
	auto dividePoint = std::next(begin, std::distance(begin, end)/2);

	std::future<ListItr> lowerFuture = 
		//std::async(&FileReader::recursiveMerge, this, begin, dividePoint);
		threadPool.submit(std::bind(&FileReader::recursiveMerge, this, begin, dividePoint));
	ListItr it2 = recursiveMerge(dividePoint, end);
	while(lowerFuture.wait_for(std::chrono::seconds(0)) == std::future_status::timeout)
		threadPool.runPendingTask();
	ListItr it1 = lowerFuture.get();

	for(auto word : it2->words)
		it1->words.insert(word);
	return it1;
}
void FileReader::printUniqueWordsNumber()
{
	std::cout << "There are " << finalWords.size() << " unique words." << std::endl;
}
