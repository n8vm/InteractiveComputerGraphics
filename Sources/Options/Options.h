/*******************************************************
 ** Generalized Voronoi Diagram Project               **
 ** Copyright (c) 2015 John Martin Edwards            **
 ** Scientific Computing and Imaging Institute        **
 ** 72 S Central Campus Drive, Room 3750              **
 ** Salt Lake City, UT 84112                          **
 **                                                   **
 ** For information about this project contact        **
 ** John Edwards at                                   **
 **    edwardsjohnmartin@gmail.com                    **
 ** or visit                                          **
 **    sci.utah.edu/~jedwards/research/gvd/index.html **
 *******************************************************/

#pragma once

#include <string>
#include <vector>
#include <set>
#include <map>

 //------------------------------------------------------------------------------
 // Options
 //------------------------------------------------------------------------------
namespace Options {
	extern std::string objLocation;

  bool ProcessArg(int& i, char** argv);
	int ProcessArgs(int argc, char** argv);
};