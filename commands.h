#pragma once

#include <filesystem>
#include <string>
#include <vector>

namespace cppdeps {

namespace fs = std::filesystem;

struct File {
	fs::path file;
	fs::path directory;
	std::string command;
};
using Files = std::vector<File>;

Files read_compile_commands(const fs::path& filename);
std::vector<std::string> execute_file(const File& file);

}