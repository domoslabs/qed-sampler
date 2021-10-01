#include <iostream>
#include "qta.h"
#include "io.h"
#include "CLI11.hpp"

int main(int argc, char **argv) {
    std::string address = "127.0.0.1";
    uint16_t port = 443;
    int n = 10;
    std::vector<uint16_t> payloads = std::vector<uint16_t>();
    payloads.push_back(100);
    payloads.push_back(150);

    CLI::App app{"Description"}; // TODO
    app.option_defaults()->always_capture_default(true);
    app.add_option("address", address, "IPv4 address of a running TWAMP Server.")->required(true);
    app.add_option("-p, --port", port, "The port of the TWAMP Server.");
    app.add_option("-n, --amount", n, "Number of samples to collect.");
    app.add_option<std::vector<uint16_t>>("-P, --payloads", payloads, "Payload size(s) defined in bytes.")->default_str(vectorToString(payloads));
    CLI11_PARSE(app, argc, argv);

    std::stringstream command;
    command << "twamp-light-client " << address << " ";
    command << "-n " << n << " ";
    command << "-l " << vectorToString(payloads, " ");
    std::cout << vectorToString(payloads) << std::endl;
    std::cout << command.str() << std::endl;
    std::string cmd_out = exec(command.str().c_str());

    auto rtts = read_csv_column<double>(std::stringstream(cmd_out), 12);
    auto intds = read_csv_column<double>(std::stringstream(cmd_out), 13);
    auto fwds = read_csv_column<double>(std::stringstream(cmd_out), 14);
    auto swds = read_csv_column<double>(std::stringstream(cmd_out), 15);
    auto plens = read_csv_column<int>(std::stringstream(cmd_out), 16);

    std::vector<Sample> samples = std::vector<Sample>();
    for(int i = 0; i < rtts.size(); i++){
        Sample sample;
        sample.rtt = rtts.at(i);
        sample.intd = intds.at(i);
        sample.fwd = fwds.at(i);
        sample.swd = swds.at(i);
        sample.plen = plens.at(i);
        samples.push_back(sample);
    }
    std::sort(samples.begin(), samples.end(), [](Sample a, Sample b){
        return a.rtt > b.rtt;
    });
    auto cdf = make_cdf(samples);
    for (auto cdf_point: cdf) {
        std::cout << cdf_point.first << ", " << cdf_point.second << std::endl;
    }
    return 0;
}
