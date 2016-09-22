
#ifndef IRIS_FILE_H
#define IRID_FILE_H
#include <map>
#include <vector>
#include <string>
#include <fstream>
#include <iostream>
#include <algorithm>
// trim from start (in place)
static inline void ltrim(std::string &s) {
    s.erase(s.begin(), std::find_if(s.begin(), s.end(),
            std::not1(std::ptr_fun<int, int>(std::isspace))));
}

// trim from end (in place)
static inline void rtrim(std::string &s) {
    s.erase(std::find_if(s.rbegin(), s.rend(),
            std::not1(std::ptr_fun<int, int>(std::isspace))).base(), s.end());
}
// trim from both ends (copying)
static inline std::string trim(std::string s) {
    rtrim(s);
    ltrim(s);
    return s;
}
/*class Field {
public:
    std::string fieldValue;
    std::string fieldName;
    std::string fieldType;
};*/

class IRIS_File {

public:

    IRIS_File();
    int numRecords() const {return records.size();}
    int numFields() const {return fieldNameList.size();}
    std::string strImageFile() const {return image;}
    std::vector<std::string> fieldNames() const {return fieldNameList;}
    const std::map< std::string, std::string >& record(int recordNum) const {return records[recordNum];}
    int load(std::string stFile);

private:
    std::vector< std::map<std::string,std::string> > records;
    std::vector< std::string > fieldNameList;
    std::string image;
};

#endif
