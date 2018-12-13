#include "commands.h"
#include "graph.h"

namespace std {
template<typename T>
std::ostream& operator<<(std::ostream& os, const std::vector<T>& v) {
	os << "[";
	bool first = true;
	for (const auto& it: v) {
		if (!first) os << ", ";
		first = false;
		os << it;
	}
	return os << "]";
}
}

#include <boost/program_options.hpp>

#include <chrono>
#include <future>
#include <iostream>
#include <string>

using namespace std::chrono_literals;

struct Executor {
	cppdeps::Graph& graph;
	std::vector<std::future<void>> futures;
	std::atomic<std::size_t> thread_counter = 0;
	const std::size_t max_threads = std::thread::hardware_concurrency() / 2;

	void operator()(const cppdeps::File& file) {
		while (thread_counter >= max_threads) {
			std::this_thread::sleep_for(1ms);
		}
		thread_counter += 1;

		futures.emplace_back(
			std::async(std::launch::async,
				[this](const auto& q) {
					graph.parse_output(cppdeps::execute_file(q), q.file);
					thread_counter -= 1;
				},
				file
			)
		);
	}
};

namespace po = boost::program_options;

int main(int argc, char* argv[]) {
	po::options_description desc("Options");
	desc.add_options()
		("help", "show help")
		("source-dir", po::value<std::string>()->required(), "path to use for cmake configuration")
		("output", po::value<std::string>()->default_value(""), "output file for graphviz file")
		("exclude", po::value<std::vector<std::string>>()->multitoken()->default_value(
			{"/usr/include/", "/usr/lib/", "/build/"}
		), "exclude patterns for files")
	;
	po::variables_map vm;
	po::store(po::parse_command_line(argc, argv, desc), vm);

	if (vm.count("help")) {
		std::cerr << desc << std::endl;
		return 0;
	}
	
	try {
		po::notify(vm);
	} catch (std::exception& e) {
		std::cerr << "Error: " << e.what() << std::endl << desc << std::endl;
		return 1;
	}


	cppdeps::Graph graph(vm["exclude"].as<std::vector<std::string>>());
	{
		auto tmpdir = cppdeps::get_tmp_dir();
		cppdeps::init_tmp_dir(tmpdir);
		cppdeps::configure_cmake(tmpdir, vm["source-dir"].as<std::string>());
		auto queue = cppdeps::read_compile_commands(tmpdir / "compile_commands.json");
		Executor e{ graph };
		std::for_each(queue.begin(), queue.end(),
			[&e](const auto& f){ e(f); }
		);
		cppdeps::cleanup_tmp_dir(tmpdir);
	}

	graph.clean();
	graph.analyze_components();
	graph.layout();
	graph.write_graphviz(vm["output"].as<std::string>());

	cppdeps::render_dot_file(vm["output"].as<std::string>());

	return 0;
}