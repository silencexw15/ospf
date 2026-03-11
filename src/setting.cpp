#include "setting.h"
#include <arpa/inet.h> // contains <netinet/in.h>
#include <cstdlib>
#include <cstdio>

namespace myconfigs {
    
    const char* nic_name = "ens33";
    uint32_t interface_ip = ntohl(inet_addr("192.168.75.128"));
    uint32_t router_id = ntohl(inet_addr("192.168.75.128"));
    std::vector<Interface*> interfaces;
    std::map<uint32_t, Interface*> ip2interface;

    pthread_attr_t thread_attr;

    void initRuntimeConfig() {
        const char* nic_env = std::getenv("OSPF_NIC");
        if (nic_env != nullptr && nic_env[0] != '\0') {
            nic_name = nic_env;
        }

        const char* ip_env = std::getenv("OSPF_INTERFACE_IP");
        if (ip_env != nullptr && ip_env[0] != '\0') {
            in_addr_t parsed = inet_addr(ip_env);
            if (parsed == INADDR_NONE) {
                fprintf(stderr, "[config] invalid OSPF_INTERFACE_IP: %s\n", ip_env);
            } else {
                interface_ip = ntohl(parsed);
            }
        }

        const char* rid_env = std::getenv("OSPF_ROUTER_ID");
        if (rid_env != nullptr && rid_env[0] != '\0') {
            in_addr_t parsed = inet_addr(rid_env);
            if (parsed == INADDR_NONE) {
                fprintf(stderr, "[config] invalid OSPF_ROUTER_ID: %s\n", rid_env);
            } else {
                router_id = ntohl(parsed);
            }
        } else {
            router_id = interface_ip;
        }
    }
} // namespace Configs

struct in_addr ipaddr_tmp;
int lsa_seq_cnt = 0;
pthread_mutex_t lsa_seq_lock;

bool to_exit = false;
