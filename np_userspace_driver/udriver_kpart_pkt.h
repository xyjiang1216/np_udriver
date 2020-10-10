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
	uint64_t 	ctl:3,					// ��ö������NPE_PKT_SEND_TYPE�е�����
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
#if 0		// ��ǰ�汾Ӳ����Ӧ��metadata
#if defined(__LITTLE_ENDIAN) /*INTER*/
#if 0	// FAST_10
	uint64_t 	ts:32,			/**< @brief ���Ľ��յ�ʱ��� @note ����û���Ҫʹ�ñ�ʾ�����ʱ�䣬����洢�ڵڶ��������У�user[2]�ֶΣ�*/
				ts2:12,
				flowID:14,		/**< @brief ��ID��*/
				priority:3,		/**< @brief �������ȼ�*/
				discard:1,		/**< @brief ָʾ�����Ƿ��� @note Ĭ��Ϊ0����ʾ����������1ʱ��ʾ����*/
				pktdst:1,		/**< @brief ���ĵ����Ŀ�ķ��� @note 0��ʾ���������˿ڣ�1��ʾ�����CPU*/
				pktsrc:1;		/**< @brief ���ĵ�����Դ���� @note 0��ʾ����˿����룬1��ʾ��CPU����*/
	uint64_t 	outport:16,		/**< @brief ��������˿ں� @note ��bitmap��ʽ��ʾ��1��ʾ��0�Ŷ˿������8��ʾ��3�Ŷ˿����*/
				seq:12,			/**< @brief ���Ľ���ʱ�����к� @note ÿ���˿ڶ���ά��һ�����*/
				dstmid:8,		/**< @brief �����´δ����Ŀ��ģ����*/
				srcmid:8,		/**< @brief �����ϴδ���ʱ��ģ����*/
				len:12,			/**< @brief ���ĳ��� @note ���ɱ�ʾ4095�ֽڣ���FASTƽ̨���Ļ��������Ϊ2048��������̫�����ĵ�MTU��Ҫ����1500*/
				inport:4,		/**< @brief ����˿ں� @note ȡֵ��0??15������ʾ16������˿�*/
				ttl:4;			/**< @brief ����ͨ��ģ���TTLֵ��ÿ��һ������ģ���1*/
#endif
#if 0	// FAST_20
		// ����ʹ�õ���FAST_20
	uint64_t 	ts:32,			/**< @brief ʱ���*/
				reserve:17,		/**< @brief ����*/
				pktsrc:1,		/**< @brief �������Դ��0Ϊ����ӿ����룬1ΪCPU���룬�˱���λ�ڽ���Ӳ��ʱ�ύ����pkttypeλ�ã�����λΪ18λ*/
				flowID:14;		/**< @brief ��ID*/
	uint64_t	seq:8,			/**< @brief ����������к�*/
				pst:8,			/**< @brief ��׼Э�����ͣ��ο�Ӳ�����壩*/
				dstmid:8,			/**< @brief ��һ����������ģ��ID*/
				srcmid:8,			/**< @brief ���һ�δ�������ģ��ID*/
				len:12,			/**< @brief ���ĳ���*/
				discard:1,		/**< @brief ����λ*/
				priority:3,		/**< @brief �������ȼ�*/
				outport:6,		/**< @brief ��������������˿�ID���鲥/���飺�鲥/������ַ����*/
				outtype:2,		/**< @brief ������ͣ�00��������01���鲥��10�����飬11��������ӿ����*/
				inport:6,		/**< @brief ���������˿ں�*/
				pktdst:1,		/**< @brief ����Ŀ�ģ�0Ϊ����ӿ������1Ϊ��CPU*/
				pkttype:1;		/**< @brief �������ͣ�0�����ݱ��ģ�1�����Ʊ��ġ�Ӳ��ʶ�������󣬻Ὣpktsrcλ�������ˣ��ָ�Ӳ�����ݸ�ʽ*/		
#endif
	uint64_t 	user[2];		/**< @brief �û��Զ���metadata���ݸ�ʽ������ @remarks ���ֶ��ɿ��û���д������Ҫ��֤���ݴ�С�ϸ��޶���16�ֽ�*/
#endif
#endif
#if 1		// ĿǰӲ���汾��Ӧ��metadata
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

/*FAST 2.0���ο���ͨ·���Ʊ���*/
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
	//CPͷ����Ӳ��ʹ��
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
		struct ctl_metadata cm;			// < ���Ʊ��ĸ�ʽ����
		struct common_metadata md;		// < ����������Ϣ���������ͣ�0�����ݣ�1�����ƣ�
	};
	
//	struct cp_send u2;
	//�������ݲ���
	uint8_t pkt_data[0];
}__attribute__((packed));
