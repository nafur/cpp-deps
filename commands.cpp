#include "commands.h"

#include <boost/process.hpp>
#include <boost/process/start_dir.hpp>
#include <boost/property_map/property_map.hpp>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>

namespace cppdeps {

namespace pt = boost::property_tree;
namespace pr = boost::process;

Files read_compile_commands(const fs::path& filename) {
	Files files;
	pt::ptree tree;
	pt::read_json(filename, tree);

	for (const auto& entry: tree) {
		files.emplace_back(File {
			entry.second.get<std::string>("file"),
			entry.second.get<std::string>("directory"),
			entry.second.get<std::string>("command")
		});
	}
	return files;
}

std::vector<std::string> execute_file(const File& file) {
	std::vector<std::string> output;

	pr::ipstream stream;
	pr::child c(file.command + " -E -H", pr::std_err > stream, pr::start_dir = file.directory.string());

	std::string line;
	while (stream && std::getline(stream, line) && !line.empty())
		output.emplace_back(line);
	c.wait();
	return output;
}

}