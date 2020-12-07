//
// Created by Deivid Ribeiro on 4/17/20.
//

#ifndef ALIGNMENT_CSVREADER_HPP
#define ALIGNMENT_CSVREADER_HPP

#include <iostream>
#include <fstream>
#include <vector>
#include <iterator>
#include <string>
#include <algorithm>
#include <boost/algorithm/string.hpp>

#include "spdlog/spdlog.h"

/*
 * A class to read data from a csv file.
 */
class CSVReader
{
    std::string fileName;
    std::string delimeter;

public:
    CSVReader(std::string filename, std::string delm = ",") :
            fileName(filename), delimeter(delm)
    { }

    // Function to fetch data from a CSV File
    std::vector<std::vector<std::string> > getData();
};


#endif //ALIGNMENT_CSVREADER_HPP
