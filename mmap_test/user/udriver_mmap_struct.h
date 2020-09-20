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


