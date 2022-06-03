#ifndef DEM_TEST_ETH_H_
#define DEM_TEST_ETH_H_

#include <arpa/inet.h>
#include <errno.h>
#include <fcntl.h>
#include <netdb.h>
#include <netinet/in.h>
#include <netinet/ip_icmp.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#include <mutex>
#include <thread>

#define ICMP_DATA_LEN 20
#define ICMP_REQUEST_TIMEOUT_SEC 5
#define ICMP_REQUEST_TIMEOUT_US 0

#define LOGE(format, args...)  printf(format, ##args)

namespace dem {

enum ETH_STATUS {
  ETH_OK,
  ETH_SRC_UNREACH,
  ETH_DST_UNREACH,
  ETH_INVALID,
};

enum ETH_RESPONSE_CHECK {
  ETH_CHECK_ERROR = -1,
  ETH_CHECK_OK,
  ETH_CHECK_TIMEOUT,
  ETH_CHECK_CONTINUE
};

class EthModule {  // PRQA S 2659
 public:
  EthModule();
  //EthModule(const EthModule &) = default;
  //EthModule &operator=(const EthModule &) = default;
  ~EthModule() ;
  //bool update(unsigned char &id, std::vector<uint8_t> &data) override;  // NOLINT
  int eth_parse_icmp_response(const struct sockaddr_in *pstFromAddr, char *pRecvBuf,
      const int iLen, const uint64_t &time_start);
  uint16_t cal_icmp_chksum(void *pPacket, int iPktLen);
  int eth_create_icmp_echo_request(const uint16_t iPacketNum, const int iDataLen);
  int eth_check_network_to_server(const char *pIpAddr);

 private:
  unsigned short icmp_pkg_seq;
};

}  // namespace dem

#endif  // DEM_TEST_ETH_H_