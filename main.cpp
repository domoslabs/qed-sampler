#include <iostream>
#include <sstream>
#include <fstream>
#include "qta.h"
#include "io.h"
int main() {
    std::string address = "127.0.0.1";
    uint16_t port = 443;
    int n = 50;

    std::ostringstream command;
    command << "twamp-light-client " << address << " ";
    command << "-f out.csv ";
    command << "-n " << n;
    std::cout << command.str() << std::endl;
    std::system(command.str().c_str());

    auto result = read_csv_column("out.csv", 12);
    auto t = make_cdf(result);
    for(auto x : t){
        std::cout << x.first << ", " << x.second << std::endl;
    }
    return 0;
}
