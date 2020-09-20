#include<stdint.h>
// open函数需要的头文件
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
// 使用mmap需要的头文件
#include <sys/mman.h> 
// 字符串操作和文件操作
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <string.h>
#include <unistd.h>

#define NP_PCI_CDEV_FILE_PATH 		"/dev/np_pci_cdev0"		// 设备文件路径
#define ALLOC_HW_BUF_NUM 			128						// 申请的硬件管理的缓冲区数目
#define ALLOC_SW_BUF_NUM 			2						// 申请的软件管理的缓冲区数目
#define THREAD_NUM					1

/******************* 结构体 *******************/
// 记录一个mmap内存的信息
struct mmap_info
{
	uint64_t user_vir_addr;
	int size;
};

// static int np_hw_init(struct bar0_regs *regs);
static int np_hw_init(void);

/************* 线程相关的函数 *************/
void polling_buffer(uint64_t first_buf_addr);				// 接收线程的功能——轮询缓冲区
int create_polling_thread(void);							// 创建接收线程		

void soft_pkt_ring_init(void);

void np_show_pkt(unsigned char *pkt);		