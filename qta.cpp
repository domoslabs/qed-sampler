#include "qta.h"
#include <algorithm>

std::map<double, double> make_cdf(std::vector<Sample>& samples) {
    std::map<double, double> cdf;
    std::sort(samples.begin(), samples.end(), [](Sample a, Sample b){
        return a.rtt < b.rtt;
    });
    for (int i = 0; i < samples.size(); ++i) {
        double latency = samples.at(i).rtt;
        cdf[latency] = (i + 1.0) / (double) samples.size();
    }
    return cdf;
}
