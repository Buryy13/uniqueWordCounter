# UniqueWordCounter
Count unique words in a huge file.

Description:
The program retrieves the file name as a first argument and reads the given file using mmap() function. The content of the file is fragmented into chunks. Each chunk stores first and last words(parts of the words), and a set of distinct unique words parsed from the body of the chunk. Some of the first/last words may be corrupted, therefore the fixCorruptedWords() function has been provided. The final stage is to gather all unique words into one storage. mergeWords() method does it recursively. It inserts words from one chunk into another until words from all chunks will be inserted into one chunk.
Multithreading is used in the program in the form of a pool of threads.

Platform:
Linux

Build system:
cmake

Requirements:
cmake version 3.10
g++

To run the program execute the following scripts from the root folder of the project:
run cmake -> ./cfg.sh
build the program -> ./build.sh
run the program -> ./run.sh

run.sh script runs the program using the default test data file - words.txt (test/words.txt)
