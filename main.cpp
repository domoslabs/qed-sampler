#include <iostream>
#include "qta.h"
#include "io.h"
#include "CLI11.hpp"
/**
 * A fancy map, each vector element is mapped to a payload size defined in payload_sizes.
 */
struct AnalysisResult{
    std::vector<double> payload_sizes =  std::vector<double>();
    std::vector<double> minDelays =  std::vector<double>();
    std::vector<double> maxDelays =  std::vector<double>();
};
template <typename T>
struct LinearFitResult{
    T slope;
    T intercept;
};
double getV(AnalysisResult ar, double payload_size, LinearFitResult<double> fr){
    auto it= std::find(ar.payload_sizes.begin(), ar.payload_sizes.end(), payload_size);
    auto index = std::distance(ar.payload_sizes.begin(), it);
    double slopeHeight = fr.intercept+fr.slope*payload_size;
    double V = ar.maxDelays.at(index)-slopeHeight;
    return V;
}
double getS(AnalysisResult ar, double payload_size, double G, double V){
    auto it= std::find(ar.payload_sizes.begin(), ar.payload_sizes.end(), payload_size);
    auto index = std::distance(ar.payload_sizes.begin(), it);
    double S = ar.maxDelays.at(index)-G-V;
    return S;
}
AnalysisResult analyzeSamples(const std::vector<Sample>& samples, const std::vector<uint16_t>& payload_sizes){
    AnalysisResult ar = AnalysisResult();
    std::map<double, double> minSamples = std::map<double, double>();
    std::map<double, double> maxSamples = std::map<double, double>();
    for (auto sample: samples) {
        // Find min
        if(minSamples.count(sample.plen)){
            if(sample.rtt < minSamples[sample.plen]){
                minSamples[sample.plen] = sample.rtt;
            }
        } else {

            minSamples[sample.plen] = sample.rtt;
        }
        // Find max
        if(maxSamples.count(sample.plen)){
            if(sample.rtt > maxSamples[sample.plen]){
                maxSamples[sample.plen] = sample.rtt;
            }
        } else {
            maxSamples[sample.plen] = sample.rtt;
        }
    }
    for(double payload_size : payload_sizes) {
        ar.payload_sizes.push_back(payload_size);
        ar.minDelays.push_back(minSamples[payload_size]);
        ar.maxDelays.push_back(maxSamples[payload_size]);
    }
    return ar;
}

template <typename T>
LinearFitResult<T> GetLinearFit(const std::vector<T>& x, const std::vector<T>& y)
{
    LinearFitResult<T> out = LinearFitResult<T>();
    T xSum = 0, ySum = 0, xxSum = 0, xySum = 0, slope, intercept;
    for (long i = 0; i < y.size(); i++)
    {
        xSum += x[i];
        ySum += y[i];
        xxSum += x[i] * x[i];
        xySum += x[i] * y[i];
    }
    slope = (y.size() * xySum - xSum * ySum) / (y.size() * xxSum - xSum * xSum);
    intercept = (ySum - slope * xSum) / y.size();
    out.slope = slope;
    out.intercept = intercept;
    return out;
}
int main(int argc, char **argv) {
    std::string address = "127.0.0.1";
    uint16_t port = 443;
    int n = 50;
    std::vector<uint16_t> payloads = std::vector<uint16_t>();
    payloads.push_back(50);
    payloads.push_back(200);
    payloads.push_back(400);
    payloads.push_back(600);
    payloads.push_back(800);
    payloads.push_back(1000);
    payloads.push_back(1200);
    payloads.push_back(1400);

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
    command << "-p " << std::to_string(port) << " ";
    command << "-d " << std::to_string(100) << " ";
    command << "-l " << vectorToString(payloads, " ");
    std::cout << vectorToString(payloads) << std::endl;
    std::cout << command.str() << std::endl;
    std::string cmd_out = exec(command.str().c_str());

    auto rtts = read_csv_column<double>(std::stringstream(cmd_out), 12);
    auto intds = read_csv_column<double>(std::stringstream(cmd_out), 13);
    auto fwds = read_csv_column<double>(std::stringstream(cmd_out), 14);
    auto swds = read_csv_column<double>(std::stringstream(cmd_out), 15);
    auto plens = read_csv_column<double>(std::stringstream(cmd_out), 16);

    std::vector<Sample> samples = std::vector<Sample>();
    for(int i = 0; i < rtts.size(); i++){
        Sample sample;
        sample.rtt = rtts.at(i);
        sample.intd = intds.at(i);
        sample.fwd = fwds.at(i);
        sample.swd = swds.at(i);
        sample.plen = plens.at(i);
        std::cout << sample.rtt << "," << sample.plen << std::endl;
        samples.push_back(sample);
    }
    auto ar = analyzeSamples(samples, payloads);
    std::cout << "-------------------------------"<< std::endl;
    for(int i = 0; i < payloads.size(); i++){
        std::cout << ar.payload_sizes[i] << "," << ar.minDelays[i] << std::endl;
    }
    std::cout << "-------------------------------"<< std::endl;
    auto fr = GetLinearFit(ar.payload_sizes, ar.minDelays);
    for(int payload_size : payloads) {
        double G = fr.intercept;
        double V = getV(ar, payload_size, fr);
        double S = getS(ar, payload_size, G, V);
        std::cout << payload_size << ":"<< G << "," << V << "," << S << std::endl;
    }

    std::cout<<std::to_string(fr.slope) << "," << std::to_string(fr.intercept) << std::endl;
    auto cdf = make_cdf(samples);
    for (auto cdf_point: cdf) {
        //std::cout << cdf_point.first << ", " << cdf_point.second << std::endl;
    }

    return 0;
}
