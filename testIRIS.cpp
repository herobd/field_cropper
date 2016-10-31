#include <assert.h>
#include "IRIS_File.h"
int main()
{
        IRIS_File* groundTruthFile = new IRIS_File();
        groundTruthFile->load("transcription/USDistrictofColumbia-1930FederalCensus/004531896_00078.xml");
        assert(groundTruthFile->numRecords()>1);
        const std::map< std::string, std::string >& check = groundTruthFile->record(0);
        assert(check.at("PR_NAME").compare("Wagner Hilda")==0);
        delete groundTruthFile;
        groundTruthFile = new IRIS_File();
         groundTruthFile->load("transcription/US_Massachusetts-1930_US_Census/004607676_00346.xml");

        return 0;
}
