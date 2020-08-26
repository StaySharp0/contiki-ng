/* yj, <ng-rpl-conf.h> */
#define ADDR_MAX_LENGTH 16
#define ADDR_START_INDEX 15
#define ADDR_LEN (ADDR_MAX_LENGTH - ADDR_START_INDEX)
#define OPT_TRIGGER 0

// ________________________________________
#define SEND_CSMA_MSG 0
#define IS_ROOT 0

#define DEBUG_RPL 0
#define DEBUG_NG_RPL 1

#define D_BLNC_DAO_PROC      ( DEBUG_NG_RPL && 0)
#define D_BLNC_DAO_SEND      ( DEBUG_NG_RPL && 0)
#define D_BLNC_DAO_ACK_SEND  ( DEBUG_NG_RPL && 0)
#define D_BLNC_DAO_ACK_RECV  ( DEBUG_NG_RPL && 0)
#define D_BLNC_BEST_PARENT   ( DEBUG_NG_RPL && 0)

#define DNR_DAO_SEND                     1
#define DNR_DAO_RECV                     0
#define DNR_DAO_PROC                     0
#define DNR_DAO_PROC_CHILD               0
#define DNR_ISRH_ROUTE                   0  
#define DNR_ISRH_SEND                    0
#define DNR_ISRH_RECV                    0
#define DNR_ISRH_DIRECT                  0


#define D_COUNT          0
#define   D_COUNT_DAO     ( D_COUNT && DEBUG_NG_RPL && 1)
#define   D_COUNT_DAO_ACK ( D_COUNT && DEBUG_NG_RPL && 1)
#define   D_COUNT_DIO     ( D_COUNT && DEBUG_NG_RPL && 1)


// ________________________________________
#define APPLY_NG_RPL                     1
#define  A_SEND_DAO    ( APPLY_NG_RPL && 1)
#define  A_RECV_DAO    ( APPLY_NG_RPL && 1)
#define  A_PROC_DAO    ( APPLY_NG_RPL && 1)

#define APPLY_ISHR     ( APPLY_NG_RPL && 1)
#define  A_ISHR_SEND     ( APPLY_ISHR && 1)
#define  A_ISHR_SET_RES  ( APPLY_ISHR && 1)
#define  A_ISHR_SET_REQ  ( APPLY_ISHR && 1)
#define  A_ISRH_DIRECT   ( APPLY_ISHR && 1)

#define APPLY_BALANCE         ( APPLY_NG_RPL        && 0)
#define  A_BLNC_DAO_PROC      ( APPLY_BALANCE       && 1)
#define  A_BLNC_DAO_ACK_SEND  ( A_BLNC_DAO_PROC     && 1)
#define  A_BLNC_DAO_ACK_RECV  ( A_BLNC_DAO_ACK_SEND && 1)

#if APPLY_BALANCE
  #define RECOMMAND_PARENT 1
  #define RECOMMAND_PARENT_ACK 2
  #define THR_DIFF_ETX 60
#endif


#define WARM_UP_TIME 3 * 60 * 60 // 6:12 50:3
#define THR_ETX 280

#define MAX_COUNT 100
#define MAX_NODE 101
#define MAX_NBR 101

#define REQUEST 2
#define RESPONSE 1