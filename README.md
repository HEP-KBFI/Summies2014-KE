Summies2014-KE
==============

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
