//
// Created by vladim0105 on 15.09.2021.
//

#ifndef QED_SAMPLER_QTA_H

#define QED_SAMPLER_QTA_H

#include <vector>
#include <map>
struct Sample{
    double rtt = 0;
    double intd = 0;
    double fwd = 0;
    double swd = 0;
    double plen = 0;
};
std::map<double, double> make_cdf(std::vector<Sample>& samples);

#endif //QED_SAMPLER_QTA_H
