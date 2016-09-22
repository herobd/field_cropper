
#include "IRIS_File.h"
int main()
{
        IRIS_File* groundTruthFile = new IRIS_File();
        groundTruthFile->load("transcription/USDistrictofColumbia-1930FederalCensus/004531896_00078.xml");
        return 0;
}
