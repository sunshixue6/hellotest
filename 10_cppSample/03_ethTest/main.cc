#include "ethTest.h"
#include <iostream>

int main(int argc, char **argv) {

    int ret = 0;
    std::shared_ptr<dem::EthModule> eth_instance1 = std::make_shared<dem::EthModule>();

    std::string ip_addr = "192.168.101.50";

    ret = eth_instance1->eth_check_network_to_server(ip_addr.c_str());
    std::cout << "eth_check_network_to_servercheck " << ip_addr <<"result is:"  << ret << std::endl;

    std::string ip_addr2 = "127.0.0.1";

    ret = eth_instance1->eth_check_network_to_server(ip_addr2.c_str());
    std::cout << "eth_check_network_to_servercheck "<< ip_addr2 <<"result is:"  << ret << std::endl;

    std::string ip_addr3 = "172.31.30.32";

    ret = eth_instance1->eth_check_network_to_server(ip_addr3.c_str());
    std::cout << "eth_check_network_to_servercheck "<< ip_addr3 <<"result is:"  << ret << std::endl;


    return 0;
}
