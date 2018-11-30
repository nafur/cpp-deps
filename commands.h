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

fs::path get_tmp_dir();
void init_tmp_dir(const fs::path& tmpdir);
void cleanup_tmp_dir(const fs::path& tmpdir);
void configure_cmake(const fs::path& tmpdir, const fs::path& sourcedir);
Files read_compile_commands(const fs::path& filename);
std::vector<std::string> execute_file(const File& file);

}