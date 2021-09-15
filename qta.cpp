#include "qta.h"
#include <algorithm>

std::map<double, double> make_cdf(std::vector<double> latencies){
    std::map<double, double> cdf;
    std::sort(latencies.begin(),  latencies.end());
    for (int i = 0; i < latencies.size(); ++i) {
        double latency = latencies.at(i);
        cdf[latency] = (i+1.0)/(double)latencies.size();
    }
    return cdf;
}