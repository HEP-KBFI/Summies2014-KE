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
