#include <iostream>
#include <sstream>
#include "qta.h"
#include "io.h"
#include "CLI11.hpp"

int main(int argc, char **argv) {
    std::string address = "127.0.0.1";
    uint16_t port = 443;
    int n = 10;

    CLI::App app{"Description"}; // TODO
    app.add_option("address", address, "IPv4 address of a running TWAMP Server.")->required(true);
    app.add_option("-p, --port", port, "The port of the TWAMP Server. (Default: " + std::to_string(port) + ")");
    app.add_option("-n, --amount", n, "Number of samples to collect. (Default: " + std::to_string(n) + ")");
    CLI11_PARSE(app, argc, argv);

    std::ostringstream command;
    command << "twamp-light-client " << address << " ";
    command << "-n " << n;
    std::cout << command.str() << std::endl;
    std::string cmd_out = exec(command.str().c_str());

    auto result = read_csv_column(std::stringstream(cmd_out), 12);
    auto cdf = make_cdf(result);
    for (auto cdf_point: cdf) {
        std::cout << cdf_point.first << ", " << cdf_point.second << std::endl;
    }
    return 0;
}
