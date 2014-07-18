#pragma once

#include <limits> // std::numeric_limits<>

// section nomenclature
#define MISC		std::string("misc")
#define SIGNAL		std::string("signal")
#define BACKGROUND 	std::string("background")
#define DATA		std::string("data")
#define VARIABLES	std::string("variables")
#define RANGES		std::string("ranges")
#define FLAVORS		std::string("flavors")
#define HISTOGRAM	std::string("histogram")
// variable nomenclature
#define DIR			std::string("dir")
#define TREE		std::string("tree")
#define VAR			std::string("var")
#define ID			std::string("id")
#define XVAL		std::string("xval")
#define XNAME		std::string("xname")
#define XRANGE		std::string("xrange")
#define WEIGHTVAR	std::string("weightvar")
#define BINS		std::string("bins")

#define inf			std::string("inf")
#define INF 		std::numeric_limits<float>::max()