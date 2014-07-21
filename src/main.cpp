#include <map> // std::map
#include <string> // std::string
#include <vector> // std::vector
#include <iostream> // std::cerr, std::endl
#include <utility> // std::make_pair

#include <TString.h>
#include <TTree.h>
#include <TFile.h>
#include <TH1F.h>

#include "Common.h"
#include "InputData.h"
#include "FilePointer.h"
#include "HistoManager.h"

/**
 * @note
 *   - boost linked statically, root dynamically
 *   - assumptions -- the same name for a TTree for all input files
 *         -# the same name for a TTree for all input files
 *         -# single file to work with
 * 
 * @todo
 *   - transition to the files needed for this project
 *   - consider multiple file case
 *   - look into dlopen (probable dyn lib linking path mismatch), or just cheat with LD_LIBRARY_PATH
 *   - documentation
 *   - proper error handling?
 *   - logging?
 */

int main(int argc, char ** argv) {
	
	std::shared_ptr<InputData> input(new InputData(argc, argv));
	std::shared_ptr<SingleFilePointer> sigPointers(new SingleFilePointer(input, SIGNAL));
	HistoManager hm(input);
	
	sigPointers -> openFile();
	sigPointers -> openTree();
	
	hm.initRanges();
	hm.createFile("recreate");
	hm.cd();
	hm.process(sigPointers);
	hm.write();
	hm.close();
	
	sigPointers -> close();
	
	return EXIT_SUCCESS;
}



