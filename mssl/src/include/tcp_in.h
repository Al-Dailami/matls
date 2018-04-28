#ifndef __TCP_IN_H__
#define __TCP_IN_H__

#ifdef DARWIN
#include <netinet/if_ether.h>
#include <netinet/tcp.h>
#include <netinet/udp.h>
#else
#include <linux/if_ether.h>
#include <linux/tcp.h>
#include <linux/udp.h>
#endif
#include <netinet/ip.h>

#include "mos_api.h"
#include "mssl.h"
#include "fhash.h"

#define TCP_FLAG_FIN  0x01
#define TCP_FLAG_SYN  0x02
#define TCP_FLAG_RST  0x04
#define TCP_FLAG_PSH  0x08
#define TCP_FLAG_ACK  0x10
#define TCP_FLAG_URG  0x20
#define TCP_FLAG_SACK 0x40
#define TCP_FLAG_WACK 0x80

#define TCP_OPT_FLAG_MSS          0x02
#define TCP_OPT_FLAG_WSCALE       0x04
#define TCP_OPT_FLAG_SACK_PERMIT  0x08
#define TCP_OPT_FLAG_SACK         0x10
#define TCP_OPT_FLAG_TIMESTAMP    0x20

#define TCP_OPT_MSS_LEN         4
#define TCP_OPT_WSCALE_LEN      3
#define TCP_OPT_SACK_PERMIT_LEN 2
#define TCP_OPT_SACK_LEN        10
#define TCP_OPT_TIMESTAMP_LEN   10

#define TCP_DEFAULT_MSS         1460
#define TCP_DEFAULT_WSCALE      7
#define TCP_INITIAL_WINDOW       14600

#define TCP_SEQ_LT(a,b)     ((int32_t)((a)-(b)) < 0)
#define TCP_SEQ_LEQ(a,b)   ((int32_t)((a)-(b)) <= 0)
#define TCP_SEQ_GT(a,b)    ((int32_t)((a)-(b)) > 0)
#define TCP_SEQ_GEQ(a,b)   ((int32_t)((a)-(b)) >= 0)
#define TCP_SEQ_BETWEEN(a,b,c)  (TCP_SEQ_GEQ(a,b) && TCP_SEQ_LEQ(a,c))

#define HZ                  1000
#define TIME_TICK           (1000000 / HZ)
#define TIMEVAL_TO_TS(t)    (uint32_t)((t)->tv_sec * HZ + ((t)->tv_usec / TIME_TICK))

#define TS_TO_USEC(t)       ((t) * TIME_TICK)
#define TS_TO_MSEC(t)       (TS_TO_USEC(t) / 1000)

#define USEC_TO_TS(t)       ((t) / TIME_TICK)
#define MSEC_TO_TS(t)       (USEC_TO_TS((t) * 1000))

#define SEC_TO_USEC(t)      ((t) * 1000000)
#define SEC_TO_MSEC(t)      ((t) * 1000)
#define MSEC_TO_USEC(t)     ((t) * 1000)
#define USEC_TO_SEC(t)      ((t) / 1000000)

#define TCP_TIMEWAIT        0
#define TCP_INITIAL_RTO     (MSEC_TO_USEC(500) / TIME_TICK)
#define TCP_FIN_RTO         (MSEC_TO_USEC(500) / TIME_TICK)
#define TCP_TIMEOUT         (MSEC_TO_USEC(30) / TIME_TICK)

#define TCP_MAX_RTX         16
#define TCP_MAX_SYN_RETRY   7
#define TCP_MAX_BACKOFF     7

enum tcp_state
{
  TCP_ST_CLOSED = 0,
  TCP_ST_LISTEN = 1,
  TCP_ST_SYN_SENT = 2,
  TCP_ST_SYN_RCVD = 3,
  TCP_ST_ESTABLISHED = 4,
  TCP_ST_FIN_WAIT_1 = 5,
  TCP_ST_FIN_WAIT_2 = 6,
  TCP_ST_CLOSE_WAIT = 7,
  TCP_ST_CLOSING = 8,
  TCP_ST_LAST_ACK = 9,
  TCP_ST_TIME_WAIT = 10,
  TCP_ST_CLOSED_RSVD = 11,
};

enum tcp_option
{
  TCP_OPT_END = 0,
  TCP_OPT_NOP = 1,
  TCP_OPT_MSS = 2,
  TCP_OPT_WSCALE = 3,
  TCP_OPT_SACK_PERMIT = 4,
  TCP_OPT_SACK = 5,
  TCP_OPT_TIMESTAMP = 8,
};

enum tcp_close_reason
{
  TCP_NOT_CLOSED = 0,
  TCP_ACTIVE_CLOSE = 1,
  TCP_PASSIVE_CLOSE = 2,
  TCP_CONN_FAIL = 3,
  TCP_CONN_LOST = 4,
  TCP_RESET = 5,
  TCP_NO_MEM = 6,
  TCP_NOT_ACCEPTED = 7,
  TCP_TIMEDOUT = 8,
};

#endif /* __TCP_IN_H__ */