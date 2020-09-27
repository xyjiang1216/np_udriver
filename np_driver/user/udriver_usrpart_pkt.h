/*新版本格式为:64 + metadata + 64 +   2+eth_pkt */
# pragma pack(1)
struct cp_head
{
	uint64_t 	ctl:3,					// 即枚举类型NPE_PKT_SEND_TYPE中的四种
				encrypt:1,
				pkt_len:11,
				slot_id:3,
				dma:21,
				block_idx:7,
				valid_pom:1,
				priority:3,
				card_id:4,
				outport:10;
	uint64_t 	busy:1,
				inport:5,
				color:2,
				cpuID:9,
				resev:8,
				pkt_offset:7,
				pkt_id:32;
	uint64_t 	time;
	uint64_t	next;	
};
# pragma pack()

# pragma pack(1)
struct cp_send
{
	uint64_t ctl;
	uint64_t a;
	uint64_t b;
	uint64_t d;
};
# pragma pack()

# pragma pack(1)
struct um_metadata{
// 目前硬件版本对应的metadata
	uint64_t	dport:16,
				ethtype:16,
				sip:32;
	uint64_t	sport:16,
				dmid:8,
				fin:1,
				syn:1,
				rst:1,
				ack:1,
				pst:3,
				pkt_type:1,
				len:12,
				discard:1,
				reserve:3,
				outport:6,
				outtype:2,
				inport:6,
				pktdst:1,
				pktsrc:1;
	uint64_t	user[2];		
};
# pragma pack()

# pragma pack(1)
struct common_metadata
{
	uint64_t 	a;
	uint64_t 	b:32,
				c:31,
				pkttype:1;
	uint64_t 	d;
	uint64_t	e;
};
# pragma pack()

# pragma pack(1)
/*FAST 2.0环形控制通路控制报文*/
struct ctl_metadata
{
	uint64_t 	data:32,
				mask:32;
	uint64_t 	addr:32,
				dstmid:8,
				srcmid:8,
				seq:12,
				type:3,
				pkttype:1;
	uint64_t	reserve;
	uint64_t 	sessionID;
};
# pragma pack()

# pragma pack(1)
struct cp_packet
{
	//CP头部，硬件使用
	union
	{
		struct cp_head cp;
		struct cp_send cs;
	};
	union
	{
		struct cp_send u2;
		struct um_metadata um;
		struct ctl_metadata cm;			// < 控制报文格式定义
		struct common_metadata md;		// < 公共控制信息，报文类型（0：数据，1：控制）
	};
	
//	struct cp_send u2;
	//报文数据部分
	uint8_t pkt_data[0];
};
# pragma pack()

/*发送控制类型*/
enum NPE_PKT_SEND_TYPE
{
	NPE_CTR_SEND = 0,		/*发送硬件描述符*/
	NPE_CTR_OSND = 1,		/*发送软件描述符*/
	NPE_CTR_DROP = 2,		/*硬件描述符丢弃，用于硬件描述符回收*/
	NPE_CTR_OSDP = 3		/*软件描述符丢弃，用于软件描述符回收*/
};/*此数据类型占用3bit*/
