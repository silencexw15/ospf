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

        struct in_addr nic_ip_addr;
        struct in_addr rid_addr;
        char ipbuf[INET_ADDRSTRLEN] = {0};
        char ridbuf[INET_ADDRSTRLEN] = {0};
        nic_ip_addr.s_addr = htonl(interface_ip);
        rid_addr.s_addr = htonl(router_id);
        inet_ntop(AF_INET, &nic_ip_addr, ipbuf, sizeof(ipbuf));
        inet_ntop(AF_INET, &rid_addr, ridbuf, sizeof(ridbuf));
        printf("[config] nic=%s interface_ip=%s router_id=%s\n",
            nic_name, ipbuf, ridbuf);
    }
} // namespace Configs

struct in_addr ipaddr_tmp;
int lsa_seq_cnt = 0;
pthread_mutex_t lsa_seq_lock;

bool to_exit = false;
