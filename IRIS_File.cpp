#include "IRIS_File.h"
#include <algorithm>
#include <assert.h>
//#include <libxml/parser.h>
//#include <libxml/tree.h>
#include "tinyxml2.h"
#include <string.h>
#include <regex>
using namespace tinyxml2;
//#define CHECK_EXPECTED(x,y) {if(0!=mystrcmp((char*)x,(char*)y))fprintf(stderr,"parse error! saw '%s' but expected '%s'\n",x,y);}
#define CHECK_EXPECTED(x,y) {if(0!=std::string((char*)x).compare(std::string((char*)y)))fprintf(stderr,"parse error! saw '%s' but expected '%s'\n",x,y);}
//same as strcmp, but doesn't SIGSEG if one (or both) strings are NULL
int mystrcmp(const char *a, const char *b){
  if((NULL==a)&&(NULL!=b))
    return -1;
  if((NULL!=a)&&(NULL==b))
    return 1;
  if((NULL==a)&&(NULL==b))
    return 0;
  return strcmp(a,b);
}
//get the property (attribute) value for the attribute named stPropName from
//an element node
static const char* getPropVal(XMLElement *a_node, const char *stPropName){
  if (a_node == NULL)
    return NULL;  
  /*xmlAttr *attr = NULL;
  if (a_node->type != XML_ELEMENT_NODE) {
    fprintf(stderr, "a_node->type (%d) != XML_ELEMENT_NODE (%s:%d)\n",
            a_node->type,__FILE__, __LINE__);
    exit(1);
  }
  attr= a_node->properties;
  while((NULL!=attr) && (0!=mystrcmp((char*)(attr->name),stPropName))){
    attr = attr->next;
  }
  if(attr){//found the attribute named stPropName, now need the value
    if(NULL != attr->children){
      return (char*)(attr->children->content);
    }
  }
  return NULL;*/
  return a_node->Attribute(stPropName);
}

IRIS_File::IRIS_File(){
    fieldNameList = {"PR_NAME"};
    //fieldNameList = {"PR_NAME","PR_RELATIONSHIP","PR_SEX","PR_AGE","PR_MARITAL_STATUS","PR_BIRTHPLACE","PR_FTHR_BIRTHPLACE","PR_MTHR_BIRTHPLACE"};
}

int IRIS_File::load(std::string stFile){
  /*
  xmlDoc *doc = NULL;
  xmlNode *root_element = NULL;

  xmlNode *page = NULL;
  xmlNode *header = NULL;
  xmlNode *header_item = NULL;
  xmlNode *datum = NULL;
  */
  XMLDocument doc;
  XMLElement *root_element = NULL;

  XMLElement *page = NULL;
  XMLElement *header = NULL;
  XMLElement *header_item = NULL;
  //XMLNode *datum = NULL;
  int fieldNum;
  int recnum;
  const char *pstTmp;
  bool fUnreadable;

  //{LIBXML_TEST_VERSION}
  try
  { 
    doc.LoadFile(stFile.c_str());
  }
  catch (...)
  {
    fprintf(stderr,"IRIS_File::loadIRIS() could not parse file %s\n", stFile.c_str());
    return 1;
  }
  //  printf("\n\ndone reading xml file. Now parsing the xml.\n");
  root_element = doc.FirstChildElement("page");
    assert(root_element);
  page = root_element;
  //while(page && (page->type != XML_ELEMENT_NODE))
  //  page = page->next;
  //CHECK_EXPECTED(page->name,"page");
  //get image name
  pstTmp = getPropVal(page,"raw-file");
  image=std::string(pstTmp);

  header = page->FirstChildElement("header");
  XMLElement* last_header = page->LastChildElement("header");
  //while(header && (header->type != XML_ELEMENT_NODE || 0!=strcmp((char*)header->name,"header")))
  //  header = header->next;
  if (header==0x0)
      return 1;
  //CHECK_EXPECTED(header->name,"header");

  // count how many records there are
  /*this->numRecords = 0;
  std::map<int,int> numFieldsPerRecord;
  header_item = header->children;
  while(header_item){
    if(header_item->type == XML_ELEMENT_NODE){
      if(0==strcmp((char*)header_item->name,"header-item")){
          pstTmp = getPropVal(header_item,"record")
            int recordNum=atoi(pstTmp);
            numFieldsPerRecord[recordNum]++;
	    if (recordNum>this->numRecords)
                this->numRecords=recordNum;
      }
      else{
	fprintf(stderr,"IRIS_File::load() possible xml parse error! "
		"(expected header-item node)\n");
	return 2;
      }
    }
    header_item = header_item->next;
  }
  this->numRecords++;
  for (int rec=0; rec<this->numRecords; rec++){
      this->rgRecords[rec].numFields=numFieldsPerRecord[rec];
      this->rgRecords[rec].rgFields=new IRIS_Field[numFieldsPerRecord[rec]];
  }
  //  printf("found %d records in xml\n", this->numRecords);

  this->rgRecords = new FSI_Record[this->numRecords];
  if(!(this->rgRecords)){
    fprintf(stderr,
	    "IRIS_File::load() memory allocation for %d records failed\n",
	    this->numRecords);
    return 3;
  }*/

  //now actually get the information out of the records
  std::map<int,std::string> givenNames, lastNames;
  std::map<int,bool> isHead;
  std::regex nonNum("[^\\d]");
  std::regex bracketed("<.*>");
  while (header)
  {
      header_item = header->FirstChildElement("header-item");
      XMLElement* last_header_item = header->LastChildElement("header-item");
      //recnum = 0;
      while(header_item)
      {
	//header_item id
	pstTmp = getPropVal(header_item,"record");
	if(NULL == pstTmp){
	  fprintf(stderr,
		  "possible xml parse error! (no header-item record specified)\n");
	  return 4;
	}
        int recordNum=atoi(pstTmp);
        if(recordNum>=0 && recordNum<50)
        { 
            pstTmp = getPropVal(header_item,"name");
            if(NULL == pstTmp){
              fprintf(stderr,
                      "possible xml parse error! (no header-item name specified)\n");
              return 4;
            }
            std::string fieldName=trim(std::string(pstTmp));
            assert(fieldName.length()>0);
            std::string fieldValue="";
            const char* text = header_item->GetText();
            if (text)
                fieldValue=trim(std::string(text));
            if (regex_match(fieldValue,bracketed))
                fieldValue="";
            else if (fieldValue.compare(" ")==0)
                fieldValue="";
            if (records.size()<=recordNum)
                records.resize(recordNum+1);
            if (find(fieldNameList.begin(),fieldNameList.end(),fieldName)!=fieldNameList.end())
            {

                //ensure initail
                if (fieldName.compare("PR_SEX")==0 || fieldName.compare("PR_RACE_OR_COLOR")==0)
                    fieldValue = fieldValue.substr(0,1);
                //we assume beyond 10 people don't put fractions in the age field
                else if (fieldName.compare("PR_MARITAL_STATUS")==0)
                {
                    assert(fieldValue.length()<=1);
                    if (fieldValue.compare("W")==0)
                        fieldValue="Wd";
                }
                else if (fieldName.compare("PR_AGE")==0)
                {
                    fieldValue = regex_replace(fieldValue,nonNum,"");
                    if ( fieldValue.length()>0 && stoi(fieldValue)<10)
                        fieldValue = "";
                }
                records[recordNum][fieldName]=fieldValue;
            }
            else if (fieldName.compare("PR_NAME_GN")==0)
            {
                givenNames[recordNum]=fieldValue;
            }
            else if (fieldName.compare("PR_NAME_SURN")==0)
            {
                lastNames[recordNum]=fieldValue;
            }
            else if (fieldName.compare("IMAGE_TYPE")==0)
            {
                if (fieldValue.compare("No Extractable Data Image")==0 ||
                    fieldValue.compare("Blank Image")==0)
                {
                    records.clear();
                    return 0;
                }
            }
            if (fieldName.compare("PR_RELATIONSHIP")==0)
            {
#ifdef MEXICO
                if (fieldValue.length()>0 || fieldValue[0]!=' ')
#else
                if (fieldValue.compare("Head") ==0 || fieldValue.compare("HEAD") ==0)
#endif
                    isHead[recordNum]=true;
            }
            
        }
        if (header_item==last_header_item)
            break;
        
        header_item = header_item->NextSiblingElement("header-item");
    }
    if (header==last_header)
        break;
    
    header = header->NextSiblingElement("header");

    // if(0==recnum%10000)
    //   printf("recnum=%d\n",recnum);
  }//while(header)
  std::string lastLastName="";
  
  for (int recordNum=0; recordNum<records.size(); recordNum++)
  {
#ifdef MEXICO        
        if (lastLastName.compare(lastNames[recordNum])==0 || (lastLastName.find_last_of(' ')!=std::string::npos && (lastLastName.substr(0,lastLastName.find_last_of(' ')).compare(lastNames[recordNum])==0 || lastLastName.substr(lastLastName.find_last_of(' ')+1).compare(lastNames[recordNum])==0)))
#else
        if (lastLastName.compare(lastNames[recordNum])==0)
#endif
        {
            if (givenNames[recordNum].length()>0 && (givenNames[recordNum].length()>1 || (givenNames[recordNum][0]!=' ') && givenNames[recordNum][0]!='\t'))
#ifdef MEXICO        
                records[recordNum]["PR_NAME"]=givenNames[recordNum]+" ["+lastNames[recordNum]+"]";
#else
                records[recordNum]["PR_NAME"]="- "+givenNames[recordNum];
#endif
            else
                records[recordNum]["PR_NAME"]="";

            if(isHead[recordNum])
            {
                //There is some oddity about this record, so we'll just not use this and the above offending record
                int rn=recordNum;
                do
                {
                    records[rn]["PR_NAME"]="";
                    rn--;
                } while (rn>=0 && lastLastName.compare(lastNames[rn])==0);
            }
        }
        else
        {
            if (givenNames[recordNum].length()>0 && (givenNames[recordNum].length()>1 || (givenNames[recordNum][0]!=' ') && givenNames[recordNum][0]!='\t'))
#ifdef MEXICO        
                records[recordNum]["PR_NAME"]=givenNames[recordNum] + " " +lastNames[recordNum];
#else
                records[recordNum]["PR_NAME"]=lastNames[recordNum] + " " +givenNames[recordNum];
#endif
            else
                records[recordNum]["PR_NAME"]=lastNames[recordNum];
            lastLastName=lastNames[recordNum];
        }
  }

  /*free the document */
  //xmlFreeDoc(doc);

  /*Free global variables that may have been allocated by the parser.*/
  //xmlCleanupParser();
  

  return 0;//successful load
}
