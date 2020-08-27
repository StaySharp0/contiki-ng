
#include "net/routing/ng-rpl.h"
#include "net/routing/rpl-lite/rpl.h"

#include "lib/list.h"
#include "lib/memb.h"
#include "sys/timer.h"

#include <stdio.h>

void
ipaddr_add(const uip_ipaddr_t *addr)
{
  uint16_t a;
  int i, f;
  
  if(addr == NULL) return;

  for(i = 0, f = 0; i < sizeof(uip_ipaddr_t); i += 2) {
    a = (addr->u8[i] << 8) + addr->u8[i + 1];
    if(a == 0 && f >= 0) {
      if(f++ == 0) {
        ADD("::");
      }
    } else {
      if(f > 0) {
        f = -1;
      } else if(i > 0) {
        ADD(":");
      }
      ADD("%x", a);
    }
  }
}

clock_time_t ng_rpl_now = 0;
clock_time_t
get_clock() {
  return ng_rpl_now;
}
void set_clock(clock_time_t clock) {
  ng_rpl_now = clock;
}

#if APPLY_NG_RPL
  #if IS_ROOT
    MEMB(unique_nodes, unique_node_t, MAX_NODE);
    static unique_node_t* unique_node_list[MAX_NODE];
  #endif
  static ng_rpl_t ng_rpl_obj = \
  { 
    FALSE,  //inited
    FALSE,  //isOpt
    0,      //opt_count
    0,      //node_count
    {0,},   // request_path
    0,      // request_path_num
    {0,},   // response_path
    0       // response_path_num
  };

  void
  ng_rpl_init()
  {
    if(ng_rpl_obj.inited == FALSE) {
      ng_rpl_now = 0;
      #if IS_ROOT
        memb_init(&unique_nodes);
      #endif
      ng_rpl_obj.inited = TRUE;
      memset(ng_rpl_obj.request_path, 0 , sizeof(ng_rpl_obj.request_path));
      memset(ng_rpl_obj.response_path, 0 , sizeof(ng_rpl_obj.response_path));
    }
  }

#if IS_ROOT
  void
  free_unique_nodes()
  {
    int i;
    for(i = 0; i < ng_rpl_obj.node_count; i++){
      memb_free(&unique_nodes, unique_node_list[i]);
    }
    ng_rpl_obj.node_count = 0;
  }


  uint16_t get_count()
  {
    return ng_rpl_obj.node_count;
  }

  unique_node_t* 
  get_node_by_idx(uint16_t idx)
  {
    if(idx < ng_rpl_obj.node_count){
      return unique_node_list[idx];
    }

    return NULL;
  }

  unique_node_t* 
  get_node_by_addr(const uint8_t* addr, uint8_t len)
  {
    unique_node_t *u;
    uint16_t i;
    
    for(i = 0; i < ng_rpl_obj.node_count; i++){
      u = unique_node_list[i];

      if(!memcmp(u->addr, addr, len)) return u;
    }

    return NULL;
  }

  unique_node_t* 
  push_node(const uint8_t* addr, uint8_t len)
  {
    unique_node_t* node = get_node_by_addr(addr, len);
    if(node != NULL) return node;

    node = memb_alloc(&unique_nodes);
    
    if( node != NULL ){
      unique_node_list[ng_rpl_obj.node_count] = node;
      memset(node, 0, sizeof (unique_node_t));

      // new node
      memcpy(node->addr, addr, len);
      node->idx = ng_rpl_obj.node_count;
      node->nbr_num = 0;

      #if APPLY_BALANCE
        node->parent_idx = ng_rpl_obj.node_count;
        node->child_num = 0;
      #endif

      ng_rpl_obj.node_count++;
      ng_rpl_obj.isOpt = FALSE;
    }

    return node;
  }

  uint16_t etx_map[MAX_NODE][MAX_NODE];
  uint8_t path_map[MAX_NODE][MAX_NODE];

  void
  opt_path()
  {
    if(ng_rpl_obj.opt_count == OPT_TRIGGER) {
      ng_rpl_obj.opt_count = 0;
      ng_rpl_obj.isOpt = FALSE;
    } else {
      ng_rpl_obj.opt_count++;
    }
    if(ng_rpl_obj.isOpt == TRUE) return;

    memset(etx_map, 0xffff, sizeof(etx_map)); 
    memset(path_map, -1, sizeof(path_map));

    unique_node_t* u;
    uint16_t r, c, s;

    for ( r = 0; r < ng_rpl_obj.node_count; r++ ){
      u = unique_node_list[r];

      for ( c = 0; c < u->nbr_num ; c++ ){
        s = u->nbr[c];

        if(u->nbr_etx[c] < THR_ETX) {
          path_map[r][s] = r;
          path_map[s][r] = s;

          etx_map[r][s] = u->nbr_etx[c];
          if (etx_map[s][r] < u->nbr_etx[c]) etx_map[r][s] = etx_map[s][r];
          else etx_map[s][r] = u->nbr_etx[c]; 
        }
      }
    }

    // if(ng_rpl_obj.node_count == MAX_NODE) output_origin_etx();

    for( s = 0; s < ng_rpl_obj.node_count; s++ )
      for( r = 0 ; r < ng_rpl_obj.node_count; r++ )
        for( c = 0; c < ng_rpl_obj.node_count; c++ ) {
          if( r == c ) continue;
          if((int) etx_map[r][c] > (int) ((int) etx_map[r][s] + (int) etx_map[s][c])){
            etx_map[r][c] = etx_map[r][s] + etx_map[s][c];
            path_map[r][c] = path_map[s][c];
          }
        }


    // if(ng_rpl_obj.node_count == MAX_NODE) output_opt();
    ng_rpl_obj.isOpt = TRUE;
  }

  uint8_t 
  get_opt_hop(uint8_t s, uint8_t d)
  {
    if(s >= MAX_NODE || d >= MAX_NODE) return -1;

    uint8_t count = 0;
    uint8_t pre = path_map[s][d];

    while(pre != s) {
      pre = path_map[s][pre];
      count++;
    }

    #if 0
      ADD("[NG-RPL] opt_path: %u(%u)", count, etx_map[s][d]);
      PRINT();
    #endif
    
    return count;
  }

  void 
  set_opt_path(uint8_t* hop_ptr, unique_node_t* src, unique_node_t* dst, uint8_t len)
  {
    uint8_t* path_ptr = hop_ptr;
    uint8_t s = src->idx;
    uint8_t d = dst->idx;
    uint8_t pre = path_map[s][d];
    unique_node_t* waypoint; 

    #if DEBUG_NG_RPL && DNR_ISRH_ROUTE
      ADD("[NG-RPL] ISRH ROUTE(%u->%u): %u", dst->addr[0], src->addr[0], dst->addr[0]);
    #endif

    while(pre != s) {
      waypoint = get_node_by_idx(pre);

      path_ptr -= len;
      memcpy(path_ptr, waypoint->addr, len);

      #if DEBUG_NG_RPL && DNR_ISRH_ROUTE
        ADD(" -> %u", waypoint->addr[0]);
      #endif
      
      pre = path_map[s][pre];
    }

    #if DEBUG_NG_RPL && DNR_ISRH_ROUTE
      ADD(" -> %u", src->addr[0]);
      PRINT();
    #endif
  }

#endif

  void
  set_direct_path(uint8_t type, uint8_t* src, uint8_t* dst, uint8_t* path, uint8_t num, uint8_t unique_len)
  {
    uint8_t i;
    uint8_t* arr_ptr = NULL;

    if(type == REQUEST) {
      arr_ptr = ng_rpl_obj.request_path;
      ng_rpl_obj.request_path_num = num;

      memcpy(arr_ptr, src + ADDR_START_INDEX, ADDR_LEN);
      for(i = 0; i < (num - 1); i++) { // 목적지 출발지 길이제외
        memcpy(arr_ptr + ADDR_LEN * (i + 1), path + unique_len * i, ADDR_LEN);
      }
      memcpy(arr_ptr + (num - 1) * ADDR_LEN, dst + ADDR_START_INDEX, ADDR_LEN);
    } 
    else if (type == RESPONSE){
      arr_ptr = ng_rpl_obj.response_path;
      ng_rpl_obj.response_path_num = num + 2;

      memcpy(arr_ptr, src + ADDR_START_INDEX, ADDR_LEN);
      memcpy(arr_ptr + ADDR_LEN, path, num * ADDR_LEN);
      memcpy(arr_ptr + (num + 1) * ADDR_LEN, dst + ADDR_START_INDEX, ADDR_LEN);
    }

    #if DEBUG_NG_RPL && DNR_ISRH_RECV
      uint8_t cond = num + (type == REQUEST ? 0 : 2);
      for(i = 0; i < cond; i++) {
        ADD("%u ", arr_ptr[i]);
      }
      PRINT();  
    #endif
  }


  uint8_t
  can_send_direct(uint8_t* dst){
    if(ng_rpl_obj.response_path_num >= 3 
      && !memcmp(ng_rpl_obj.response_path, dst + ADDR_START_INDEX, ADDR_LEN)) 
      return RESPONSE;
    else if(ng_rpl_obj.request_path_num >= 3 
      && !memcmp(ng_rpl_obj.request_path, dst + ADDR_START_INDEX, ADDR_LEN)) 
      return REQUEST;
    else return FALSE;
  }

  uint8_t 
  get_direct_len(uint8_t type)
  {
    return type == REQUEST ? 
      ng_rpl_obj.request_path_num :
      ng_rpl_obj.response_path_num;
  }

  void 
  get_direct_path(uint8_t type, uint8_t prefix, uint8_t* hop_ptr)
  {
    int16_t i;
    uint8_t num;
    uint8_t* ptr;

    if(type == REQUEST) {
      num = ng_rpl_obj.request_path_num;
      ptr = ng_rpl_obj.request_path;
    } else {
      num = ng_rpl_obj.response_path_num;
      ptr = ng_rpl_obj.response_path;
    }

    for(i = num - 3; i >= 0; i--) { //src, next-hop 제외
      hop_ptr[0] = ptr[i];
      hop_ptr[2] = ptr[i];
      hop_ptr[4] = ptr[i];
      hop_ptr[6] = ptr[i];
      hop_ptr += (ADDR_MAX_LENGTH - prefix);
    }
  }

  void 
  get_direct_dst(uint8_t type, uint8_t* addr)
  {
    uint8_t* ptr = type == REQUEST ? 
      ng_rpl_obj.request_path :
      ng_rpl_obj.response_path;
    uint8_t ptr_idx = type == REQUEST ? 
      ng_rpl_obj.request_path_num - 2 :
      ng_rpl_obj.response_path_num - 2;

    addr[9] = ptr[ptr_idx];
    addr[11] = ptr[ptr_idx];
    addr[13] = ptr[ptr_idx];
    addr[15] = ptr[ptr_idx];

    if(ptr[ptr_idx] == 0) {
      uint8_t i;
      for(i = 0; i < ptr_idx + 2; i++) {
        ADD("%u ", ptr[i]);
      }
      PRINT();  
    }
  }
#endif

#if APPLY_BALANCE && IS_ROOT
  uint16_t high_parent_etx = 0;
  uint16_t avg_parent_etx = 0;

  int get_rank1_child(unique_node_t *node, bool isFirst) {
    static uint16_t loopChk[MAX_NODE];
    unique_node_t *parent = get_node_by_idx(node->parent_idx);

    if(isFirst) {
      memset(loopChk, 0, sizeof(loopChk));
      loopChk[node->idx] = TRUE;
    }

    if(node->addr[0] == 1) {
      return node->child_num;
    } else {
      if(loopChk[parent->idx]) {
        return 999999;
      } else {
        loopChk[parent->idx] = TRUE;
        return get_rank1_child(parent, FALSE) + node->child_num;
      }
    }
  }

  uint8_t check(int16_t a, int16_t b) {
    if (a - b == 7 || a - b == -7) return FALSE;
    else if (a - b == 8 || a - b == -8) return FALSE;
    else if (a - b == 6 || a - b == -6) return FALSE;
    else if (a - b == 1 || a - b == -1) return FALSE;
    
    return TRUE;
  }

  uint8_t
  find_good_parent(unique_node_t* self, uint8_t *ptr) {
    uint16_t i, avg_count;
    unique_node_t *nbr, *parent, *origin;
    uint16_t nbr_etx;
    uint16_t origin_child_num, origin_etx;
    uint16_t parent_child_num, parent_etx;
    int self_rank, nbr_rank, parent_rank, origin_rank;
    int nbr_rank1_child = 0, parent_rank1_child  = 0, origin_rank1_child = 0;

    parent = get_node_by_idx(self->parent_idx);
    origin = parent;

    self_rank = get_rank(self, TRUE);
    origin_etx = self->parent_etx;
    origin_child_num = parent->child_num;
    origin_rank = get_rank(parent, TRUE);
    origin_rank1_child = get_rank1_child(parent, TRUE);

    parent_etx = origin_etx;
    parent_child_num = origin_child_num;
    parent_rank = origin_rank;
    parent_rank1_child = origin_rank1_child;
    

    high_parent_etx = 0;
    avg_parent_etx = 0;
    avg_count = 0;
    for(i = 0; i < ng_rpl_obj.node_count; i++){
        nbr = get_node_by_idx(i);
      
        if(high_parent_etx < nbr->parent_etx) high_parent_etx = nbr->parent_etx;

        if(nbr->parent_etx != 0){
          avg_parent_etx += nbr->parent_etx;
          avg_count++;
        }
    }
    
    avg_parent_etx /= avg_count;

    #if D_BLNC_DAO_ACK_SEND
      // ADD("find_good_parent: %u[%u]:%d -> %u[%u]:%d\n", 
      //   self->addr[0], self->idx, self_rank, parent->addr[0], parent->idx, parent_rank);
      ADD("find_good_parent: %u -> %u (%u, %u) ", 
        self->addr[0], parent->addr[0], self_rank, parent_rank);
      ADD("\nhigh etx: %u, avg etx: %u", high_parent_etx, avg_parent_etx);
      PRINT();
    #endif

    for (i = 0; i < self->nbr_num; i++) {
      nbr = get_node_by_idx(self->nbr[i]);
      nbr_rank = get_rank(nbr, TRUE);
      nbr_etx = self->nbr_etx[i];
      nbr_rank1_child = get_rank1_child(nbr, TRUE);

      if(self->parent_idx == self->nbr[i]) {
        #if D_BLNC_DAO_ACK_SEND
          ADD(" ! %u[%u] rank: %u, child: %u, allChild: %u, etx: %u", \
            nbr->addr[0], nbr->idx, nbr_rank, nbr->child_num, nbr_rank1_child, nbr_etx);
          PRINT();
        #endif
        continue;
      }
      else if(nbr_rank < (self_rank - 1) && nbr_etx <= high_parent_etx) {
        #if D_BLNC_DAO_ACK_SEND
          ADD(" - %u[%u] rank: %u, child: %u, allChild: %u, etx: %u", \
            nbr->addr[0], nbr->idx, nbr_rank, nbr->child_num, nbr_rank1_child, nbr_etx);
        #endif

        if (
          nbr_etx < parent_etx
          // && (nbr_etx < parent_etx || nbr_etx < avg_parent_etx * 1.3 )
          && nbr_rank1_child + nbr->child_num < (parent_rank1_child + parent->child_num - 1) // 25 case
        ) {
          #if D_BLNC_DAO_ACK_SEND
            ADD(" <-");
            PRINT();
          #endif
          parent = nbr;
          parent_etx = nbr_etx;
          parent_rank = nbr_rank;
          parent_rank1_child = nbr_rank1_child;
        }
        else {
          #if D_BLNC_DAO_ACK_SEND
            PRINT();
          #endif
        }
      }
    }

    if(nbr_rank && nbr_etx && self_rank && parent_rank && parent_child_num && nbr_rank1_child && parent_rank1_child) {}

    if(self->parent_idx != parent->idx) {
      #if D_BLNC_DAO_ACK_SEND 
        ADD(">> %u[%u]:%d -> %u[%u]%u", \
           self->addr[0], self->idx, self_rank, parent->addr[0], parent->idx, parent_rank);   
        if(check(self->addr[0], parent->addr[0])) ADD("!!!");
        PRINT();
      #endif

      self->parent_idx = parent->idx;
      self->parent_etx = parent_etx;
      parent->child_num++;
      origin->child_num--;
    
      ptr[1] = RECOMMAND_PARENT;
      ptr[4] = ADDR_LEN;
      memcpy((ptr + 5), parent->addr, ADDR_LEN);

      return TRUE;
    }

    return FALSE;
  }


  int
  get_rank(unique_node_t* self, bool isFirst) {
    static uint16_t loopChk[MAX_NODE];
    unique_node_t *u = NULL;
    uint16_t i;

    if(isFirst) {
      memset(loopChk, 0, sizeof(loopChk));
    }

    if(self->addr[0] == 1) {
      return 1;
    } 
    else if(self->idx == self->parent_idx) {
      return 101;
    } else {
      for(i = 0; i < ng_rpl_obj.node_count; i++){
        u = unique_node_list[i];

        if(self->parent_idx == u->idx) {
          if(loopChk[u->idx]) {
            return 999999;
          } else {
            loopChk[u->idx] = TRUE;
            return 1 + get_rank(u, FALSE);
          }
        }
      }
    }

    return 101;
  }
#endif

#if D_COUNT
  uint32_t cnt_dao = 0;
  uint32_t cnt_dao_ack = 0;
  uint32_t cnt_dio = 0;

  uint32_t get_cnt_dao() {return ++cnt_dao;}
  uint32_t get_cnt_dao_ack() {return ++cnt_dao_ack;}
  uint32_t get_cnt_dio() {return ++cnt_dio;}
#endif