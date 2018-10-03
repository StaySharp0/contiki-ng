#include "contiki.h"
#include "net/routing/routing.h"
#include "net/ipv6/uip-ds6-route.h"
#include "net/ipv6/uip-sr.h"

#include <stdio.h>
#include <string.h>

char buf[256];
int blen;


#define ADD(...) do {                                                   \
    blen += snprintf(&buf[blen], sizeof(buf) - blen, __VA_ARGS__);      \
  } while(0)
#define PRINT(s) do { \
  printf("%s\n", buf); \
  blen = 0; \
} while(0);


void ipaddr_add(const uip_ipaddr_t *addr);


void util_init();

typedef struct unique_node
{
  uint8_t addr[16];
  uint16_t idx;
  uint16_t nbr[128];
  uint16_t nbr_num;
} unique_node_t;

int
insert_unique_node(const uint8_t* addr, int prefix_len);

unique_node_t*
get_node(int idx);

int
get_idx(const uint8_t* addr, int prefix_len);


