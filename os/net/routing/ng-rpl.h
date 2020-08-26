#ifndef NG_RPL_H
#define NG_RPL_H

#include "contiki.h"
#include "net/routing/ng-rpl-conf.h"
#include "net/routing/routing.h"
#include "net/ipv6/uip-ds6-route.h"
#include "net/ipv6/uip-sr.h"
#include "net/packetbuf.h"

#include <stdio.h>
#include <string.h>

#define TRUE 1
#define FALSE 0

char buf[128];
uint16_t blen;

#define ADD(...) do {                                                   \
    blen += snprintf(&buf[blen], sizeof(buf) - blen, __VA_ARGS__);      \
  } while(0)
#define PRINT(s) do { \
  printf("%s\n", buf); \
  blen = 0; \
} while(0);

void ipaddr_add(const uip_ipaddr_t *addr);
clock_time_t get_clock();
void set_clock(clock_time_t);

#if APPLY_NG_RPL
  typedef struct ng_rpl
  {
    uint8_t inited;
    uint8_t isOpt;
    uint8_t opt_count;
    uint16_t node_count;
    uint8_t request_path[ADDR_LEN * MAX_NODE];
    uint8_t request_path_num;
    uint8_t response_path[ADDR_LEN * MAX_NODE];
    uint8_t response_path_num;

  } ng_rpl_t;

  typedef struct unique_node
  {
    uint16_t idx;
    uint8_t addr[ADDR_MAX_LENGTH-ADDR_START_INDEX];
    uint16_t nbr[MAX_NBR];
    uint16_t nbr_etx[MAX_NBR];
    uint16_t nbr_num;

    #if APPLY_BALANCE
      uint16_t parent_idx;
      uint16_t parent_etx;
      uint16_t child_num;
    #endif
  } unique_node_t;

  void ng_rpl_init();
  
#if IS_ROOT
  void free_unique_nodes();

  uint16_t get_count();
  unique_node_t* get_node_by_addr(const uint8_t* addr, uint8_t len);
  unique_node_t* get_node_by_idx(uint16_t idx);
  unique_node_t* push_node(const uint8_t* addr, uint8_t len);

  void opt_path();
  uint8_t get_opt_hop(uint8_t s, uint8_t d);
  void set_opt_path(uint8_t* hop_ptr, unique_node_t* s, unique_node_t* d, uint8_t len);
#endif

  void set_direct_path(uint8_t type, uint8_t* src, uint8_t* dst, uint8_t* path, uint8_t num, uint8_t unique_len);
  uint8_t can_send_direct(uint8_t* dst);
  uint8_t get_direct_len(uint8_t type);
  void get_direct_path(uint8_t type, uint8_t prefix, uint8_t* hop_ptr);
  void get_direct_dst(uint8_t type, uint8_t* ptr);
#endif

#if APPLY_BALANCE
  uint8_t find_good_parent(unique_node_t* self, uint8_t *ptr);
  int get_rank(unique_node_t* self, bool isFirst);
#endif


#if D_COUNT
  uint32_t get_cnt_dao();
  uint32_t get_cnt_dao_ack();
  uint32_t get_cnt_dio();
#endif
#endif