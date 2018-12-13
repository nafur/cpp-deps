#pragma once

#include <filesystem>
#include <map>
#include <memory>
#include <mutex>
#include <string>
#include <vector>

namespace cppdeps {

namespace fs = std::filesystem;

struct BoostGraph;

class Graph {
	std::unique_ptr<BoostGraph> mGraph;
	std::map<std::string,std::size_t> mVertices;
	std::mutex mMutex;
	std::vector<std::string> mExcludes;

	bool consider_file(const fs::path& filename);
public:
	Graph(const std::vector<std::string>& excludes);
	~Graph() noexcept;
	void parse_output(const std::vector<std::string>& output, const std::string& filename);
	void clean();
	void analyze_components();
	void layout();
	void write_graphviz(const fs::path& filename = "") const;
};

}