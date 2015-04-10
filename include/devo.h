#ifndef __DEVO_H__
#define __DEVO_H__
void build_bind_pkt();
void add_pkt_suffix();
static void build_data_pkt();
void cyrf_set_bound_sop_code();
void scramble_pkt();
void build_beacon_pkt(int upper);
void set_radio_channels();
void DEVO_Initialize();
u16 DEVO_Callback();
void DEVO_BuildPacket();
#endif
