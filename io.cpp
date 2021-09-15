//
// Created by vladim0105 on 15.09.2021.
//

#include <fstream>
#include <sstream>
#include "io.h"
std::vector<double> read_csv_column(const std::string& file, int col_idx){
    std::ifstream infile(file);
    std::vector<double> result;
    std::string line;
    // Skip first line
    std::getline(infile, line);
    while (std::getline(infile, line))
    {
        std::istringstream iss(line);

        int count = 0;
        while( iss.good() ) {
            std::string substr;
            getline(iss, substr, ',');
            if(count == col_idx){
                result.push_back(std::stod(substr));
            }
            count++;
        }
    }

    return result;
}