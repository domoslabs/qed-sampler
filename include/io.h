//
// Created by vladim0105 on 15.09.2021.
//

#ifndef QED_SAMPLER_IO_H
#define QED_SAMPLER_IO_H

#include <vector>
#include <string>
#include <fstream>
#include <json/value.h>
#include <json/reader.h>
#include "iterator"
#include "sstream"

std::string exec(const char *cmd);

template<class T>
std::string vectorToString(std::vector<T> vec, const char *sep = ",") {
    std::stringstream result;
    std::copy(vec.begin(), vec.end(), std::ostream_iterator<T>(result, sep));
    return result.str().substr(0, result.str().size() - 1);
}

template<typename T>
T string_to_number(const std::string &str) {
    std::istringstream ss(str);
    T num;
    ss >> num;
    return num;
}

template<class T>
std::vector<T> read_csv_column(std::stringstream data, int col_idx, bool skip_header = true, char delim = ',') {
    std::vector<T> result;
    std::string line;

    if (skip_header)
        std::getline(data, line); // Skip first line
    while (std::getline(data, line)) {
        std::istringstream iss(line);

        int count = 0;
        while (iss.good()) {
            std::string substr;
            getline(iss, substr, delim);
            if (count == col_idx) {
                result.push_back(string_to_number<T>(substr));
            }
            count++;
        }
    }
    return result;
}
std::vector<std::string> read_csv_column(std::stringstream data, int col_idx, bool skip_header = true, char delim = ',') {
    std::vector<std::string> result;
    std::string line;

    if (skip_header)
        std::getline(data, line); // Skip first line
    while (std::getline(data, line)) {
        std::istringstream iss(line);

        int count = 0;
        while (iss.good()) {
            std::string substr;
            getline(iss, substr, delim);
            if (count == col_idx) {
                result.push_back(substr);
            }
            count++;
        }
    }
    return result;
}
template<class T>
std::vector<T> fromJsonVector(const Json::Value &jsonVector) {
    std::vector<T> out;
    for (const auto &value: jsonVector) {
        out.push_back((T) value.asDouble());
    }
    return out;
}

Json::Value stringToJson(const std::string &s) {
    Json::Value root;
    Json::Reader reader;
    bool parsingSuccessful = reader.parse(s, root);
    if (!parsingSuccessful) {
        std::cerr << "Error parsing the string" << std::endl;
    }
    return root;
}

#endif //QED_SAMPLER_IO_H

/**
 * https://stackoverflow.com/questions/478898/how-do-i-execute-a-command-and-get-the-output-of-the-command-within-c-using-po
 * @param cmd Command to execute
 * @return Output of the command
 */
std::string exec(const char *cmd, bool verbose=false) {
    char buffer[128];
    std::string result;
    FILE *pipe = popen(cmd, "r");
    if (!pipe) throw std::runtime_error("popen() failed!");
    try {
        while (fgets(buffer, sizeof buffer, pipe) != nullptr) {
            if(verbose)
                std::cout << buffer <<std::flush;
            result += buffer;
        }
    } catch (...) {
        pclose(pipe);
        throw;
    }
    pclose(pipe);
    return result;
}

std::string read_file(const std::string &path) {
    std::ifstream t(path);
    if (t.fail()) {
        std::cerr << "Error reading file: " << path << std::endl;
        std::exit(EXIT_FAILURE);
    }
    std::stringstream buffer;
    buffer << t.rdbuf();
    return buffer.str();
}