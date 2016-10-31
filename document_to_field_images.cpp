//Authored by:  Brian Davis, Robert Clawson
//Date:         August 2016, about Feb 2012
//

#include "dimage.h"
#include "dslantangle.h"
#include "drect.h"
#include "dpoint.h"
#include "ddefs.h"
#include <string>
#include "census_proj.h"
#include <algorithm>
#include <iostream>
#include <sys/stat.h>
#include <sys/types.h>
#include <sstream>
#include "dthresholder.h"
#include "IRIS_File.h"
#include <fstream>
#include <vector>
#include <assert.h>


using namespace std;

#define SAVE_IMAGES 0

#define NUM_RECORDS 50
//#define NUM_FIELDS 27
//#define NUM_FIELDS_USING 13

#define FIRSTPAGE 8

//I am setting this definition because i am not using the
//multiple template functionality
#define TEMPLATE_DEFAULT 0
//#define CROP  4
#define CROP  0

ofstream outNames;
ofstream outGTP;
int minX, minY, maxX, maxY;
string croppedDir;
string croppedFileName;

vector< vector<DRect> > fields;
vector< vector<DRect> > refinedFields;

/*int numRecordFields;
int fieldCount;
int refinedFieldsSize;*/

IRIS_File* groundTruthFile=NULL;

//std::string fieldNames [NUM_FIELDS_USING];
std::vector<string> fieldNames;
//int redactedFileIndex = FIRSTPAGE;

//std::string enumerators [1127];

//static int fUsingMapping [NUM_FIELDS_USING] = {2,3,4,6,7,8,9,10,14,15,16,18,20};

/*int tToGTIndexMapping(int templateIndex)
{
    //Template to Ground truth index mapping
    static int mapping [27] = {-1,-1,12,17,10,-1,18,15,11,8,7,-1,-1,
                               -1,1,2,0,-1,4,-1,9,-1,-1,-1,-1,-1,-1};

    return mapping[templateIndex];
}*/

//static int fileNameNumbers [NUM_FIELDS_USING] = {1,1,1,1,1,1,1,1,1,1,1,1,1};
static std::vector<int> fileNameNumbers;

/*template <class T>
inline std::string to_string (const T& t)
{
    std::stringstream ss;
    ss << t;
    return ss.str();
}*/

/*void readEnumeratorFile(char* enumeratorFileName)// = "enumerators/enumerators_1920_us_census_4390330.csv")
{
    std::string line;
    std::ifstream myfile (enumeratorFileName);

    int strIndex;
    int enuIndex=0;


    if (myfile.is_open())
    {
        while ( myfile.good() )
        {
            getline (myfile,line);
            strIndex = line.find(",", 0);
            enumerators[enuIndex] = line.substr(strIndex+1,line.length() - 1);
            enuIndex++;
        }
        myfile.close();
    }
    else std::cout << "Unable to open file" << std::endl;
}*/


/* The format I'm expecting is a csv of "x1 y1 x2 y2",
   each line is a record, each comma seperated value
   is a field. The first line is a header
   */
void initialize(std::string projectFileName)
{
    /*int tnum=0;
    int numRecords;
    int numRecFields;
    int numTotFields;
    int numRects;
    int x0, y0, x1, y1;

    int fieldIndex = 0;

    CField *pCField;
    CensusProjectInfo prj;

    if(!prj.load(projectFileName.c_str())){
        fprintf(stderr, "ERROR loading project file '%s'\n",projectFileName.c_str());
        exit(1);
    }


    numTotFields = 0;
    numRecords = prj.rgTemplates[tnum].numRecords;
    for(int rr=0; rr < numRecords; ++rr)
    {
        numRecordFields = numRecFields = prj.rgTemplates[tnum].rgRecords[rr].numFields;
        numTotFields += numRecFields;

        for(int ff=0; ff < numRecFields; ++ff)
        {
            pCField = &(prj.rgTemplates[tnum].rgRecords[rr].rgFields[ff]);
            numRects = pCField->numRects;
            for(int rect=0; rect < numRects; ++rect)
            {
                x0=pCField->rgRects[rect][0];
                y0=pCField->rgRects[rect][1];

                x1 = x0 + pCField->rgRects[rect][2];
                y1 = y0 + pCField->rgRects[rect][3];

                //fields[fieldIndex++] = DRect(x0,y0,x1-x0,y1-y0);
                fields[recordIndex][fieldIndex].x = x0;
                fields[recordIndex][fieldIndex].y = y0;
                fields[recordIndex][fieldIndex].w = x1-x0;
                fields[recordIndex][fieldIndex].h = y1-y0;
                fieldIndex++;
            }
        }
    }
    fieldCount = fieldIndex;*/
    ifstream in (projectFileName);

    string line;
    getline(in,line);//header
    stringstream sshead(line);
    string headername;
    fieldNames.clear();
    while (getline(sshead,headername,','))
    {
        fieldNames.push_back(trim(headername));
    }
    int recordIdx=0;
    while (getline(in,line))
    {
        fields.resize(recordIdx+1);
        stringstream ss(line);
        string fieldStr;
        //int fieldIdx;
        while (getline(ss,fieldStr,','))
        {
            stringstream ss2(fieldStr);
            string number;
            getline(ss2,number,' ');
            int x1=stoi(number);
            getline(ss2,number,' ');
            int y1=stoi(number);
            getline(ss2,number,' ');
            int x2=stoi(number);
            getline(ss2,number,' ');
            int y2=stoi(number);

            DRect r(x1,y1,x2-x1+1,y2-y1+1);
            fields[recordIdx].push_back(r);
        }
        recordIdx++;
    }
}

D_uint8 getPixel(DImage& img, int x, int y)
{
    assert(x>=0 && x<img.width());
    assert(y>=0 && y<img.height());
    D_uint8* data = img.dataPointer_u8();

    return data[y*img.width() + x];
}

int right(DRect& r)
{
    return r.x + r.w-1;
}

int bottom(DRect& r)
{
    return r.y + r.h-1;
}

//Scores the alignment of the fields based on the x and y shift
int scoreFields(DImage& source, int xShift, int yShift)
{
    int ret=0;
    for(int recordIndex = 0; recordIndex < fields.size(); recordIndex++)
    {
        for(int fieldIndex = 0; fieldIndex < fields[recordIndex].size(); fieldIndex++)
        {
            int topY=max(0,fields[recordIndex][fieldIndex].y + yShift);
            int botY=min(source.height()-1,topY+fields[recordIndex][fieldIndex].h-1);
            int leftX=max(0,fields[recordIndex][fieldIndex].x + xShift);
            int rightX=min(source.width()-1,leftX+fields[recordIndex][fieldIndex].w-1);
            if (botY-topY < 10 || rightX-leftX<10)
                return 0;
            for (int y=topY; y<=botY; y++)
            {
                ret+=std::abs(getPixel(source, leftX,y) - 255);
                ret+=std::abs(getPixel(source, rightX,y) - 255);
            }
            for (int x=leftX+1; x<rightX; x++)
            {
                ret+=std::abs(getPixel(source, x,topY) - 255);
                ret+=std::abs(getPixel(source, x,botY) - 255);
            
            }
        }
    }
    return ret;
}

//Refine the form registration
//Sets refinedFields
void refineReg(DImage& source)
{
    int FACTOR=50;
    int FACTOR2=7;
    int origScore = scoreFields(source,0,0);

    int maxScore=0;
    int bestY;
    for (int yShift=-FACTOR; yShift<=FACTOR; yShift++)
    {
        int score = scoreFields(source,0,yShift);
        if (score>maxScore)
        {
            maxScore=score;
            bestY=yShift;
        }
    }
    maxScore=0;
    int bestX;
    for (int xShift=-FACTOR; xShift<=FACTOR; xShift++)
    {
        int score = scoreFields(source,xShift,0);
        if (score>maxScore)
        {
            maxScore=score;
            bestX=xShift;
        }
    }
    
    maxScore=0;
    for (int yShift=-FACTOR2; yShift<=FACTOR2; yShift++)
        for (int xShift=-FACTOR2; xShift<=FACTOR2; xShift++)
        {
            int score = scoreFields(source,xShift,yShift);
            if (score>maxScore)
            {
                maxScore=score;
                bestX=xShift;
                bestY=yShift;
            }
        }

    refinedFields.resize(fields.size());
    cout <<"finetune registration: "<<bestX<<", "<<bestY<<endl;
    for(int recordIndex = 0; recordIndex < refinedFields.size(); recordIndex++)
    {
        refinedFields[recordIndex].resize(fields[recordIndex].size());
        for(int fieldIndex = 0; fieldIndex < refinedFields[recordIndex].size(); fieldIndex++)
        {
            if (maxScore>origScore)
            {
                refinedFields[recordIndex][fieldIndex].x = fields[recordIndex][fieldIndex].x+bestX; 
                refinedFields[recordIndex][fieldIndex].y = fields[recordIndex][fieldIndex].y+bestY;
                refinedFields[recordIndex][fieldIndex].w = fields[recordIndex][fieldIndex].w; 
                refinedFields[recordIndex][fieldIndex].h = fields[recordIndex][fieldIndex].h;
            }
            else
            {
                refinedFields[recordIndex][fieldIndex].x = fields[recordIndex][fieldIndex].x; 
                refinedFields[recordIndex][fieldIndex].y = fields[recordIndex][fieldIndex].y;
                refinedFields[recordIndex][fieldIndex].w = fields[recordIndex][fieldIndex].w; 
                refinedFields[recordIndex][fieldIndex].h = fields[recordIndex][fieldIndex].h;
            }
        }
    } 
}
void improveFields(DImage& source)
{
    int FACTOR = 12;

    int refinedFieldsIndex=0;

    int columnLength;
    int rowLength;
    int columnIndex;
    int rowIndex;
    int maxVal;
    int newLeft;
    int newRight;
    int newTop;
    int newBottom;
    int  y;
    int  x;
    int bottomVal;
    int rightVal;
    std::vector<int> columns;
    std::vector<int> rows;
    for(int recordIndex = 0; recordIndex < groundTruthFile->numRecords(); recordIndex++)
    {
        for(int fieldIndex = 0; fieldIndex < refinedFields[recordIndex].size(); fieldIndex++)
        {
            
            if (groundTruthFile->record(recordIndex).find(fieldNames[fieldIndex]) == groundTruthFile->record(recordIndex).end() ||
                    groundTruthFile->record(recordIndex).at(fieldNames[fieldIndex]).length()==0)
            {
                continue;
            }
            int test;
            if (fieldIndex==3)
                test=stoi(groundTruthFile->record(recordIndex).at(fieldNames[fieldIndex]));
            columnLength = refinedFields[recordIndex][fieldIndex].w + 2 * FACTOR;
            rowLength = refinedFields[recordIndex][fieldIndex].h    + 2 * FACTOR;
            columns.assign(columnLength,0);
            rows.assign(rowLength,0);
            columnIndex = 0;
            rowIndex = 0;

            //ROW and COLUMN profiles
            y=  std::max(refinedFields[recordIndex][fieldIndex].y - FACTOR,0);
            bottomVal = bottom(refinedFields[recordIndex][fieldIndex]);
            for(;y < std::min(bottomVal + FACTOR,source.height()); y++)
            {
                x  =  std::max(refinedFields[recordIndex][fieldIndex].x - FACTOR,0);
                rightVal = right(refinedFields[recordIndex][fieldIndex]);
               for(;x < std::min(rightVal + FACTOR,source.width()); x++)
                {
                    columns.at(columnIndex) += std::abs(getPixel(source, x,y) - 255);
                    rows.at(rowIndex) += std::abs(getPixel(source, x,y) - 255);
                    columnIndex++;
                }
                columnIndex = 0;
                rowIndex++;
            }

            //Move LEFT Border
            maxVal = -1;
            for(columnIndex = 0; columnIndex < 2*FACTOR;columnIndex++)
            {
                if(columnIndex >= columnLength)
                    printf("ERROR overwriting buffer\n");
                if(columns.at(columnIndex) > maxVal)
                {
                    maxVal = columns.at(columnIndex);
                    newLeft = columnIndex;
                }
            }

            //Move RIGHT Border
            maxVal = -1;
            for(columnIndex = columnLength - 2*FACTOR; columnIndex < columnLength;columnIndex++)
            {
                if(columnIndex >= columnLength)
                    printf("ERROR overwriting buffer\n");
                if(columns.at(columnIndex) > maxVal)
                {
                    maxVal = columns.at(columnIndex);
                    newRight = columnIndex;
                }
            }

            //Move TOP Border
            maxVal = -1;
            for(rowIndex = 0; rowIndex < 2*FACTOR;rowIndex++)
            {
                if(rowIndex >= rowLength)
                    printf("ERROR overwriting buffer\n");
                if(rows.at(rowIndex) > maxVal)
                {
                    maxVal = rows.at(rowIndex);
                    newTop = rowIndex;
                }
            }

            //Move BOTTOM Border
            maxVal = -1;
            for(rowIndex = rowLength - 2*FACTOR; rowIndex < rowLength;rowIndex++)
            {
                if(rowIndex >= rowLength)
                    printf("ERROR overwriting buffer\n");
                if(rows.at(rowIndex) > maxVal)
                {
                    maxVal = rows.at(rowIndex);
                    newBottom = rowIndex;
                }
            }

            //printf("FieldIndex : %d\n", fieldIndex);

            int x0;
            int y0;
            int y1;
            int x1;
            x0 = refinedFields[recordIndex][fieldIndex].x + newLeft   - FACTOR;
            x1 = refinedFields[recordIndex][fieldIndex].x + newRight   - FACTOR;
            y0 = refinedFields[recordIndex][fieldIndex].y + newTop    - FACTOR;
            y1 = refinedFields[recordIndex][fieldIndex].y + newBottom    - FACTOR;

            refinedFields[recordIndex][fieldIndex].x = x0;
            refinedFields[recordIndex][fieldIndex].y = y0;
            refinedFields[recordIndex][fieldIndex].w = x1-x0+1;
            refinedFields[recordIndex][fieldIndex].h = y1-y0+1;

    //        std::cout << "old : " <<
    //                fields[refinedFieldsIndex].x << " "
    //             << fields[refinedFieldsIndex].y << " "
    //             << fields[refinedFieldsIndex].w << " "
    //             << fields[refinedFieldsIndex].h << std::endl;
    //        std::cout << "new : " <<
    //                refinedFields[refinedFieldsIndex].x << " "
    //             << refinedFields[refinedFieldsIndex].y << " "
    //             << refinedFields[refinedFieldsIndex].w << " "
    //             << refinedFields[refinedFieldsIndex].h << std::endl;

            refinedFieldsIndex++;
            //DRect newFieldRect = DRect(x0,y0,x1-x0,y1-y0);
            //refinedFields[refinedFieldsIndex++] = newFieldRect;
        }
    }
    //refinedFieldsSize = refinedFieldsIndex;
}

DImage imageFromFields(DImage& source)
{
    DImage fieldsOnly = DImage();
    fieldsOnly.create(source.width(),source.height(), DImage::DImage_u8);
    fieldsOnly.fill(255);

    for(int rI=0 ;rI < groundTruthFile->numRecords(); rI++)
    for(int fI=0 ;fI < fields[rI].size(); fI++)
    {
//        for(int y = refinedFields[rI].y + CROP; y <= bottom(refinedFields[rI]) - CROP;y++)
//        {
//            for(int x = refinedFields[rI].x + CROP; x <= right(refinedFields[rI]) - CROP; x++)
//            {
//                fieldsOnly.setPixel(x,y,getPixel(source,x,y));
//            }
//        }
        for(int y = fields[rI][fI].y + CROP; y <= bottom(fields[rI][fI]) - CROP;y++)
        {
            for(int x = fields[rI][fI].x + CROP; x <= right(fields[rI][fI]) - CROP; x++)
            {
                fieldsOnly.setPixel(x,y,getPixel(source,x,y));
            }
        }
    }

    return fieldsOnly;
}

double getSlantAngle(DImage &dImg)
{
    int RUNLENGTH_THRESHOLD = 1;

    return DSlantAngle::getTextlineSlantAngleDeg(dImg,RUNLENGTH_THRESHOLD);
}

std::string outputDirectory;

void createFolderStructure(std::string imageFolder)
{
    std::string directoryPath = "";

    directoryPath += outputDirectory;
    directoryPath += imageFolder;


    mkdir (directoryPath.c_str(), S_IRWXU | S_IRWXG | S_IRWXO);

    directoryPath = "";

    std::cout << "outputDirectory: " << outputDirectory.c_str() << std::endl;

    for(int x = 0; x < fieldNames.size();x++)
    {
        directoryPath += outputDirectory;
        directoryPath += std::string("/");
        directoryPath += fieldNames[x];
        mkdir(directoryPath.c_str(), S_IRWXU | S_IRWXG | S_IRWXO);
        directoryPath = "";
    }
    croppedDir = outputDirectory;
    croppedDir += std::string("/");
    croppedDir += "NAME_COLUMNS";
    mkdir(croppedDir.c_str(), S_IRWXU | S_IRWXG | S_IRWXO);
    croppedDir += "/";
}


bool fExists(const char *filename)
{
  std::ifstream ifile(filename);
  return ifile;
}

//TODO: for now, we are cheating. Better to look at image and decide if blank,
// but that is more work when we have ground truth already
bool blankField(DImage& current)
{
    const char* groundTruth = current.getPropertyValByIndex(0).c_str();
    if(0 == strcmp(groundTruth, "") ||
            0 == strcmp(groundTruth, "_NA_"))
        return true;
    else
        return false;
}


//done for each field. crops only rows and columns that are completely blank right now.
void cropWhitespace(DImage& img)
{
    D_uint8 curPix;
    int h = img.height();
    int w = img.width();

    int rows [h];
    int columns [w];

    for(int a = 0; a < h;a++)
        rows[a] = 0;
    for(int a = 0; a < w;a++)
        columns[a] = 0;

    for(int y = 0; y < h ; y++)
    {
        for(int x = 0; x < w; x++)
        {
            curPix = getPixel(img,x,y);
            if(curPix == 0){
                rows[y] ++;
                columns[x] ++;
            }
        }
    }

    int x0 = 1;
    while(columns[x0] <= columns[x0-1])
        x0++;

    int x1 = w - 2;
    while(columns[x1] <= columns[x1+1])
        x1--;

    int y0 = 1;
    while(rows[y0] <= rows[y0-1])
        y0++;

    int y1 = h - 2;
    while(rows[y1] <= rows[y1+1])
        y1--;

    img = img.copy(x0,y0,x1-x0,y1-y0);
}

void chopFields(int recordNum, DImage& source, double skewAngle)
{
    DImage current;
    std::string filename = "";
    int x,y,w,h, rFIndex, fi;//fi means fieldIndex
    int thresVal [1] = {1} ;

    std::string imgNumStr = groundTruthFile->strImageFile();

    //imgNumStr = imgNumStr.substr(10);

    //int imgNum = atoi(imgNumStr.c_str());
    //imgNum -= 9;//since we start on the ninth image

    for(int z = 0; z < groundTruthFile->numFields();z++)
    {
        std::string fieldName=fieldNames[z];
        if (groundTruthFile->record(recordNum).find(fieldName) == groundTruthFile->record(recordNum).end() ||
                groundTruthFile->record(recordNum).at(fieldName).length()==0)
            continue;
        //fi = fUsingMapping[z];
        //rFIndex = recordNum*groundTruthFile->numFields() + z;
        x = refinedFields[recordNum][z].x+CROP;  //Brian, These are the values you want. Back trace from here
        y = refinedFields[recordNum][z].y + CROP;
        w = refinedFields[recordNum][z].w-2*CROP;
        h = refinedFields[recordNum][z].h-2*CROP;
#if SAVE_IMAGES
        current = source.copy(x,y,w,h);


        //TODO
        //PREPROCESSING
        
        //binarize
        //DThresholder::ccThreshImage_(current, current, __null, 8, thresVal);
        
        //deskew
        current = current.shearedH(skewAngle,255,false);
        
        //furthercrop
        //cropWhitespace(current);
        
        //setProperties
        /*QString fullname;
        if(cRecord->numFields > tToGTIndexMapping(fi))
        {
            if(fi == 2)//name field
                current.setProperty("GroundTruth", QString(cRecord->rgFields[13].fieldValue.c_str()).append(" ").append(cRecord->rgFields[12].fieldValue.c_str()).toStdString());
            else
                current.setProperty("GroundTruth", cRecord->rgFields[tToGTIndexMapping(fi)].fieldValue);
        }
        else
            {current.setProperty("GroundTruth", std::string("_NA_"));}*/
        current.setProperty("GroundTruth", groundTruthFile->record(recordNum).at(fieldName));
        current.setProperty("FieldName", fieldName);
        //current.setProperty("FieldType", groundTruthFile->record(recordNum)[z].fieldType);
        //current.setProperty("Readable", groundTruthFile->record(recordNum)[z].fUnreadable?"no":"yes");
        
        current.setProperty("RecordNum", to_string(recordNum));
        current.setProperty("OrigImage", groundTruthFile->strImageFile());
        //current.setProperty("ThresVal", to_string(*thresVal));
        current.setProperty("SkewAngle", to_string(skewAngle));
        current.setProperty("Deskewed", "false");
        current.setProperty("x", to_string(x));
        current.setProperty("y", to_string(y));
        current.setProperty("w", to_string(w));
        current.setProperty("h", to_string(h));
        //current.setProperty("enumerator", enumerators[imgNum]);
        //end setProperties
        //if(blankField(current))
        //    continue;
        //save
//        filename = "";
//        filename += outputDirectory;
//        filename += fieldNames[z];
//        filename += std::string("/");
//        filename += to_string(fileNameNumbers[z]++);
//        filename += std::string(".pgm");
//        current.save(filename.c_str());

        //save
        std::string qFileName="0000000000";
        sprintf(&qFileName[0],"%06d.pgm", fileNameNumbers[z]++);
        filename = "";
        filename += outputDirectory;
        filename += fieldNames[z];
        filename += std::string("/");
        filename += qFileName;
        //cout<<"save: "<<filename<<endl;
        current.save(filename.c_str());
#endif
        if (fieldName.compare("PR_NAME")==0 
                && groundTruthFile->record(recordNum).at(fieldName).length()>0
                && groundTruthFile->record(recordNum).at(fieldName).compare(" ")!=0) 
        {
            if (refinedFields[recordNum][z].x<minX)
                minX=refinedFields[recordNum][z].x;
            if (refinedFields[recordNum][z].y<minY)
                minY=refinedFields[recordNum][z].y;
            if (refinedFields[recordNum][z].x+refinedFields[recordNum][z].w>maxX)
                maxX=refinedFields[recordNum][z].x+refinedFields[recordNum][z].w;
            if (refinedFields[recordNum][z].y+refinedFields[recordNum][z].h>maxY)
                maxY=refinedFields[recordNum][z].y+refinedFields[recordNum][z].h;
            outNames<<filename<<" "<<groundTruthFile->record(recordNum).at(fieldName)<<endl;
            outGTP<<croppedFileName<<" "<<x-minX<<" "<<y-minY<<" "<<(x-minX)+w-1<<" "<<(y-minY)+h-1<<" "<<groundTruthFile->record(recordNum).at(fieldName)<<endl;
        }
    }
}

//Brian's
/*void minCutFieldsOfRecord(DImage& source, double skewAngle)
{
    int thresVal [1] = {1} ;
    FSI_Record* cRecord = groundTruthFile->rgRecords + redactedFileIndex;
    int recIndex = 0;

    if(cRecord->fBlocked || cRecord->numFields == 1)
    {
        std::cout<< "Blocked " << std::endl;
        redactedFileIndex++;// move on to next image
        return;
    }
    
    //binarize
    DImage source_mod;
    DThresholder::ccThreshImage_(source_mod, source, __null, 8, thresVal);
    //deskew
    source_mod = source_mod.shearedH(skewAngle,255,false);
    

    std::string filenameTest = "";
    filenameTest += outputDirectory;
    filenameTest += std::string("/threshdeskew.pgm");

    source_mod.save(filenameTest.c_str());
    
    const char* curImgName = cRecord->strImageFile.c_str();
    
    QVector< QVector<DRect> > imageRectsByField(NUM_FIELDS_USING);
    int redactedFileIndex_iter = redactedFileIndex;
    
    while(0== strcmp(cRecord->strImageFile.c_str(), curImgName))
    {
        if(recIndex < 50) //if the redactedFile has more records than in image should hold
        {    
            int x,y,w,h, rFIndex, fi;//fi means fieldIndex
        
            std::string imgNumStr = cRecord->strImageFile;
        
            imgNumStr = imgNumStr.substr(10);
        
            int imgNum = atoi(imgNumStr.c_str());
            imgNum -= 9;//since we start on the ninth image
            
        
            for(int z = 0; z < NUM_FIELDS_USING;z++)
            {
                fi = fUsingMapping[z];
                rFIndex = recIndex*NUM_FIELDS + fi;
                x = refinedFields[rFIndex].x + CROP;
                y = refinedFields[rFIndex].y + CROP;
                w = refinedFields[rFIndex].w-2*CROP;
                h = refinedFields[rFIndex].h-2*CROP;
                
                DRect r(x,y,w,h);
                imageRectsByField[z].append(r);//append as we are indexing by recIndex
                ///////////////////////////////////
                
            }
        }
        else
            std::cout << "Encountered extra records" << std::endl;

        redactedFileIndex_iter++;
        recIndex++;

        cRecord = groundTruthFile->rgRecords + redactedFileIndex_iter;
    }
    
    //Do yo fancy cuttin' stuff
    
    
    
    recIndex = 0;
    cRecord = groundTruthFile->rgRecords + redactedFileIndex;
    while(0== strcmp(cRecord->strImageFile.c_str(), curImgName))
    {
        if(recIndex < 50) //if the redactedFile has more records than in image should hold
        {    
        
    
            DImage current;
            std::string filename = "";
            int x,y,w,h, rFIndex, fi;//fi means fieldIndex
            
        
            std::string imgNumStr = cRecord->strImageFile;
        
            imgNumStr = imgNumStr.substr(10);
        
            int imgNum = atoi(imgNumStr.c_str());
            imgNum -= 9;//since we start on the ninth image
            
        
            for(int z = 0; z < NUM_FIELDS_USING;z++)
            {
                fi = fUsingMapping[z];
                rFIndex = recIndex*NUM_FIELDS + fi;
                
                DRect r = imageRectsByField[z][recIndex];
                x = r.x;
                y = r.y;
                w = r.w;
                h = r.h;
                
                current = source_mod.copy(x,y,w,h);
                
                
                //furthercrop
                //cropWhitespace(current);
            
                //setProperties
                QString fullname;
                if(cRecord->numFields > tToGTIndexMapping(fi))
                {
                    if(fi == 2)//name field
                        current.setProperty("GroundTruth", QString(cRecord->rgFields[13].fieldValue.c_str()).append(" ").append(cRecord->rgFields[12].fieldValue.c_str()).toStdString());
                    else
                        current.setProperty("GroundTruth", cRecord->rgFields[tToGTIndexMapping(fi)].fieldValue);
                }
                else
                    {current.setProperty("GroundTruth", std::string("_NA_"));}
                current.setProperty("RecordId", cRecord->strRecordId);
                current.setProperty("ImageNum", cRecord->strImageFile);
                current.setProperty("ThresVal", to_string(*thresVal));
                current.setProperty("SkewAngle", to_string(skewAngle));
                current.setProperty("x", to_string(x));
                current.setProperty("y", to_string(y));
                current.setProperty("w", to_string(w));
                current.setProperty("h", to_string(h));
                //current.setProperty("enumerator", enumerators[imgNum]);
                //end setProperties
                if(blankField(current))
                    continue;
                //save
        //        filename = "";
        //        filename += outputDirectory;
        //        filename += fieldNames[z];
        //        filename += std::string("/");
        //        filename += to_string(fileNameNumbers[z]++);
        //        filename += std::string(".pgm");
        //        current.save(filename.c_str());
        
                //save
                QString qFileName;
                qFileName.sprintf("%05d.pgm", fileNameNumbers[z]++);
                filename = "";
                filename += outputDirectory;
                filename += fieldNames[z];
                filename += std::string("/");
                filename += qPrintable(qFileName);
        
                current.save(filename.c_str());
            }
        }
        //else
        //    std::cout << "Encountered extra records" << std::endl;

        redactedFileIndex++;
        recIndex++;

        cRecord = groundTruthFile->rgRecords + redactedFileIndex;
    }
}*/

void chopRecord(int recordNum, DImage& source, double skewAngle)
{
    chopFields(recordNum, source, skewAngle);
}

void chopImage(DImage& source, double skewAngle)
{

    /*if(cRecord->fBlocked || cRecord->numFields == 1)
    {
        std::cout<< "Blocked " << std::endl;
        //redactedFileIndex++;// move on to next image
        return;
    }

    const char* curImgName = cRecord->strImageFile.c_str();*/
    for (int recordNum=0; recordNum<groundTruthFile->numRecords(); recordNum++)
            chopRecord(recordNum,source, skewAngle);
    //while(0== strcmp(cRecord->strImageFile.c_str(), curImgName))
    //{
        //if(recIndex < 50) //if the redactedFile has more records than in image should hold
            //chopRecord(recordNum,source, skewAngle, recIndex);
        //else
        //    std::cout << "Encountered extra records" << std::endl;

        //redactedFileIndex++;
        //recIndex++;

        //cRecord = groundTruthFile->rgRecords + redactedFileIndex;
    //}
}


/*I'll expect a file containing lines of:
  <path to xml>,<path to image>
  */
int main(int argc, char** argv)
{
    char stOutDir[1025];
    struct stat fileStatStruct;

    DImage currentImage;
    DImage fieldImage;
    std::string censusIDName;
    double slantAngle;

    //5000 is arbitrary, just chosen to be big enough
    //fields = new DRect[5000];
    //refinedFields = new DRect[5000];

    int num_images;



    if(argc < 5){
        for(int x = 0; x < argc;x++)
        {
            printf("%s\n", argv[x]);
        }
        fprintf(stderr,
              "usage: %s outputdir pointerfile rootDir templatefile (startPageId)\n",
              argv[0]);
        exit(1);
    }
    //readEnumeratorFile(argv[3]);

    strncpy(stOutDir, argv[1], 1024);
    stOutDir[1024] = '\0';
    if(strlen(stOutDir) < 1)
        strcat(stOutDir, "/");
    if('/' != stOutDir[strlen(stOutDir)-1])
        strcat(stOutDir, "/");
    if(0 != stat(stOutDir, &fileStatStruct)){
        fprintf(stderr, "error trying to stat() outputdir '%s'\n",stOutDir);
        exit(1);
    }
    if(!S_ISDIR(fileStatStruct.st_mode)){
        fprintf(stderr, "outputdir '%s' is NOT a directory!\n", stOutDir);
        exit(1);
    }
    outputDirectory = stOutDir;

    //read in and parse pointerfile
    map<string,string> gt_to_images;
    ifstream in(argv[2]);
    std::string line;
    while (getline(in,line))
    {
        std::stringstream ss(line);
        string gt;
        getline(ss,gt,',');
        string im;
        getline(ss,im,',');
        gt_to_images[gt]=im;
    }
    in.close();
        
    num_images = gt_to_images.size();//argc - 2;  // Number of images

    printf("Processing %d images\n", num_images);
    
    string rootDir=argv[3];
    if (rootDir[rootDir.length()-1]!='/')
        rootDir+='/';
    //Populate template data
    initialize(argv[4]);//"1920.prj");

    bool start = true;
    string startPageId;
    if (argc>5)
    {
        start=false;
        startPageId = argv[5];
        outNames.open(outputDirectory+"names_1930.txt",ios_base::app);
        assert(outNames.is_open());
        outGTP.open(outputDirectory+"names_1930.gtp",ios_base::app);
        assert(outGTP.is_open());
    }
    else
    {
        outNames.open(outputDirectory+"names_1930.txt");
        assert(outNames.is_open());
        outGTP.open(outputDirectory+"names_1930.gtp");
        assert(outGTP.is_open());
    }
    /*//get ground truth
    groundTruthFile = new FSI_File();
    groundTruthFile->load("redacted_indexing/N1821861-001.xml.redacted");


    //set field names
    for(int x = 0; x < NUM_FIELDS_USING;x++)
    {
        fieldNames[x] = groundTruthFile->rgRecords[redactedFileIndex].
                        rgFields[tToGTIndexMapping(fUsingMapping[x])].fieldName;
    }

//    for(int x = 509; x < groundTruthFile.numRecords;x++)
//    {
//        std::cout<<"Page: " << groundTruthFile.rgRecords[x].strRecordId.c_str() << std::endl;

//        for(int y = 0; y < groundTruthFile.rgRecords[x].numFields;y++)
//        {
//            std::cout<<"Name: " << groundTruthFile.rgRecords[x].rgFields[y].fieldValue.c_str() << std::endl;
//        }
//        fflush(stdout);
//        exit(1);

//    }
    */
    createFolderStructure(censusIDName);

    fileNameNumbers.resize(fieldNames.size(),0);
    int minImgH = fields.back().front().y+fields.back().front().h + 5;
    int maxImgH = fields.back().front().y+fields.back().front().h + 80;
    //for(int ci = 2; ci < argc; ++ci){
    for (auto gt_and_image : gt_to_images)
    {
        if (
                gt_and_image.first.compare("transcription/US_Massachusetts-1930_US_Census/004607676_00346.xml")==0 ||
                gt_and_image.first.compare("transcription/US_Massachusetts-1930_US_Census/004607676_00401.xml")==0)
            continue;//xml parser fails on this for some reason
        if (!start)
        {
            int startC=gt_and_image.second.find_last_of('/')+1;
            int endC = gt_and_image.second.find_last_of('.');
            string pageId = gt_and_image.second.substr(startC,endC-startC);
            //cout <<pageId<<endl;
            if (pageId.compare(startPageId)==0)
                start=true;
            else
                continue;
        }
        //if (groundTruthFile!=NULL)
        //    delete groundTruthFile;
        //groundTruthFile = new IRIS_File();
        IRIS_File gtFile;
        groundTruthFile = &gtFile;
        groundTruthFile->load(rootDir+gt_and_image.first);
        minX=minY=9999999;
        maxX=maxY=-999;
        //assert(groundTruthFile->numRecords()==1);
        if (groundTruthFile->numRecords()==0)
            continue;
        for (int i=0; i<groundTruthFile->fieldNames().size(); i++)
        {
            assert(fieldNames[i].compare(groundTruthFile->fieldNames()[i])==0);
        }

        int j;
        j = gt_and_image.second.length() - 1;
        while((j >= 0) && (gt_and_image.second[j] != '/'))
            --j;
        ++j;
        //strcpy(csBaseName, &(gt_and_image.second.c_str[j]));
        censusIDName = gt_and_image.second.substr(j);
        //printf("load '%s'...\n", argv[ci]);
        croppedFileName = croppedDir+censusIDName; 

        censusIDName = censusIDName.substr(0,censusIDName.find_first_of('.'));

        if(!currentImage.load((rootDir+gt_and_image.second).c_str()))
        {
            fprintf(stderr, "ERROR loading input image '%s'\n", gt_and_image.second.c_str());
            //delete groundTruthFile;
            //groundTruthFile=NULL;
            continue;
        }
        if (currentImage.height()<minImgH)
        {
            DImage newImg;
            double scale = (0.0+minImgH)/currentImage.height();
            currentImage.scaled_(newImg, scale, scale,
                                         DImage::DImageTransSmooth);
            currentImage=newImg;
        }
        if (currentImage.height()>maxImgH)
        {
            DImage newImg;
            double scale = (0.0+maxImgH)/currentImage.height();
            currentImage.scaled_(newImg, scale, scale,
                                         DImage::DImageTransSmooth);
            currentImage=newImg;
        }
        assert(minImgH<=currentImage.height() && currentImage.height()<=maxImgH);
        //sets refinedFields
        refineReg(currentImage);

        //modifies refinedFields
        improveFields(currentImage);
        //refinedFields = fields;

        fieldImage = imageFromFields(currentImage);

        slantAngle = getSlantAngle(fieldImage);
        //printf("Slant Angle: %f\n", slantAngle);

        //Brian's function
        //minCutFieldsOfRecord(currentImage, slantAngle);
        //original function
        chopImage(currentImage, slantAngle);
        
        if (minX<maxX)
        {
            DImage croppedFile = currentImage.copy(minX,minY,maxX-minX+1,maxY-minY+1);
            croppedFile.save(croppedFileName.c_str(),DImage::DFileFormat_jpg);
        }

        std::cout<< "Completed image : " << censusIDName.c_str() << std::endl;
    }

    //delete [] fields;
    //delete [] refinedFields;
    //delete groundTruthFile;
    outNames.close();
    return 0;
}
