#include "util.h"
#include "lib/list.h"
#include "lib/memb.h"

MEMB(unique_nodes, unique_node_t, 1024);

void
ipaddr_add(const uip_ipaddr_t *addr)
{
  uint16_t a;
  int i, f;
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

void
util_init()
{
  static uint8_t inited = 0;
  if(!inited) {
    memb_init(&unique_nodes);
	inited = 1;
  }
}

int unique_node_count = 0;
static unique_node_t* unique_node_list[128];

int
insert_unique_node(const uint8_t* addr, int prefix_len)
{
  int idx = get_idx(addr, prefix_len);
  if(idx != -1) return idx;

  unique_node_t* node = memb_alloc(&unique_nodes);
  if( node != NULL ){
	unique_node_list[unique_node_count] = node;
	memset(node, 0, sizeof (unique_node_t));
  } else {
    return -1;
  }

  memcpy(node->addr, addr, prefix_len);
  node->idx = unique_node_count;
  node->nbr_num = 0;
  unique_node_count++;

  return node->idx;
}

unique_node_t*
get_node(int idx)
{
  if(idx < unique_node_count){
    return unique_node_list[idx];
  }

  return NULL;
}

int
get_idx(const uint8_t* addr, int prefix_len)
{
  unique_node_t *u;
  for(int i = 0; i < unique_node_count; i ++){
	u = unique_node_list[i];
	//printf("%x%x:%x%x:%x%x:%x%x\n", u->addr[0], u->addr[1], u->addr[2], u->addr[3],u->addr[4],u->addr[5],u->addr[6],u->addr[7]);

	if(!memcmp(u->addr, addr, prefix_len)) return u->idx;
  }
  return -1;
}

void
free_unique_nodes()
{
  for(int i = 0; i < unique_node_count; i++){
    memb_free(&unique_nodes,unique_node_list[i]);
  }
  unique_node_count = 0;
}

