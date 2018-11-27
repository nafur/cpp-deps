#include "commands.h"
#include "graph.h"

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
		("commands", po::value<std::string>()->required(), "path to compile_commands.json")
		("output", po::value<std::string>()->default_value(""), "output file for graphviz file")
	;
	po::variables_map vm;
	po::store(po::parse_command_line(argc, argv, desc), vm);
	try {
		po::notify(vm);
	} catch (std::exception& e) {
		std::cerr << "Error: " << e.what() << std::endl << desc << std::endl;
		return 1;
	}

	if (vm.count("help")) {
		std::cerr << desc << std::endl;
		return 0;
	}

	cppdeps::Graph graph;
	{
		auto queue = cppdeps::read_compile_commands(vm["commands"].as<std::string>());
		Executor e{ graph };
		std::for_each(queue.begin(), queue.end(),
			[&e](const auto& f){ e(f); }
		);
	}

	graph.layout();
	graph.write_graphviz(vm["output"].as<std::string>());

	return 0;
}