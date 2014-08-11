Summies2014-KE
==============

Environment
-----------

A complete GCC 4.8 (C++11-compatible) and ROOT environment can be set using

~~~
export SCRAM_ARCH=slc6_amd64_gcc481
source /cvmfs/cms.cern.ch/cmsset_default.sh
cmsrel CMSSW_7_1_3
cd CMSSW_7_1_3
cmsenv
g++ `root-config --cflags --libs` test.cc -o mytest
./mytest
~~~

where *test.cc* is

~~~
#include <iostream>
#include <TFile.h>

int main(void) {
    auto func = [] (bool x) -> bool { return x; };
    std::cout << "hello" << std::endl;
    TFile* tf = new TFile("test.root", "RECREATE");
    tf->Close();
    return 0;
}
~~~

Programs
--------

analyze.cpp - plots of iterations per event

combinations.cpp - combining btagging probabilities, needs to be modified

consistency.cpp - finds the difference of two histograms normalized to the number of events (which is the same for both)

copytree.cpp - copies only relevant branches from TTree

cumulative.cpp - finds cumulative distributions from given PDFs

cumulplot.cpp - plots the results obtained by cumulative.cpp

efficiency.cpp - finds the efficiency from given PDFs

genrand.cpp - samples PDF once using cumulative distribution (or GetRandom())

gsample.cpp - the same as sample.cpp but uses cumulative distribution

histoplot.cpp - plots the results obtained by process.cpp

nevents.cpp - finds the number of events (i.e. entries) from the given root file

process.cpp - constructs PDFs for CSV and sampled CSV, and finds the histograms for the number of iterations needed to pass WP

sample.cpp - creates new TTree with the entries csvGen (generated CSV value) and csvN (number of iterations needed)

selection.cpp - an attempt to reproduce Fig 1 from AN

stackem.cpp - visualizes the results obtained by selection.cpp

test.cpp - does statistical tests between histograms
