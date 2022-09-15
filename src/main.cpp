#include <iostream>
#include <sys/stat.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <cstdio>

int main(int argc, const char* argv[])
{
	struct stat status;
	size_t size = -1;
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
	const char* mapped = nullptr;
	mapped = reinterpret_cast<char*>(mmap(mapped, portion, PROT_READ, MAP_PRIVATE, fd, 0));
	std::cout << "First part: " << std::endl;
	for (int i = 0; i < size; ++i)
	{
		char c = mapped[i];
		putchar(c);
	}
	std::cout << "Second part" << std::endl;

	mapped = reinterpret_cast<char*>(mmap(mapped, size-portion, PROT_READ, MAP_PRIVATE, fd, portion));
	for (int i = 0; i < size; ++i)
	{
		char c = mapped[i];
		putchar(c);
	}
	return 0;
}