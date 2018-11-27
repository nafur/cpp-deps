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

fs::path sanitize_filename(const fs::path& path) {
	return fs::canonical(path);
}
fs::path clean_filename(const fs::path& path) {
	return fs::relative(path, fs::path("/home/gereon/carl/src/carl/"));
}

bool consider_file(const fs::path& path) {
	const auto name = path.string();
	if (name.find("/usr/include/") != std::string::npos) return false;
	if (name.find("/usr/lib/") != std::string::npos) return false;
	if (name.find("build/resources/") != std::string::npos) return false;
	if (name.find("examples/") != std::string::npos) return false;
	if (name.find("tests/") != std::string::npos) return false;
	return true;
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

Graph::Graph(): mGraph(std::make_unique<BoostGraph>()) {}
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

			auto source_file = sanitize_filename(stack[stack.size() - 2]);
			auto target_file = sanitize_filename(stack[stack.size() - 1]);

			if (consider_file(source_file) && consider_file(target_file)) {
				source_file = clean_filename(source_file);
				target_file = clean_filename(target_file);
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