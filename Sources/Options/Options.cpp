#include <cstdlib>
#include <fstream>
#include "./Options.h"

//------------------------------------------------------------------------------
// Options
//------------------------------------------------------------------------------
namespace Options {
	std::string objLocation = ResourcePath "Teapot/teapot.obj";
	//std::string objLocation = ResourcePath "Chalet/chalet.obj";
	
#define $(flag) (strcmp(argv[i], flag) == 0)
	bool ProcessArg(int& i, char** argv) {
		int orig_i = i;
		if $("--obj") {
			printf("Setting device\n");
			++i;
			objLocation = std::string(argv[i]);
			++i;
		}
		/*else if $("-v") {
			++i;
			debug = true;
		}*/

		return i != orig_i;
	}
#undef $
	int ProcessArgs(int argc, char** argv) {
		using namespace Options;

		int i = 1;
		bool stop = false;
		while (i < argc && !stop) {
			stop = true;
			if (ProcessArg(i, argv)) {
				stop = false;
			}
		}

		/* Process the first filename at the end of the arguments as a model */
		for (; i < argc; ++i) {
			std::string filename(argv[i]);
			objLocation = filename;
		}
		return 0;
	}
}
