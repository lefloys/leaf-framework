#include <iostream>

#include <leaf/core/string.hpp>
#include <leaf/core/vector.hpp>
#include <leaf/leaf.hpp>

int main(int argc, char* argv[]) {
	lf::vector<lf::string_view> args;
	args.reserve(argc);
	for (int i = 0; i < argc; ++i) { args.push_back(argv[i]); }

	const lf::error error = lf::Init(args);
	if (error) { std::cerr << "leaf init failed: " << error.message << "\n"; return 1; }

	lf::Update();

	lf::Exit();
	return 0;
}
