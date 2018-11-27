#pragma once

#include <filesystem>
#include <memory>
#include <mutex>
#include <string>
#include <vector>

namespace cppdeps {

namespace fs = std::filesystem;

struct BoostGraph;

class Graph {
	std::unique_ptr<BoostGraph> mGraph;
	std::mutex mMutex;
public:
	Graph();
	~Graph() noexcept;
	void parse_output(const std::vector<std::string>& output, const std::string& filename);
	void layout();
	void write_graphviz(const fs::path& filename = "") const;
};

}