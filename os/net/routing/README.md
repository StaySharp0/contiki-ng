1. ADD RPL_CONTROL_OPTION nbr info in rpl-conf.h
2. ADD nbr_info logic in DAO_OUTPUT(rpl-icmp6.c)
3. in rpl_process_dao(rpl-dag.c)
  3.1 ADD save_nbr_info 
  3.2 GET shotPath


idea:
2 - OR operate nbr info hash(bloom filter)
  - and send root through dao message
  
