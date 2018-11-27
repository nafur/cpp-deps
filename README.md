# cpp-deps
Visualize C++ include dependencies

# build
Simply run `make`.

Requirements:
- `g++` with support for `c++17`
- `boost` (`graph`, `process`, `program_options`, `property_map`, `property_tree`)

# usage

	Options:
		--help                show help
		--commands arg        path to compile_commands.json
		--output arg          output file for graphviz file
		--exclude arg (=[/usr/include/, /usr/lib/, /build/])
		                      exclude patterns for files


# output
Produces a graph representation of the include dependencies.