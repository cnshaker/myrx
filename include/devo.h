#ifndef __DEVO_H__
#define __DEVO_H__
void Init_DEVO();
u16 DEVO_Callback();
class DEVO
{
public:
	DEVO();
	void Init(void);
	u16 Callback();

private:
	u8 channel_packets;
	u32 fixed_id;
	u32 transmitter_id;
	u8 chns[3];
	u8 chns_idx;
	CYRF6936 CYRF;
	u8 RFChannel;
	u8 ChannelRetry;
	u16 bind_packets;
	bool use_fixed_id;
	enum
	{
		Uninitialized, Binding, Bound, Lost,
	} RFStatus;
	bool ProcessPacket(u8[]);
	void scramble_pkt(u8*pac)
	{
		pac++;
		u32* p=(u32*)pac;
		p[0]^=transmitter_id;
		p[1]^=transmitter_id;
		p[2]^=transmitter_id;
		p[3]^=transmitter_id;
	}
	s32 Channels[12];
};
extern DEVO* pDEVO;
#endif
