#include "graph.h"

#include <boost/graph/adjacency_list.hpp>
#include <boost/graph/circle_layout.hpp>
#include <boost/graph/fruchterman_reingold.hpp>
#include <boost/graph/graph_traits.hpp>
#include <boost/graph/graphviz.hpp>
#include <boost/graph/labeled_graph.hpp>
#include <boost/graph/point_traits.hpp>

#include <regex>

namespace cppdeps {

std::regex line_regex("(\\.+) (.+)");

std::size_t longest_common_prefix(const std::string& s, const std::string& t) {
	std::size_t res = 0;
	while (res < s.size() && res < t.size() && s[res] == t[res]) ++res;
	return res;
}

struct VertexData {
	std::string name;
	boost::square_topology<>::point point;
};
struct BoostGraph: public boost::labeled_graph<
	boost::adjacency_list<
		boost::vecS, boost::vecS, boost::directedS, VertexData
	>,
	std::string
> {};

template<typename G>
struct vertex_property_writer {
	template<typename VertexOrEdge>
	void operator()(std::ostream& out, const VertexOrEdge& v) const {
		out << "[";
		out << "label=" << boost::escape_dot_string(graph[v].name) << ", ";
		out << "pos=\"" << graph[v].point[0] << "," << graph[v].point[1] << "!\"";
		out << "]";
	}
	const G& graph;
};
template<typename G>
auto make_vertex_property_writer(const G& g) {
	return vertex_property_writer<G>{ g };
}

bool Graph::consider_file(const fs::path& filename) {
	const auto name = filename.string();
	for (const auto& pattern: mExcludes) {
		if (name.find(pattern) != std::string::npos) return false;
	}
	return true;
}

Graph::Graph(const std::vector<std::string>& excludes): mGraph(std::make_unique<BoostGraph>()), mExcludes(excludes) {}
Graph::~Graph() {}

void Graph::parse_output(const std::vector<std::string>& output, const std::string& filename) {
	std::vector<std::string> stack({ filename });
	for (const auto& o: output) {
		std::smatch match;
		if (std::regex_match(o, match, line_regex)) {
			std::size_t indent = match[1].length();
			assert(indent <= stack.size());
			if (indent == stack.size()) {
				stack.emplace_back(match[2]);
			} else {
				while (indent < stack.size() - 1) {
					stack.pop_back();
				}
				stack.back() = match[2];
			}

			auto source_file = fs::canonical(stack[stack.size() - 2]);
			auto target_file = fs::canonical(stack[stack.size() - 1]);

			if (consider_file(source_file) && consider_file(target_file)) {
				std::lock_guard<std::mutex> guard(mMutex);
				auto source = boost::add_vertex(source_file, VertexData{ source_file }, *mGraph);
				auto target = boost::add_vertex(target_file, VertexData{ target_file }, *mGraph);
				if (!boost::edge(source, target, *mGraph).second) {
					boost::add_edge(source, target, *mGraph);
				}
			}
		}
	}
}

void Graph::clean() {
	const auto& vert = boost::vertices(*mGraph);
	std::string cur_prefix = mGraph->graph()[*vert.first].name;
	std::size_t prefix_length = cur_prefix.size();
	for (auto it = vert.first; it != vert.second; ++it) {
		prefix_length = std::min(prefix_length, longest_common_prefix(cur_prefix, mGraph->graph()[*it].name));
	}
	for (auto it = vert.first; it != vert.second; ++it) {
		mGraph->graph()[*it].name = mGraph->graph()[*it].name.substr(prefix_length);
	}
}

void Graph::layout() {
	boost::circle_graph_layout(*mGraph, boost::get(&VertexData::point, *mGraph), 15.0);
	
	boost::fruchterman_reingold_force_directed_layout(
		*mGraph,
		boost::get(&VertexData::point, *mGraph),
		boost::square_topology<>(40.0),
		boost::cooling(boost::linear_cooling<double>(100))
	);
}

void Graph::write_graphviz(const fs::path& filename) const {
	if (filename == "") {
		boost::write_graphviz(std::cout, *mGraph, make_vertex_property_writer(mGraph->graph()));
	} else {
		std::ofstream out(filename.string());
		boost::write_graphviz(out, *mGraph, make_vertex_property_writer(mGraph->graph()));
		out.close();
	}
}

}