//
// Created by vladim0105 on 15.09.2021.
//

#ifndef QED_SAMPLER_IO_H
#define QED_SAMPLER_IO_H

#include <vector>
#include <string>

std::vector<double> read_csv_column(const std::string& file, int col_idx);
#endif //QED_SAMPLER_IO_H
