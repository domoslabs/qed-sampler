# QED Sampler
## Introduction
QED Sampler is a utility tool for gathering information about network quality using TWAMP-Light with varying payload sizes. It focuses on two areas:
#### QED
It constructs a CDF detailing the network quality and can optionally calculate the overlap between the CDF and a provided QTA.
#### Network Decomposition
It can also use the measurements to decompose the latency into G, S, and V. 
#### Output
The output is placed in stdout and is json-formatted. Can be redirected to a file using `>`.

See `test_cdf.json` and `test_qta.json` for more information about the output.
## Building and Installing
Requires t-digest to be installed: https://github.com/domoslabs/t-digest
```shell 
mkdir build
cd build
cmake ..
make
make install
```