struct cp_head
{
#if defined(__LITTLE_ENDIAN) /*INTER*/	
	uint64_t	outport:10,
				card_id:4,
				priority:3,
				valid_pom:1,
				block_idx:7,
				dma:21,		
				slot_id:3,
				pkt_len:11,
				encrypt:1,
				ctl:3;
	uint64_t 	pkt_id:32,
				pkt_offset:7,
				resev:8,
				cpuID:9,
				color:2,
				inport:5,
				busy:1;
	uint64_t 	next;
	uint64_t 	time;
#elif defined(__BIG_ENDIAN)  /*FT*/	
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
#else
#error	"Please fix <asm/byteorder.h>"
#endif
}__attribute__((packed));

struct cp_send
{
	uint64_t ctl;
	uint64_t a;
	uint64_t b;
	uint64_t d;
}__attribute__((packed));

struct um_metadata{
#if 0		// 年前版本硬件对应的metadata
#if defined(__LITTLE_ENDIAN) /*INTER*/
#if 0	// FAST_10
	uint64_t 	ts:32,			/**< @brief 报文接收的时间戳 @note 如果用户需要使用表示更大的时间，建议存储在第二拍数据中（user[2]字段）*/
				ts2:12,
				flowID:14,		/**< @brief 流ID号*/
				priority:3,		/**< @brief 报文优先级*/
				discard:1,		/**< @brief 指示报文是否丢弃 @note 默认为0，表示不丢弃，置1时表示丢弃*/
				pktdst:1,		/**< @brief 报文的输出目的方向 @note 0表示输出到网络端口，1表示输出到CPU*/
				pktsrc:1;		/**< @brief 报文的输入源方向 @note 0表示网络端口输入，1表示从CPU输入*/
	uint64_t 	outport:16,		/**< @brief 报文输出端口号 @note 以bitmap形式表示，1表示从0号端口输出；8表示从3号端口输出*/
				seq:12,			/**< @brief 报文接收时的序列号 @note 每个端口独立维护一个编号*/
				dstmid:8,		/**< @brief 报文下次处理的目的模块编号*/
				srcmid:8,		/**< @brief 报文上次处理时的模块编号*/
				len:12,			/**< @brief 报文长度 @note 最大可表示4095字节，但FAST平台报文缓存区最大为2048，完整以太网报文的MTU不要超过1500*/
				inport:4,		/**< @brief 输入端口号 @note 取值：0??15，最多表示16个输入端口*/
				ttl:4;			/**< @brief 报文通过模块的TTL值，每过一个处理模块减1*/
#endif
#if 0	// FAST_20
		// 这里使用的是FAST_20
	uint64_t 	ts:32,			/**< @brief 时间戳*/
				reserve:17,		/**< @brief 保留*/
				pktsrc:1,		/**< @brief 分组的来源，0为网络接口输入，1为CPU输入，此比特位在进入硬件时会交换到pkttype位置，保留位为18位*/
				flowID:14;		/**< @brief 流ID*/
	uint64_t	seq:8,			/**< @brief 分组接收序列号*/
				pst:8,			/**< @brief 标准协议类型（参考硬件定义）*/
				dstmid:8,			/**< @brief 下一个处理分组的模块ID*/
				srcmid:8,			/**< @brief 最近一次处理分组的模块ID*/
				len:12,			/**< @brief 报文长度*/
				discard:1,		/**< @brief 丢弃位*/
				priority:3,		/**< @brief 分组优先级*/
				outport:6,		/**< @brief 单播：分组输出端口ID，组播/泛洪：组播/泛洪表地址索引*/
				outtype:2,		/**< @brief 输出类型，00：单播，01：组播，10：泛洪，11：从输入接口输出*/
				inport:6,		/**< @brief 分组的输入端口号*/
				pktdst:1,		/**< @brief 分组目的，0为网络接口输出，1为送CPU*/
				pkttype:1;		/**< @brief 报文类型，0：数据报文，1：控制报文。硬件识别报文类别后，会将pktsrc位交换到此，恢复硬件数据格式*/		
#endif
	uint64_t 	user[2];		/**< @brief 用户自定义metadata数据格式与内容 @remarks 此字段由可用户改写，但需要保证数据大小严格限定在16字节*/
#endif
#endif
#if 1		// 目前硬件版本对应的metadata
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
#endif			
}__attribute__((packed));

struct common_metadata
{
	uint64_t 	a;
	uint64_t 	b:32,
				c:31,
				pkttype:1;
	uint64_t 	d;
	uint64_t	e;
}__attribute__((packed));

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
//	uint64_t 	pad[4];
}__attribute__((packed));

struct cp_packet
{
	//CP头部，硬件使用
	union
	{
		struct cp_head cp;
		struct cp_send cs;
	};
/**/	
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
}__attribute__((packed));
