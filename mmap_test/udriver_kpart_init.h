#include <linux/pci.h>
#include <linux/module.h>
#include <linux/mm.h>
#include <linux/gfp.h>
#include <linux/uio_driver.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/cdev.h>
#include <linux/slab.h>
#include <linux/fs.h>
#include <linux/platform_device.h>
#include <linux/delay.h>
#include <linux/kdev_t.h>

/******************* 宏 *******************/
#define DRIVER_NAME "udriver_kpart"
#define ORDER_OF_PAGES_INFO_4_RELEASE 2 		// 存放所有页表信息的结构体的页表order 16K
#define ORDER_OF_BUF_ADDR_INFO 9			// 存放buf_addr_info结构体的内存的order 2M
#define DMA_FLAG 1					// 是DMA内存，并且进行了DMA映射
#define NOT_DMA_FLAG 0					// 不是DMA内存，没进行DMA映射
#define ALLOC_HW_BUF_NUM 1				// 申请的缓冲区数目
#define ORDER_OF_BUF 9					// 缓冲区大小 2M

/***************** 结构体 *****************/

// pages_info_4_release记录内核中申请的所有页
// 用于在free_all_pages()释放这些页面
// page_info记录某一次分配的内存的信息
struct page_info
{
	uint64_t base_addr;
	uint64_t bus_addr;
	uint64_t order;								// size = (1 << order) * PAGE_SIZE
	uint64_t is_dma_map;						// 1:dma_map; 0:no dma_map
}__attribute__((packed));	
struct pages_info_4_release{
	uint64_t cnt;
	struct page_info p_info[160]; 				// 最大分配160个内存块
}__attribute__((packed));						// 使用这个标识是为了节省空间，使结构体更为紧凑

// buf_addr_info记录内核申请的软硬件地址对
// 这个结构体会被mmap到用户地址空间中
// 用户地址空间根据此结构体的信息向硬件寄存器写地址
// buf_addr_triple记录某一块2M内存的地址三元组：物理内存地址、总线地址、用户空间地址
// 其中用户空间地址在mmap到用户地址空间中再填写
struct buf_addr_triple
{
//	uint64_t kernel_vir_addr;					// 使用不到内核虚拟地址
	uint64_t bus_addr;
	uint64_t phy_addr;
	uint64_t user_vir_addr;
}__attribute__((packed));
struct buf_addr_info
{
	uint64_t buf_cnt;
	uint64_t sw_buf_cnt;
	uint64_t hw_buf_cnt;
	uint64_t vir_addr_first;					// 记录用户空间虚拟地址开头的32个bits，即4个字节
	struct buf_addr_triple buf_addr[200];
}__attribute__((packed));

// np_pci_cdev包含字符设备和一些内存的地址
// 这些内存将会被映射到用户地址空间
// np_pci_cdev_mem相当于uio中的uio_mem
struct np_pci_cdev_mem{
	uint64_t phy_addr;
	int64_t size;
};
struct np_pci_cdev
{
	struct cdev cdev;
	uint8_t mem_info_cnt;
	struct np_pci_cdev_mem np_pci_cdev_mem_info[200];
};



/***************** 函数声明 ***************/
// 使用get_free_pages方法申请内存，将内存清零，并使之驻留在主存中
// kzalloc最多只能分配128K内存（？），这里不适用
static uint64_t get_pages(uint64_t pages_order, uint8_t dma_flag);		//dma_flag 1:有GFP_DMA 0:无GFP_DMA
// 将get_pages申请的内存记录在pages_info_4_release结构体中
static void record_pages_info(struct pages_info_4_release *p_release, uint64_t base_address, uint64_t bus_address, uint64_t pages_order, uint64_t dma_flag);
// 软件缓冲区初始化
static int np_sw_init(void);
// 释放pages_info_4_release中记录的所有内存，并释放存放pages_info_4_release结构体的内存
// 使用kzalloc()分配的内存，在np_kernel_release中被释放
static void free_all_pages(struct pages_info_4_release *p_release);


// 字符设备文件的file_operations
static int np_pci_cdev_open(struct inode *inode, struct file *filp);
static int np_pci_cdev_mmap(struct file *filp, struct vm_area_struct *vma);
