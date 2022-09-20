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
 
FileReader::FileReader(const char* fName) :
  fileName(fName), chunks([this](const Chunk& lhs, const Chunk& rhs) {return lhs.id < rhs.id; })
{
  bufferSize = sysconf(_SC_PAGE_SIZE);
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
  // TODO: optimize buffer size coresponding to filesize and concurrency
  numOfChunks = fileSize / bufferSize;
  if (fileSize % bufferSize) numOfChunks++;
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
    auto itrNotAlpha = chunk.lastWord.find_first_not_of("abcdefghijklmnopqrstuvwxyz");
    if (itrNotAlpha != std::string::npos)
      chunk.lastWord.erase(itrNotAlpha);
  }
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
  for (auto it = futureChunks.begin(); it != futureChunks.end(); it++)
    chunks.insert(it->get());

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
void FileReader::printWordsSize()
{
	std::cout << "There are " << finalWords.size() << " unique words." << std::endl;
}
