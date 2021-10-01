//
// Created by vladim0105 on 15.09.2021.
//

#ifndef QED_SAMPLER_IO_H
#define QED_SAMPLER_IO_H

#include <vector>
#include <string>
#include "iterator"
#include "sstream"
std::string exec(const char *cmd);
template <class T>
std::string vectorToString(std::vector<T> vec, const char* sep=","){
    std::stringstream result;
    std::copy(vec.begin(), vec.end(), std::ostream_iterator<T>(result, sep));
    return result.str().substr(0, result.str().size()-1);
}
template <typename T> T string_to_number(const std::string &str)
{
    std::istringstream ss(str);
    T num;
    ss >> num;
    return num;
}
template <class T>
std::vector<T> read_csv_column(std::stringstream data, int col_idx) {
    std::vector<T> result;
    std::string line;
    // Skip first line
    std::getline(data, line);
    while (std::getline(data, line)) {
        std::istringstream iss(line);

        int count = 0;
        while (iss.good()) {
            std::string substr;
            getline(iss, substr, ',');
            if (count == col_idx) {
                result.push_back(string_to_number<T>(substr));
            }
            count++;
        }
    }

    return result;
}
#endif //QED_SAMPLER_IO_H
