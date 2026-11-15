/*
 * main.cpp
 *
 *  Created on: 5 ott 2026
 *      Author: feder
 */
// Project
// C++
#include <iostream>
#include <vector>
#include <cassert>
#include <cstring>
#include <fstream>
#include <filesystem>
// C

struct parameters_t
{
	enum {
		NONE,
		UNIT_TESTS,
		FILE
	} mode = NONE;
	std::string input;
	uint32_t start = 0;
	uint32_t length = 0;
	bool verbose = false;
} parameters;

uint32_t crc32(const std::vector<uint8_t> m)
{
	const uint32_t poly = 0x04c11db7;

	uint32_t o = 0xffffffff;

	std::cout << std::hex;
	for(uint32_t i = 0; i < m.size(); i++)
	{
		if(parameters.verbose)
		{
			if(i > 0 && i % 0x10 == 0)
				std::cout << '\n';
			if(m[i] < 0x10)
				std::cout << '0';
			std::cout << (uint32_t)m[i] << ' ';
		}

		for(int n = 7; n >= 0; n--)
		{
			uint32_t next = ((m[i] >> n) & 1);
			bool last = ((o >> 31) & 1) ^ next;
			o <<= 1;

			if(last)
			{
				o ^= poly;
			}
		}
	}

	if(parameters.verbose)
	{
		std::cout << '\n' << std::dec << m.size() << std::endl;
	}

	return o;
}

uint32_t crc32_file(const std::string filename, const size_t start, size_t length)
{
	if(length == 0)
	{
		length = std::filesystem::file_size(filename) - start;
	}

	// Preallocate memory
	std::vector<uint8_t> m;
	m.resize(length);

	// Load file contents in buffer
	std::fstream fstrm(filename, std::ios::binary | std::ios::in);
	fstrm.read((char*) m.data(), start);
	fstrm.read((char*) m.data(), length);
	if (!fstrm)
		std::cout << "error: only " << fstrm.gcount() << " could be read\n";
	fstrm.close();

	// Compute crc
	uint32_t crc = crc32(m);

	return crc;
}

#ifdef DEBUG
void run_tests()
{
	// 1 byte tests
	assert(crc32({0x00}) == 0x4e08bfb4);
	assert(crc32({0x0a}) == 0x61826962);
	assert(crc32({0xa0}) == 0xbf27e9ba);

	// 200 bytes tests
	assert(crc32_file("assets/test200_01", 0, 0) == 0x69e25580);
	assert(crc32_file("assets/test200_02", 0, 0) == 0x6d454f9a);

	std::cout << "All tests passed." << std::endl;
}
#endif

int main(int argc, const char* argv[])
{
	bool quit = false;

	for(int i = 1; i < argc; i++)
	{
		if(strcmp(argv[i], "-i") == 0)
		{
			parameters.input = argv[++i];
			parameters.mode = parameters_t::FILE;
		}
		else if(strcmp(argv[i], "-l") == 0 || strcmp(argv[i], "--length") == 0)
		{
			parameters.length = std::atoi(argv[++i]);
		}
		else if(strcmp(argv[i], "-s") == 0 || strcmp(argv[i], "--start") == 0)
		{
			parameters.start = std::atoi(argv[++i]);
		}
		else if(strcmp(argv[i], "-u") == 0 || strcmp(argv[i], "--unit") == 0)
		{
			parameters.mode = parameters_t::UNIT_TESTS;
		}
		else if(strcmp(argv[i], "--self") == 0)
		{
			parameters.input = argv[0];
			parameters.mode = parameters_t::FILE;
		}
		else if(strcmp(argv[i], "-v") == 0)
		{
			parameters.verbose = true;
		}
		else if(strcmp(argv[i], "-h") == 0 || strcmp(argv[i], "--help") == 0)
		{
			// Help has higher priority, print then stop
			std::cout << "Execute MPEG2 CRC algorithm on a given file. Compatible with RP2040 bootrom.\n";
			std::cout << "Usage 1: crc32 [-l length] [-s start] [-v] -i input-file\n";
			std::cout << "Usage 2: crc32 --self\n";
#ifdef DEBUG
			std::cout << "Usage 3: crc32 -u\n";
#endif
			std::cout << "Options:\n";
			std::cout << "  -i STRING\t\tSpecify file to process.\n";
			std::cout << "  -l, --length INT\tHow many bytes to process.\n";
			std::cout << "  -s, --start INT\tFrom which byte to start.\n";
			std::cout << "  --self\t\tCompute CRC of own executable.\n";
			std::cout << "  -h, --help\t\tPrint help screen.\n";
#ifdef DEBUG
			std::cout << "  -u, --unit\t\tPerform unit tests.\n";
#endif
			return 0;
		}
		else
		{
			std::cout << "Parameter " << argv[i] << " not defined." << std::endl;
			quit = true;
		}
	}

	// At least one parameter was invalid, stop
	if(quit)
		return -1;

	switch(parameters.mode)
	{
	case parameters_t::NONE:
		std::cout << "Invalid list of parameters.\n";
		return -1;
	case parameters_t::UNIT_TESTS:
#ifdef DEBUG
		run_tests();
#else
		std::cout << "Unit tests not available in Release build." << std::endl;
#endif
		break;
	case parameters_t::FILE:
		uint32_t crc = crc32_file(parameters.input, parameters.start, parameters.length);
		std::cout << "Int8 Little endian: " << std::hex <<
				"0x" << ((crc & 0x000000ff) >> 0) << ' ' <<
				"0x" << ((crc & 0x0000ff00) >> 8) << ' ' <<
				"0x" << ((crc & 0x00ff0000) >> 16) << ' ' <<
				"0x" << ((crc & 0xff000000) >> 24) << ' ' <<
				"\nInt32 Big endian: " << "0x" << crc <<
				std::endl;
		break;
	}

	return 0;
}
