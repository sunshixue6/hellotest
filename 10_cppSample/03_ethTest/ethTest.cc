#include "ethTest.h"

namespace dem {

static uint8_t send_buf[1024] = {0U};
static uint8_t recv_buf[1024] = {0U};

EthModule::EthModule() {  // PRQA S 4054

  icmp_pkg_seq = 1;
}

EthModule::~EthModule() { }


uint16_t EthModule::cal_icmp_chksum(void *pPacket, int iPktLen) {
  uint16_t chkSum = 0U;
  uint16_t *pOffset = NULL;

  pOffset = reinterpret_cast<uint16_t *>(pPacket);

  while (1 < iPktLen) {
    chkSum += *pOffset++;  // PRQA S 3361, 3700
    iPktLen -= static_cast<int>(sizeof(uint16_t));
  }

  if (1 == iPktLen) {
    chkSum += *(reinterpret_cast<char *>(pOffset));  // PRQA S 3711
  }

  chkSum = (chkSum >> 16U) + (chkSum & 0xffffU);  // PRQA S 3323
  chkSum += (chkSum >> 16U);                      // PRQA S 3323

  return ~chkSum;
}

int EthModule::eth_create_icmp_echo_request(const uint16_t iPacketNum, const int iDataLen) {
  struct icmp *pstIcmp = NULL;

  memset(static_cast<uint8_t *>(send_buf), 0, sizeof(send_buf));
  pstIcmp = (struct icmp *)send_buf;
  pstIcmp->icmp_type = static_cast<uint8_t>(ICMP_ECHO);
  pstIcmp->icmp_code = 0;
  pstIcmp->icmp_seq = htons(iPacketNum);
  pstIcmp->icmp_id = htons((uint16_t)getpid());

  pstIcmp->icmp_cksum = 0U;
  pstIcmp->icmp_cksum = cal_icmp_chksum(pstIcmp, iDataLen + 8);

  return iDataLen + 8;
}

int EthModule::eth_parse_icmp_response(const struct sockaddr_in *pstFromAddr, char *pRecvBuf, const int iLen,
    const uint64_t &time_start) {
  int iIpHeadLen = 0;
  int iIcmpLen = 0;
  struct ip *pstIp = NULL;
  struct icmp *pstIcmpReply = NULL;
  uint64_t cur_time;

  if ((pstFromAddr == NULL) || (pRecvBuf == NULL) || (iLen == 0)) {
    return ETH_CHECK_ERROR;
  }

  pstIp = (struct ip *)pRecvBuf;
  iIpHeadLen = pstIp->ip_hl << 2;
  pstIcmpReply = (struct icmp *)(pRecvBuf + iIpHeadLen);
  iIcmpLen = iLen - iIpHeadLen;
  if (8 > iIcmpLen) {
    return ETH_CHECK_ERROR;
  }

  printf("pstIcmpReply->icmp_id = %d\n",pstIcmpReply->icmp_id);

  if (pstIcmpReply->icmp_type == static_cast<uint8_t>(ICMP_ECHOREPLY)) {
    return ETH_CHECK_OK;
  }

  cur_time = std::chrono::duration_cast<std::chrono::milliseconds>(
      std::chrono::steady_clock::now().time_since_epoch()).count();
  if ((cur_time - time_start) > 1000 * ICMP_REQUEST_TIMEOUT_SEC)
    return ETH_CHECK_TIMEOUT;

  if (pstIcmpReply->icmp_id != htons((uint16_t)getpid())) {
    return ETH_CHECK_CONTINUE;
  }

  return ETH_CHECK_CONTINUE;
}

int EthModule::eth_check_network_to_server(const char *pIpAddr) {
  int ret;
  int sockfd;
  int len;
  int ret_val = -1;
  struct sockaddr_in destAddr;
  struct sockaddr_in stFromAddr;
  socklen_t fromLen = sizeof(struct sockaddr_in);
  uint64_t start_time;

  if (pIpAddr == NULL) {
    LOGE("%s(), arg == NULL\n", __func__);
    return -1;
  }

  memset(&destAddr, 0, sizeof(struct sockaddr_in));
  ret = inet_aton(pIpAddr, &destAddr.sin_addr);
  destAddr.sin_family = AF_INET;
  if (ret == 0) {
    LOGE("%s(), inet_aton(), error: %s\n", __func__, strerror(errno));
    return -1;
  }

  sockfd = socket(PF_INET, SOCK_RAW, IPPROTO_ICMP);
  if (0 > sockfd) {
    LOGE("%s(), create raw socket(), error: %s\n", __func__, strerror(errno));
    return -1;
  }

  len = eth_create_icmp_echo_request(icmp_pkg_seq++, ICMP_DATA_LEN);
  if (icmp_pkg_seq >= 0xFFFF)
    icmp_pkg_seq = 1;

   printf("addr = %s\n",pIpAddr);

  start_time = std::chrono::duration_cast<std::chrono::milliseconds>(
      std::chrono::steady_clock::now().time_since_epoch()).count();
  ret = sendto(sockfd, static_cast<uint8_t *>(send_buf), static_cast<size_t>(len), 0, (struct sockaddr *)&destAddr,
               fromLen);
  if (ret <= 0) {  
    LOGE("%s(), sendto() error: %s\n", __func__, strerror(errno));
    close(sockfd);
    return -1;
  }

  while (1) {
      fd_set readset;
      FD_ZERO(&readset);
      FD_SET(sockfd, &readset);

      struct timeval tv;
      tv.tv_sec = ICMP_REQUEST_TIMEOUT_SEC;
      tv.tv_usec = ICMP_REQUEST_TIMEOUT_US;

      int ret = select(sockfd+1, &readset, NULL, NULL, &tv);
      if (ret == -1) {
        close(sockfd);
        return -1;
      } else if (ret == 0) {
        close(sockfd);
        return -1;
      } else {
          printf("revieve %s response!\n", pIpAddr);
          //break;

    #if 1    
        memset(static_cast<uint8_t *>(recv_buf), 0, 1024U);
        len = recvfrom(sockfd, reinterpret_cast<void *>(recv_buf), sizeof(recv_buf), 0,
                       reinterpret_cast<struct sockaddr *>(&stFromAddr), &fromLen);
        ret = eth_parse_icmp_response(&stFromAddr, reinterpret_cast<char *>(recv_buf), len, start_time);
        if (ret == ETH_CHECK_OK) {
          ret_val = 0;
          break;
        } else if (ret == ETH_CHECK_TIMEOUT) {
          ret_val = -1;
          break;
        } else if (ret == ETH_CHECK_ERROR) {
          ret_val = -2;
          break;
        }
 #endif       
      }
  }

  close(sockfd);
  return ret_val;
}

}  // namespace dem