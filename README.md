Summies2014-KE
==============

Update (21/07/14)
-----------------

TODO:

1. extract 10k events from the root file
2. modify config file accordingly (possible redesign)
3. sbatch?

Task 1. July 10
---------------

Create a standalone program which takes as a single command line argument the name of a config file. The config file is loaded, the section [INPUTS] is parsed for .root input files containing signal, background and data events in TTrees.

The input .root files are assumed to contain the components of the 4-momenta of particles identified in the event as separate TBranches, among other variables.
One can assume that the variables will be something like

1. jet1_pt, jet1_eta
2. jet2_pt, jet2_eta

etc...

The program will load the relevant input files, loop over the events and perform some elementary operations on the contents of the events. For this, TBranches need to be created for the relevant variables, the branches attached to the TTree and the events loaded using the relevant TTree method inside the loop. The output of these elementary operations will be stored per-event in an output file as a TTree.

Additionally, the *differential distribution* of some particular variable over all the events is stored in a TH1 in the output file.

This program will serve as a skeleton for physics tasks defined later.

Environment
-----------

A complete GCC 4.8 (C++11-compatible) and ROOT environment can be set using

~~~
export SCRAM_ARCH=slc6_amd64_gcc481
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
