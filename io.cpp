//
// Created by vladim0105 on 15.09.2021.
//

#include <fstream>
#include <sstream>
#include <array>
#include <memory>
#include <iostream>
#include <iterator>
#include "io.h"


/**
 * https://stackoverflow.com/questions/478898/how-do-i-execute-a-command-and-get-the-output-of-the-command-within-c-using-po
 * @param cmd Command to execute
 * @return Output of the command
 */
std::string exec(const char *cmd) {
    char buffer[128];
    std::string result;
    FILE *pipe = popen(cmd, "r");
    if (!pipe) throw std::runtime_error("popen() failed!");
    try {
        while (fgets(buffer, sizeof buffer, pipe) != nullptr) {
            result += buffer;
        }
    } catch (...) {
        pclose(pipe);
        throw;
    }
    pclose(pipe);
    return result;
}
