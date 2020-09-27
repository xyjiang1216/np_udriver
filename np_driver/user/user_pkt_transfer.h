#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <fcntl.h>
#include <stdint.h>
#include <asm/types.h>
#include <pthread.h>

// 设备文件路径
#define NP_PCI_CDEV_FILE_PATH_recv 	"/dev/np_pci_cdev0"
#define NP_PCI_CDEV_FILE_PATH_send 	"/dev/np_pci_cdev1" 
#define PKT_SIZE 2048
#define DESC_NUM 2048

// 描述符
// 描述符组织成环形链表
struct desc{
	// 该描述符上是否有报文
	char busy;
	struct cp_packet *pkt;
	struct desc *nxt;
};
// 描述符环
// 需要被频繁访问，可以优化
struct desc_ring{
	int d_num;
	struct desc *ring;
	// 环上的读写指针
	struct desc *snd;
	struct desc *rcv;
};

