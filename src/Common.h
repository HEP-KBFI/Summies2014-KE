#pragma once

#include <limits> // std::numeric_limits<>
#include <map> // std::map
#include <vector> // std::vector
#include <utility> // std::pair

#include <TString.h>

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
#define XVAR		std::string("xvar")
#define XNAME		std::string("xname")
#define XRANGE		std::string("xrange")
#define WEIGHTVAR	std::string("weightvar")
#define BINS		std::string("bins")
#define HNAME		std::string("hname")

#define inf			std::string("inf")
#define INF 		std::numeric_limits<float>::max()

typedef std::map<TString, std::vector<std::pair<Float_t, Float_t> > >	Ranges;
typedef std::map<std::string, std::vector<std::string> >				StringMap;