#include<stdint.h>


/******************* 结构体 *******************/
// 存放地址对信息的结构体
// 注意内存对齐的问题！
// 如果不加# pragma pack(1)的参数，struct buf_addr_triple的大小为32，即4个8字节
// 内核态中的取消编译优化命令__attribute__((packed))在用户态没有作用
# pragma pack(1)
struct buf_addr_triple
{
//	uint64_t kernel_vir_addr;
	uint64_t bus_addr;
	uint64_t phy_addr;
	uint64_t user_vir_addr;
};
# pragma pack()
# pragma pack(1)
struct buf_addr_info
{
	uint64_t buf_cnt;
	uint64_t sw_buf_cnt;
	uint64_t hw_buf_cnt;
	uint64_t vir_addr_first;
	struct buf_addr_triple buf_addr[200];
};
# pragma pack()
/*
// PCIE 寄存器定义说明
#define NPE_REG_THREAD_CTL 					0x000
#define NPE_REG_HARD_SYS_SET 				0x008
#define NPE_REG_HARD_DESC_DMA				0x010
#define NPE_REG_VIRT_ADDR 					0x018
#define NPE_PORT_CLEAN 						0x020
#define NPE_REG_HARD_RESET 					0x028
#define NPE_REG_CHANNEL_THRESHOLD 			0x030
#define NPE_REG_HARD_BUF_REQUEST_DESC_CNT 	0x038
*/

// bar0空间的结构体
// 硬件寄存器的长度为32位，因此0~31同32~63的内容是完全一样的
# pragma pack(1)
struct bar0_regs
{
	uint64_t thread_ctl;				// 写需要发送的报文地址的寄存器
	uint64_t hard_sys_set;
	uint64_t hard_desc_dma;
	uint64_t virt_addr;
	uint64_t port_clean;
	uint64_t hard_reset;
	uint64_t channel_threshold;
	uint64_t hard_buf_request_desc_cnt;
};
# pragma pack()

