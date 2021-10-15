#include <iostream>
#include "qta.h"
#include "io.h"
#include "CLI11.hpp"
#include "json/json.h"
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
double area_part(std::pair<const double, double> cdf_point1,std::pair<const double, double> cdf_point2, std::pair<const double, double> qta_point1, std::pair<const double, double> qta_point2){
    double x_cdf1 = cdf_point1.first;
    double y_cdf1 = cdf_point1.second;
    double x_cdf2 = cdf_point2.first;
    double y_cdf2 = cdf_point2.second;
    double x_qta = qta_point1.first;
    double y_qta = qta_point1.second;
    double x_qta2 = qta_point2.first;

    // Find the line between cdf1 and cdf2
    if(x_cdf2 - x_cdf1 == 0){
        return 0;
    }
    double a = (y_cdf2-y_cdf1)/(x_cdf2-x_cdf1);
    double b = y_cdf1 - (a*x_cdf1);
    // We now assume cdf1 and cdf2 are the relevant points for calculating the area under the QTA curve, then check for corner cases
    double x_1 = x_cdf1;
    double y_1 = y_cdf1;
    double x_2 = x_cdf2;
    double y_2 = y_cdf2;
    // Check if we need to move points (x_1, y_1) and (x_2, y_2) before calculating the area
    if (y_cdf1 > y_qta || x_cdf1 > x_qta2){
        return 0;
    }
    if (x_cdf1 < x_qta){
        // Find point of intersection between the line connecting cdf1 and cdf2, and the vertical at x_qta
        if (x_2 < x_qta) {
            return  0;
        }
        x_1 = x_qta;
        y_1 = a*x_qta+b;
        if (y_1 > y_qta){
            return 0;
        }
    }
    if (y_cdf2 > y_qta){
        // Find point of intersection between the line connecting cdf1 and cdf2, and the horizontal at y_qta
        x_2 = (y_qta-b)*(1/a);
        y_2 = y_qta;
    }
    if (x_1 < x_qta2 && x_2 > x_qta2){
        // Avoid overlap between areas calculated for points qta1 and qta2
        x_2 = x_qta2;
        y_2 = a*x_qta2+b;
    }
    // Calculate area:
    double triangle = (y_2 - y_1) * (x_2 - x_1) * 0.5;
    double square = (y_qta - y_2) * (x_2 - x_1);
    return triangle + square;
}
double cdf_qta_overlap(std::map<double, double> cdf, std::map<double, double> qta){
    double area = 0;
    for(auto qta_it = qta.begin(); qta_it != qta.end(); ++qta_it){
        //If you dereference an iterator of the map, you get a reference to the pair.
        auto qta_point1 = *qta_it;
        ++qta_it; // Get the next one too.
        auto qta_point2 = *qta_it;
        for(auto cdf_it = cdf.begin(); cdf_it != cdf.end(); ++cdf_it){
            auto cdf_point1 = *cdf_it;
            ++cdf_it;
            auto cdf_point2 = *cdf_it;
            area+= area_part(cdf_point1, cdf_point2, qta_point1, qta_point2);
        }
    }
    return area;
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
    int n = 100;
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
    command << "-P " << std::to_string(2500) << " ";
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
    auto fr = GetLinearFit(ar.payload_sizes, ar.minDelays);
    Json::Value root;
    for(int payload_size : payloads) {
        double G = fr.intercept;
        double V = getV(ar, payload_size, fr);
        double S = getS(ar, payload_size, G, V);
        root["GVS"][std::to_string(payload_size)]["G"]=G;
        root["GVS"][std::to_string(payload_size)]["V"]=V;
        root["GVS"][std::to_string(payload_size)]["S"]=S;
    }
    root["slope"]=fr.slope;
    root["intercept"]=fr.intercept;
    auto cdf = make_cdf(samples);
    for (auto cdf_point: cdf) {
        root["CDF"][std::to_string(cdf_point.first)] = cdf_point.second;
    }
    root["cdf_qta_overlap"] = cdf_qta_overlap(cdf, cdf);
    std::cout << root << std::endl;
    return 0;
}
