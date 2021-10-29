#include <iostream>
#include "qta.h"
#include "io.h"
#include "CLI11.hpp"
#include "json/json.h"

/**
 * Decomposition is based on: https://www.martingeddes.com/think-tank/network-performance-chemistry/
 */
 void performDecomposition(const std::vector<Sample>& samples, const std::vector<uint16_t>& payloads, Json::Value& root){
    auto ar = analyzeSamples(samples, payloads);
    auto fr = GetLinearFit(ar.payload_sizes, ar.minDelays);

    for (int i = 0; i < payloads.size(); i++) {
        int payload_size = payloads.at(i);
        double G = fr.intercept;
        Distribution V = getV(ar, payload_size, fr);
        double S = getS(payload_size, G, fr);
        root["decomposition"]["values"][i]["payload_size"] = payload_size;
        root["decomposition"]["values"][i]["G"] = G;
        root["decomposition"]["values"][i]["V"]["mean"] = V.mean;
        root["decomposition"]["values"][i]["V"]["std"] = V.stdev;
        root["decomposition"]["values"][i]["V"]["median"] = V.median;
        root["decomposition"]["values"][i]["V"]["min"] = V.min;
        root["decomposition"]["values"][i]["V"]["max"] = V.max;
        root["decomposition"]["values"][i]["S"] = S;
    }
    root["decomposition"]["slope"] = fr.slope;
    root["decomposition"]["intercept"] = fr.intercept;
 }
int main(int argc, char **argv) {
    std::string address = "127.0.0.1";
    std::string qta_path, cdf_path;
    uint16_t port = 443;
    uint16_t precision = 8;
    uint16_t delay = 100;
    uint16_t localPort = 443;
    uint32_t n = 20;
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
    app.get_formatter()->column_width(75);
    app.option_defaults()->always_capture_default(true);
    auto opt_address = app.add_option("address", address, "IPv4 address of a running TWAMP Server.");
    auto opt_port = app.add_option("-p, --port", port, "The port of the TWAMP Server.");
    auto opt_amount = app.add_option("-n, --amount", n, "Number of samples to collect.");
    app.add_option("-d, --delay", delay, "Delay between samples (ms)");
    app.add_option("-P, --local_port", localPort, "Local port to run on.");
    auto opt_payloads = app.add_option<std::vector<uint16_t>>("-l, --payloads", payloads,
                                                              "Payload size(s) defined in bytes.")->default_str(
            vectorToString(payloads));
    auto opt_qta = app.add_option("--qta", qta_path,
                                  "Calculate the CDF-QTA overlap, requires path to a QTA-file. Expects a JSON with the same \"CDF\"-item as the output of this program.");
    app.add_option("--cdf", cdf_path,
                   "Use a pre-obtained CDF instead of sampling. Expects a JSON with the same \"CDF\"-item as the output of this program. Disables decomposition. Needs a QTA file.")->excludes(
            opt_address, opt_amount, opt_port, opt_payloads)->needs(opt_qta);
    app.add_option("--precision", precision, "Numerical precision in the output.")->check(CLI::Range((uint16_t)2, (uint16_t)16));
    auto opt_verbose = app.add_flag("-v, --verbose", "Enable verbose output.");
    auto opt_comments = app.add_flag("--comments", "Enable comments in the json output.");

    CLI11_PARSE(app, argc, argv);
    Json::Value root;
    std::vector<double> rtts;
    uint32_t num_losses = 0;
    std::map<double, double> cdf;
    if (cdf_path.empty()) {
        std::stringstream command;
        if(*opt_verbose)
            command << "stdbuf --output=L "; // To force the twamp output to be line-buffered
        command << "twamp-light-client " << address << " ";
        command << "-n " << n << " ";
        command << "-p " << std::to_string(port) << " ";
        command << "-d " << std::to_string(delay) << " ";
        command << "-P " << std::to_string(localPort) << " ";
        command << "-l " << vectorToString(payloads, " ");
        if(*opt_verbose)
            std::cout << command.str() << std::endl;
        std::string cmd_out = exec(command.str().c_str(), (bool)*opt_verbose);
        if(*opt_verbose)
            std::cout << cmd_out << std::endl;
        rtts = read_csv_column<double>(std::stringstream(cmd_out), 12);
        auto intds = read_csv_column<double>(std::stringstream(cmd_out), 13);
        auto fwds = read_csv_column<double>(std::stringstream(cmd_out), 14);
        auto swds = read_csv_column<double>(std::stringstream(cmd_out), 15);
        auto plens = read_csv_column<double>(std::stringstream(cmd_out), 16);
        // We only look at the last line of the losses column
        num_losses = *read_csv_column<uint32_t>(std::stringstream(cmd_out), 17).end();
        root["loss_pct"] = (double)num_losses/(double)rtts.size();
        std::vector<Sample> samples = std::vector<Sample>();
        for (int i = 0; i < rtts.size(); i++) {
            Sample sample;
            sample.rtt = rtts.at(i);
            sample.intd = intds.at(i);
            sample.fwd = fwds.at(i);
            sample.swd = swds.at(i);
            sample.plen = plens.at(i);
            samples.push_back(sample);
        }
        performDecomposition(samples, payloads, root);
        cdf = make_cdf(rtts, num_losses);
    } else {
        auto cdf_data = read_file(cdf_path);
        Json::Value cdf_json_root = stringToJson(cdf_data);
        auto cdf_rtts = fromJsonVector<double>(cdf_json_root["CDF"]["latencies"]);
        auto cdf_percentiles = fromJsonVector<double>(cdf_json_root["CDF"]["percentiles"]);
        cdf = make_cdf(cdf_rtts, num_losses, &cdf_percentiles);
    }

    auto json_latency_arr = Json::Value(Json::arrayValue);
    auto json_percentile_arr = Json::Value(Json::arrayValue);
    for (auto cdf_point: cdf) {
        json_latency_arr.append(cdf_point.first);
        json_percentile_arr.append(cdf_point.second);
    }
    root["CDF"]["latencies"] = json_latency_arr;
    root["CDF"]["percentiles"] = json_percentile_arr;
    if (!qta_path.empty()) {
        auto qta_data = read_file(qta_path);
        Json::Value qta_json_root = stringToJson(qta_data);
        auto qta_rtts = fromJsonVector<double>(qta_json_root["CDF"]["latencies"]);
        auto qta_percentiles = fromJsonVector<double>(qta_json_root["CDF"]["percentiles"]);
        // Assuming that the packet losses are already incorporated into the qta rtts
        auto qta = make_cdf(qta_rtts, 0, &qta_percentiles);

        root["cdf_qta_overlap"] = cdf_qta_overlap(cdf, qta);
    }

    if(*opt_comments){
        root["CDF"].setComment(std::string("// Normalized CDF consisting of latency-percentile pairs. When plotting, latency is x and percentile [0, 1] is y."), Json::commentBefore);
        root["cdf_qta_overlap"].setComment(std::string("// The area overlap between the CDF and the provided QTA. >0 if there is an overlap."), Json::commentBefore);
        root["decomposition"]["intercept"].setComment(std::string("// The intercept of the slope found by linear regression through the min value of each packet size."), Json::commentBefore);
        root["decomposition"]["slope"].setComment(std::string("// The slope found by linear regression through the min value of each packet size."), Json::commentBefore);
        root["decomposition"]["values"].setComment(std::string("//Array of G(eographic)-, S(erialization)-, and V(ariable contention)-delays for each packet size. V is a distribution."), Json::commentBefore);
    }

    //------------------ Output ---------------
    std::stringstream outs;
    Json::StreamWriterBuilder wbuilder;
    wbuilder.settings_["precision"] = precision;
    std::unique_ptr<Json::StreamWriter> writer(wbuilder.newStreamWriter());
    writer->write(root, &outs);
    std::cout << outs.str() << std::endl;


    return 0;
}
