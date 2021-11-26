//
// Created by vladim0105 on 15.09.2021.
//

#ifndef QED_SAMPLER_QTA_H

#define QED_SAMPLER_QTA_H

#include <vector>
#include <map>
#include <algorithm>
#include <numeric>
#include <complex>
#include <json/json.h>
#include "tdigest.h"
#define MAXLATENCY 15000.0
struct Sample {
    double rtt = 0;
    double intd = 0;
    double fwd = 0;
    double swd = 0;
    double plen = 0;
};
std::map<double, double> make_cdf(std::vector<double>& rtts, uint32_t num_losses, std::vector<double> *percentiles = nullptr) {
    // Make a copy so that we can modify the array without messing up somewhere else
    std::vector<double> _rtts = std::vector<double>(rtts);
    std::map<double, double> cdf;
    // Add packet losses as MAX_LATENCY
    for(int i = 0; i < num_losses; i++){
        _rtts.push_back(MAXLATENCY);
    }
    std::sort(_rtts.begin(), _rtts.end(), [](double a, double b) {
        return a < b;
    });
    for (int i = 0; i < _rtts.size(); ++i) {
        // Make sure that we cap the latency at MAXLATENCY
        double latency = std::min(_rtts.at(i), MAXLATENCY);
        if (percentiles) {
            cdf[latency] = percentiles->at(i);
        } else {
            cdf[latency] = (i + 1.0) / (double) _rtts.size();
        }

    }
    return cdf;
}
std::map<double, double> make_cdf_t(std::vector<double>& rtts, uint32_t num_losses, std::vector<double> *percentiles = nullptr) {
    // Make a copy so that we can modify the array without messing up somewhere else
    std::vector<double> _rtts = std::vector<double>(rtts);
    std::map<double, double> cdf;
    // Add packet losses as MAX_LATENCY
    for(int i = 0; i < num_losses; i++){
        _rtts.push_back(MAXLATENCY);
    }
    auto td = td_new(100);
    // Feed the rtts to tdigest.
    for (int i = 0; i < _rtts.size(); ++i) {
        // Make sure that we cap the latency at MAXLATENCY
        double latency = std::min(_rtts.at(i), MAXLATENCY);
        td_add(td, latency, 1);
    }
    // Works weird if not calling "compress" implicitly.
    td_compress(td);
    // Add the fewer values to cdf output
    int td_len = td_centroid_count(td);
    double weights = 0;
    for(int i = 0; i < td_len; i++){
        weights+= td_centroids_weight_at(td, i);
        auto latency = td_centroids_mean_at(td, i);
        if (percentiles) {
            cdf[latency] = percentiles->at(i);
        } else {
            cdf[latency] = weights / td_size(td);
        }
    }
    return cdf;
}
/**
 * Bjorn Ivar's code ported to C++
 */
double area_part(std::pair<const double, double> cdf_point1, std::pair<const double, double> cdf_point2,
                 std::pair<const double, double> qta_point1, std::pair<const double, double> qta_point2) {
    double x_cdf1 = cdf_point1.first;
    double y_cdf1 = cdf_point1.second;
    double x_cdf2 = cdf_point2.first;
    double y_cdf2 = cdf_point2.second;
    double x_qta = qta_point1.first;
    double y_qta = qta_point1.second;
    double x_qta2 = qta_point2.first;

    // Find the line between cdf1 and cdf2
    if (x_cdf2 - x_cdf1 == 0) {
        return 0;
    }
    double a = (y_cdf2 - y_cdf1) / (x_cdf2 - x_cdf1);
    double b = y_cdf1 - (a * x_cdf1);
    // We now assume cdf1 and cdf2 are the relevant points for calculating the area under the QTA curve, then check for corner cases
    double x_1 = x_cdf1;
    double y_1 = y_cdf1;
    double x_2 = x_cdf2;
    double y_2 = y_cdf2;
    // Check if we need to move points (x_1, y_1) and (x_2, y_2) before calculating the area
    if (y_cdf1 > y_qta || x_cdf1 > x_qta2) {
        return 0;
    }
    if (x_cdf1 < x_qta) {
        // Find point of intersection between the line connecting cdf1 and cdf2, and the vertical at x_qta
        if (x_2 < x_qta) {
            return 0;
        }
        x_1 = x_qta;
        y_1 = a * x_qta + b;
        if (y_1 > y_qta) {
            return 0;
        }
    }
    if (y_cdf2 > y_qta) {
        // Find point of intersection between the line connecting cdf1 and cdf2, and the horizontal at y_qta
        x_2 = (y_qta - b) * (1 / a);
        y_2 = y_qta;
    }
    if (x_1 < x_qta2 && x_2 > x_qta2) {
        // Avoid overlap between areas calculated for points qta1 and qta2
        x_2 = x_qta2;
        y_2 = a * x_qta2 + b;
    }
    // Calculate area:
    double triangle = (y_2 - y_1) * (x_2 - x_1) * 0.5;
    double square = (y_qta - y_2) * (x_2 - x_1);
    return triangle + square;
}

double cdf_qta_overlap(std::map<double, double> cdf, std::map<double, double> qta) {
    // Bjorn Olav's code mentions that cdfs always have a max latency entry (in the old code),
    // so I am adding it here just in case it breaks the math or something...
    qta[MAXLATENCY] = 1.0;
    cdf[MAXLATENCY] = 1.0;
    double area = 0;
    auto qta_it = qta.begin();
    while (qta_it != --qta.end()) {
        //If you dereference an iterator of the map, you get a reference to the pair.
        auto qta_point1 = *qta_it;
        ++qta_it; // Get the next one too.
        auto qta_point2 = *qta_it;
        auto cdf_it = cdf.begin();
        while (cdf_it != --cdf.end()) {
            auto cdf_point1 = *cdf_it;
            ++cdf_it;
            auto cdf_point2 = *cdf_it;
            area += area_part(cdf_point1, cdf_point2, qta_point1, qta_point2);
        }
    }
    return area;
}

/**
 * A fancy map, each vector element is mapped to a payload size defined in payload_sizes.
 */
struct AnalysisResult {
    std::vector<double> payload_sizes = std::vector<double>();
    std::vector<double> minDelays = std::vector<double>();
    std::vector<double> maxDelays = std::vector<double>();
    std::map<double, std::vector<double>> delays = std::map<double, std::vector<double>>();
};
template<typename T>
struct LinearFitResult {
    T slope;
    T intercept;
};
struct Distribution {
    double mean = 0;
    double stdev = 0;
    double min = 0;
    double max = 0;
    double median = 0;
};
Distribution getV(AnalysisResult ar, double payload_size, LinearFitResult<double> fr) {
    Distribution V;
    auto it = std::find(ar.payload_sizes.begin(), ar.payload_sizes.end(), payload_size);
    auto index = std::distance(ar.payload_sizes.begin(), it);
    double slopeHeight = fr.intercept + fr.slope * payload_size;
    std::vector<double> residuals = std::vector<double>();
    for(auto delay : ar.delays[payload_size]){
        residuals.push_back(delay-slopeHeight);
    }
    // Mean https://stackoverflow.com/questions/7616511/calculate-mean-and-standard-deviation-from-a-vector-of-samples-in-c-using-boos
    double sum = std::accumulate(residuals.begin(), residuals.end(), 0.0);
    double mean = sum / (double)residuals.size();
    // St.Dev https://stackoverflow.com/questions/7616511/calculate-mean-and-standard-deviation-from-a-vector-of-samples-in-c-using-boos
    double sq_sum = std::inner_product(residuals.begin(), residuals.end(), residuals.begin(), 0.0);
    double stdev = std::sqrt(sq_sum / (double)residuals.size() - mean * mean);
    // Median https://stackoverflow.com/questions/1719070/what-is-the-right-approach-when-using-stl-container-for-median-calculation/1719155#1719155
    auto _tmp = std::vector<double>(residuals);
    size_t n = _tmp.size() / 2;
    nth_element(_tmp.begin(), _tmp.begin()+n, _tmp.end());
    double median = _tmp[n];

    V.max = ar.maxDelays.at(index)-slopeHeight;
    V.min = ar.minDelays.at(index)-slopeHeight;
    V.mean = mean;
    V.stdev = stdev;
    V.median = median;
    return V;
}
double getS(double payload_size, double G, LinearFitResult<double> fr) {
    double slopeHeight = fr.intercept + fr.slope * payload_size;
    double S = slopeHeight-G;
    return S;
}

AnalysisResult analyzeSamples(const std::vector<Sample> &samples, const std::vector<uint16_t> &payload_sizes) {
    AnalysisResult ar = AnalysisResult();
    std::map<double, double> minSamples = std::map<double, double>();
    std::map<double, double> maxSamples = std::map<double, double>();
    std::map<double, std::vector<double>> delays = std::map<double, std::vector<double>>();
    for (auto sample: samples) {
        if(delays[sample.plen].empty()){
            delays[sample.plen] = std::vector<double>();
        }
        delays[sample.plen].push_back(sample.rtt);
        // Find min
        if (minSamples.count(sample.plen)) {
            if (sample.rtt < minSamples[sample.plen]) {
                minSamples[sample.plen] = sample.rtt;
            }
        } else {

            minSamples[sample.plen] = sample.rtt;
        }
        // Find max
        if (maxSamples.count(sample.plen)) {
            if (sample.rtt > maxSamples[sample.plen]) {
                maxSamples[sample.plen] = sample.rtt;
            }
        } else {
            maxSamples[sample.plen] = sample.rtt;
        }
    }
    for (double payload_size : payload_sizes) {
        ar.payload_sizes.push_back(payload_size);
        ar.minDelays.push_back(minSamples[payload_size]);
        ar.maxDelays.push_back(maxSamples[payload_size]);
    }
    ar.delays = delays;
    return ar;
}

template<typename T>
LinearFitResult<T> GetLinearFit(const std::vector<T> &x, const std::vector<T> &y) {
    LinearFitResult<T> out = LinearFitResult<T>();
    T xSum = 0, ySum = 0, xxSum = 0, xySum = 0, slope, intercept;
    for (long i = 0; i < y.size(); i++) {
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
/**
 * Decomposition is based on: https://www.martingeddes.com/think-tank/network-performance-chemistry/
 */
void performDecomposition(const std::vector<Sample>& samples, const std::vector<uint16_t>& payloads, const std::vector<double> plens, Json::Value& root){
    auto ar = analyzeSamples(samples, payloads);
    auto fr = GetLinearFit(ar.payload_sizes, ar.minDelays);
    int position = 0;
    for (int i = 0; i < payloads.size(); i++) {
        if(std::find(plens.begin(), plens.end(), payloads.at(i)) == plens.end()){
            continue;
        }
        int payload_size = payloads.at(i);
        double G = fr.intercept;
        Distribution V = getV(ar, payload_size, fr);
        double S = getS(payload_size, G, fr);
        root["decomposition"]["values"][position]["payload_size"] = payload_size;
        root["decomposition"]["values"][position]["G"] = G;
        root["decomposition"]["values"][position]["V"]["mean"] = V.mean;
        root["decomposition"]["values"][position]["V"]["std"] = V.stdev;
        root["decomposition"]["values"][position]["V"]["median"] = V.median;
        root["decomposition"]["values"][position]["V"]["min"] = V.min;
        root["decomposition"]["values"][position]["V"]["max"] = V.max;
        root["decomposition"]["values"][position]["S"] = S;
        position++;
    }
    root["decomposition"]["slope"] = fr.slope;
    root["decomposition"]["intercept"] = fr.intercept;
}
#endif //QED_SAMPLER_QTA_H
