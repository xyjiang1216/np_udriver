#include "udriver_mmap_struct.h"
#include "udriver_usrpart_init.h"
#include "udriver_usrpart_pkt.h"
#include <pthread.h>
#include <sys/time.h>
#include <sys/wait.h>
#include <errno.h>

/******************* 全局变量 *******************/
// 记录地址三元组信息的结构体，指向map1映射到用户空间的地址
struct buf_addr_info *buf_addr_info;
// bar0空间对应的结构体，指向map0映射到用户空间的地址
struct bar0_regs *bar0_registers;
//unsigned char *bar0_registers;
uint64_t g_buf_addr[200];

// 线程标识
pthread_t thread[THREAD_NUM];			// 一个线程

FILE *filp;

// #define handle_error(meg) \	
	// do { perror(msg); exit(EXIT_FAILURE); } while(0)

// 测试报文
char tcp_pkt[66] = {
	0x00, 0xe0, 0x4c, 0x12, 0x59, 0x90, 0x00, 0x21, 0x85, 0xc5, 0x2b, 0x8f, 0x08, 0x00, 0x45, 0x00, 
0x00, 0x34, 0x0a, 0x27, 0x40, 0x00, 0x40, 0x06, 0xac, 0x83, 0xc0, 0xa8, 0x01, 0x64, 0xc0, 0xa8, 
0x01, 0x65, 0x05, 0x05, 0x00, 0x50, 0x7a, 0x77, 0xff, 0x4b, 0x00, 0x00, 0x00, 0x00, 0x80, 0x02, 
0xff, 0xff, 0x59, 0xcf, 0x00, 0x00, 0x02, 0x04, 0x17, 0xca, 0x01, 0x03, 0x03, 0x01, 0x01, 0x01, 
0x04, 0x02
};

/*char icmp_pkt[74] = {0x00, 0xe0, 0x4c, 0x12, 0x59, 0x90, 0x00, 0x21, 0x85, 0xc5, 0x2b, 0x8f, 0x08, 0x00, 0x45, 0x00, 
						0x00, 0x3c, 0x79, 0x19, 0x00, 0x00, 0x40, 0x01, 0x00, 0x00, 0xc0, 0xa8, 0x01, 0x64, 0xc0, 0xa8, 
						0x01, 0x65, 0x08, 0x00, 0x44, 0x5c, 0x04, 0x00, 0x05, 0x00, 0x61, 0x62, 0x63, 0x64, 0x65, 0x66, 
						0x67, 0x68, 0x69, 0x6a, 0x6b, 0x6c, 0x6d, 0x6e, 0x6f, 0x70, 0x71, 0x72, 0x73, 0x74, 0x75, 0x76, 
						0x77, 0x61, 0x62, 0x63, 0x64, 0x65, 0x66, 0x67, 0x68, 0x69};
*/
/*char icmp_pkt[74] = {0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0x00, 0x21, 0x85, 0xc5, 0x2b, 0x8f, 0x08, 0x00, 0x45, 0x00, 
						0x00, 0x3c, 0x79, 0x19, 0x00, 0x00, 0x40, 0x01, 0x7d, 0x8e, 0xc0, 0xa8, 0x01, 0x64, 0xc0, 0xa8, 
						0x01, 0x65, 0x08, 0x00, 0x44, 0x5c, 0x04, 0x00, 0x05, 0x00, 0x61, 0x62, 0x63, 0x64, 0x65, 0x66, 
						0x67, 0x68, 0x69, 0x6a, 0x6b, 0x6c, 0x6d, 0x6e, 0x6f, 0x70, 0x71, 0x72, 0x73, 0x74, 0x75, 0x76, 
						0x77, 0x61, 0x62, 0x63, 0x64, 0x65, 0x66, 0x67, 0x68, 0x69};
*/
/*char icmp_pkt[34] = {0x00, 0xe0, 0x4c, 0x12, 0x59, 0x90, 0x00, 0x21, 0x85, 0xc5, 0x2b, 0x8f, 0x08, 0x00, 0x45, 0x00, 
						0x00, 0x3c, 0x79, 0x19, 0x00, 0x00, 0x40, 0x01, 0x7d, 0x8e, 0xc0, 0xa8, 0x01, 0x64, 0xc0, 0xa8, 
						0x01, 0x65};
*/
void handler(int sig){

	if(filp){
		fclose(filp);
	}

	// 解mmap映射
	int i;
	int err;
/*	for(i = 0; i < ALLOC_HW_BUF_NUM + ALLOC_SW_BUF_NUM + 2; i++){
		// printf("%s\n", "++++++++++++++++++++ test0 ++++++++++++++++++++\n");
		if( i < ALLOC_HW_BUF_NUM + ALLOC_SW_BUF_NUM){
			err = munmap((void *)buf_addr_info->buf_addr[i].user_vir_addr, (1 << 9) * getpagesize());
		}else if(i == ALLOC_HW_BUF_NUM + ALLOC_SW_BUF_NUM){
			munmap((void *)bar0_registers, (1 << 9) * getpagesize());
		}else if(i == ALLOC_HW_BUF_NUM + ALLOC_SW_BUF_NUM + 1){
			munmap((void *)buf_addr_info, (1 << 9) * getpagesize());
		}
		if(err == -1){
			printf("munmmap failure value i %d\n", i);
		}

	}
*/	
	printf("%s\n", "polling is over!!!");
	exit(0);
}


void np_show_pkt(unsigned char *pkt)
{
        int i=0,len=0;
        printf("-----------------------*******-----------------------\n");
        printf("Memo addr:%lX\n",(unsigned long)pkt);

        len = 224;
		for(i=0;i<16;i++)
		{
			if(i % 16 == 0)
				printf("      ");
			printf(" %X ",i);
			if(i % 16 == 15)
				printf("\n");
		}
        
/*		for(i = 0; i < 8; i++){
			if(i % 8 == 0)
				printf("      ");
			printf(" %X ",i);
			if(i % 8 == 7)
				printf("\n");
        }
*/
/**/
        for(i=0;i<len;i++)
        {
                if(i % 16 == 0)
                        printf("%04X: ",i);
                 printf("%02X ",*(pkt+i));
                if(i % 16 == 15)
                        printf("\n");
        }
        if(len % 16 !=0)
                printf("\n");
	
/*		for(i = 0;i < len;i ++)
        {
			if(i % 8 == 0)
				printf("%04X: ",i);
			printf("%02X ",*(pkt+i));
			if(i % 8 == 7)
				printf("\n");
        }
		if(len % 8 != 0)
			printf("\n");
*/		printf("-----------------------*******-----------------------\n");
		for(i=0;i<16;i++)
		{
			if(i % 16 == 0)
				printf("      ");
			printf(" %X ",i);
			if(i % 16 == 15)
				printf("\n");
		}
/*
        for(i = 0; i < 8; i++){
			if(i % 8 == 0)
				printf("      ");
			printf(" %X ",i);
			if(i % 8 == 7)
				printf("\n");
        }
*/        
/*		for(i = 0;i < len;i ++)
		{
			if(i % 8 == 0)
				printf("%04X: ",i);

			if(i < 64){
				printf("%02X ",*(pkt+i));
			}else{
				// 可打印字符
				if(*(pkt + i) >= 0x20 && *(pkt + i) <= 0x7e){
					printf("%2c ",*(pkt+i));
				}else{
					printf(".. ");
				}
			}

			if(i % 8 == 7)
				printf("\n");
        }
		if(len % 8 != 0)
			printf("\n");
*/
		for(i = 0;i < len;i ++)
		{
			if(i % 16 == 0)
				printf("%04X: ",i);

			if(i < 64){
				printf("%02X ",*(pkt+i));
			}else{
				// 可打印字符
				if(*(pkt + i) >= 0x20 && *(pkt + i) <= 0x7e){
					printf("%2c ",*(pkt+i));
				}else{
					printf(".. ");
				}
			}

			if(i % 16 == 15)
				printf("\n");
        }
		if(len % 16 != 0)
			printf("\n");
		printf("-----------------------*******-----------------------\n");
}

void polling_buffer(uint64_t first_buf_addr){				// 接收线程的功能 —— 轮询缓冲区

	int i, j;
	char *current_buf_addr;
	char *next_buf_addr;
	uint64_t busy;
	char *pkt;
	int tmp;

	struct timeval starttime, endtime, probe_time;
	int buf_cnt = 0;

	current_buf_addr = (char *)first_buf_addr;

//	filp = fopen("delay.txt", "w");
//	if(!filp)
//		return;
	// 线程休眠25ms，保证信息先发送到硬件
//	usleep(35000);
	i = 0;
	j = 0;
	// 从本线程对应的内存块的第一个缓冲区开始询问	因此第一个缓冲区的地址要作为参数传进来
//	gettimeofday(&starttime, 0);
//	printf("user wait....\n");
	while(1){
		
		// if(mprotect(current_buf_addr, 2048, PROT_READ | PROT_WRITE) == -1){
			// printf("error value %d\n", errno);
			// printf("msg: %s\n", strerror(errno));
			// printf("debug mprotect %d\n", err);
			// return;
		// }
	//	cacheflush(current_buf_addr, 2048, 0);
	//	msync(current_buf_addr, 64, MS_SYNC);

	//	rmb();
	//	dma_rmb();

		// busy位是否为1
		if(((struct cp_packet *)current_buf_addr)->cp.busy){
		// //	printf("%s\n", "send!!!");
			// buf_cnt ++;
			// gettimeofday(&probe_time, 0);
			// printf("               [%10ld.%ld]: %d : %p\n", probe_time.tv_sec, probe_time.tv_usec, buf_cnt, current_buf_addr);
			// current_buf_addr += 2048;
			// 打印报文
			//np_show_pkt(current_buf_addr);
			//break;
			
			
			// 取出下一buff的地址
			next_buf_addr = (char *)((struct cp_packet *)current_buf_addr)->cp.next;

			// 过滤TCP报文

			
			// printf("the origin pkt um.inport is %d\n", ((struct cp_packet *)current_buf_addr)->um.inport);
			// printf("the origin pkt um.outport is %d\n", ((struct cp_packet *)current_buf_addr)->um.outport);
			// printf("the origin pkt um.timestamp is %d\n", ((struct cp_packet *)current_buf_addr)->um.ts);
			// printf("the origin pkt um.dstmid is %d\n", ((struct cp_packet *)current_buf_addr)->um.dstmid);
			// printf("the origin pkt um.pktdst is %d\n", ((struct cp_packet *)current_buf_addr)->um.pktdst);
			// printf("the origin pkt um.length is %d\n", ((struct cp_packet *)current_buf_addr)->um.len);
			// printf("the origin pkt um.pkttype is %d\n", ((struct cp_packet *)current_buf_addr)->um.pkttype);
			// printf("the origin pkt cp.ctl is %d\n", ((struct cp_packet *)current_buf_addr)->cp.ctl);
			// printf("the origin pkt cp.pkt_len is %d\n", ((struct cp_packet *)current_buf_addr)->cp.pkt_len);
			// printf("the origin pkt length is %d\n", ((struct cp_packet *)current_buf_addr)->cp.pkt_len - 64);
			
		//	printf("the ethtype is %#x\n", ((struct cp_packet *)current_buf_addr)->um.ethtype);
		//	printf("the inport is %d\n", ((struct cp_packet *)current_buf_addr)->um.inport);
		//	printf("the outport is %d\n", ((struct cp_packet *)current_buf_addr)->um.outport);

			// 测试发送报文能否成功
			// 将报文缓冲区清零
		//	memset(((struct cp_packet *)current_buf_addr)->pkt_data, 0, ((struct cp_packet *)current_buf_addr)->cp.pkt_len - 64);
		//	上述代码会触发alignment fault，改用下面的代码实现
		//	for(j = 0; j < ((struct cp_packet *)current_buf_addr)->cp.pkt_len - 64; j ++){
		//		((struct cp_packet *)current_buf_addr)->pkt_data[j] = 0;
		//	}
			// 将icmp报文复制到报文缓冲区
		//	memcpy(((struct cp_packet *)current_buf_addr)->pkt_data, icmp_pkt, sizeof(icmp_pkt));
			// memcpy(((struct cp_packet *)current_buf_addr)->pkt_data, tcp_pkt, sizeof(tcp_pkt));
			// 设置cp_head的长度字段
		//	((struct cp_packet *)current_buf_addr)->cp.pkt_len = 64 + sizeof(icmp_pkt);
			// ((struct cp_packet *)current_buf_addr)->cp.pkt_len = 64 + sizeof(tcp_pkt);
			// 设置um_metadata的长度字段
		//	((struct cp_packet *)current_buf_addr)->um.len = 32 + sizeof(icmp_pkt);
			// ((struct cp_packet *)current_buf_addr)->um.len = 32 + sizeof(tcp_pkt);
			// 交换报文的um.inport和um.outport字段
			// tmp = ((struct cp_packet *)current_buf_addr)->um.inport;
			
			// 0 1 互转
			if(((struct cp_packet *)current_buf_addr)->um.inport == 0){
				((struct cp_packet *)current_buf_addr)->um.inport = ((struct cp_packet *)current_buf_addr)->um.outport;
				((struct cp_packet *)current_buf_addr)->um.outport = 1;
			}
			else if(((struct cp_packet *)current_buf_addr)->um.inport == 1){
				((struct cp_packet *)current_buf_addr)->um.inport = ((struct cp_packet *)current_buf_addr)->um.outport;
				((struct cp_packet *)current_buf_addr)->um.outport = 0;
			}

			// 设置um_metadata的输入端口字段
		//	((struct cp_packet *)current_buf_addr)->um.inport = ((struct cp_packet *)current_buf_addr)->um.outport;
			// 设置um_metadata的输出端口字段
		//	((struct cp_packet *)current_buf_addr)->um.outport = tmp;
			// 设置um_matadata的目标模块字段
			((struct cp_packet *)current_buf_addr)->um.dmid = 0x45;
			// 设置um_metadata的pkt_dse字段
			((struct cp_packet *)current_buf_addr)->um.pktdst = 0;

			// 设置cp_head的描述符控制字段
			// 发送报文要注释掉这一条
			((struct cp_packet *)current_buf_addr)->cp.ctl = 0;
			// 设置um_matadata的报文类型字段			
			((struct cp_packet *)current_buf_addr)->um.pkt_type = 0;

			// 将busy位置为0
			// 发送报文注释这一条
			((struct cp_packet*)current_buf_addr)->cp.busy = 0;



			// np_show_pkt(current_buf_addr);
			
			// 回收缓冲区：把该报文的第一个八字节写到BAR0空间的第一个寄存器
			// 这里也是向下发送报文
			bar0_registers->thread_ctl = ((struct cp_packet *)current_buf_addr)->cs.ctl;
			
			
			current_buf_addr = next_buf_addr;
/**/
/*
			gettimeofday(&endtime, 0);
			double timeuse = 1000000*(endtime.tv_sec - starttime.tv_sec) + endtime.tv_usec - starttime.tv_usec;
			timeuse /=1000000;

			fprintf(filp, "%lf\n", timeuse);

			gettimeofday(&starttime, 0);
*/
		}
	}

//	printf("total pkt num is %d\n", i);

	return;
}

int create_polling_thread(void){
	int temp;
	int i;
	memset(&thread, 0, sizeof(thread));		// 清零
	/*创建线程*/
	for(i = 0; i < THREAD_NUM; i++){
		temp = pthread_create(&thread[i], NULL, (void *)&polling_buffer, (void *)buf_addr_info->buf_addr[0].user_vir_addr);
	//	temp = pthread_create(&thread[i], NULL, (void *)&polling_buffer, (void *)buf_addr_info->buf_addr[ALLOC_HW_BUF_NUM].user_vir_addr);
		if(temp){
			printf("fail to create thread %d\n", i);
			goto err_thread_create;
		}else{
			printf("thread %d being created successfully\n", i);
		}
	}

err_thread_create:
	return -1;
}



static int np_hw_init(void){

	uint64_t hw_desc;			// 向硬件下发的地址描述符信息
	int i;

	/*------------------ 硬件复位 ------------------*/
	/*------------------ 调试需要，硬件复位目前放在内核态完成 ------------------*/
	// 向bar0_vir_base+0x028的位置写入1
	bar0_registers->hard_reset = 1;
	printf("np_hw_init000000000000\n");
	// 休眠1ms（实际休眠时间大于1ms）
	// 传入参数的单位是微秒
	usleep(1000);
	// 向bar0_vir_base+0x028的位置写入0
	bar0_registers->hard_reset = 0;
	// 向bar0_vir_base+0x020的位置写入7
	bar0_registers->port_clean = 7;
	// 休眠1ms（实际休眠时间大于1ms）
	usleep(1000);
	// 向bar0_vir_base+0x020的位置写入0
	bar0_registers->port_clean = 0;
/**/
	/*------------------向bar0_vir_base+0x018下发报文高32位虚拟地址信息------------------*/
	// 向寄存器写buf_addr_info结构体的成员变量uint64_t vir_addr_first的信息
	bar0_registers->virt_addr = buf_addr_info->vir_addr_first;

//	printf("%s\n", "write the virt_first addr!");

	/*------------------下发DMA空间描述符信息：21位物理地址+21位虚拟地址------------------*/
	for(i = 0; i < buf_addr_info->hw_buf_cnt; i++){
		// 用户空间虚拟地址左移32位
		hw_desc = buf_addr_info->buf_addr[i].user_vir_addr << 32;
		// 上述结果右移32位
		hw_desc = hw_desc >> 32;
		// 上述结果左移10位 与 总线地址右移11位后 做或操作
		hw_desc = (hw_desc << 10 | buf_addr_info->buf_addr[i].bus_addr >> 11);
		// 向0x10的位置写上述结果
		bar0_registers->hard_desc_dma = hw_desc;
	}

	/*------------------读取硬件配置信息------------------*/
	// 读取当前系统可用核数
	// 核数-1 为最大DMA通道数

	// 当前DMA通道数即虚拟端口数

	printf("%s\n", "hardware reset success!");
	return 0;
}

void soft_pkt_ring_init(void){
	uint8_t *current_buf_addr, *next_buf_addr;
	uint32_t current_dma_addr, next_dma_addr;
	struct cp_packet *pkt;
	int i,j;
	current_buf_addr = (uint8_t *)buf_addr_info->buf_addr[ALLOC_HW_BUF_NUM].user_vir_addr;
	current_dma_addr = buf_addr_info->buf_addr[ALLOC_HW_BUF_NUM].bus_addr;
	for(i = 0; i < 1024; i ++){
		if(i < 1023){
			next_buf_addr = current_buf_addr + 2048;
		}else{
			next_buf_addr = (uint8_t *)buf_addr_info->buf_addr[ALLOC_HW_BUF_NUM].user_vir_addr;
		}
		next_dma_addr = current_dma_addr + 2048;
		pkt = (struct cp_packet *)current_buf_addr;
		pkt->cp.next = (uint64_t)next_buf_addr;
		pkt->cp.dma = current_dma_addr >> 11;
		pkt->cp.pkt_offset = 66;
		pkt->cp.card_id = 15;
		pkt->cp.block_idx = 0;
//		
		memcpy(((struct cp_packet *)pkt)->pkt_data, tcp_pkt, sizeof(tcp_pkt));
		pkt->cp.pkt_len = 64 + sizeof(tcp_pkt);
		pkt->cp.busy = 1;
		pkt->um.len = 32 + sizeof(tcp_pkt);
		pkt->um.inport = 12;
		pkt->um.outport = 0;

//
	//	*(short *)pkt->pkt_data = 0x0923;
		pkt->cp.ctl = 1;

		current_buf_addr = next_buf_addr;
	}
//	for(j = 0; j < 2; j ++){
//		np_show_pkt(current_buf_addr + j * 2048);
//	}

}

int main(void){

	int np_pci_cdev_fd;

	void *user_addr;
	int addr_cnt, size_cnt;
	int i;
	uint64_t sys_ctl;
	char *pkt;


	np_pci_cdev_fd = open(NP_PCI_CDEV_FILE_PATH, O_RDWR);
	


	/*------------------ 将BAR0空间、buf_addr_info结构体、128块硬件管理的缓冲区和2块软件管理的缓冲区映射到用户空间 ------------------*/
	
	for(i = 0; i < ALLOC_HW_BUF_NUM + ALLOC_SW_BUF_NUM + 2; i++){
		// printf("%s\n", "++++++++++++++++++++ test0 ++++++++++++++++++++\n");
	//	传入i作为索引
		printf("mmap!!!\n");
		user_addr = mmap(NULL, (1 << 9) * getpagesize(), PROT_READ | PROT_WRITE, MAP_SHARED, np_pci_cdev_fd, i*getpagesize());
	//	使用调用次数作为索引
	//	user_addr = mmap(NULL, (1 << 9) * getpagesize(), PROT_READ | PROT_WRITE, MAP_SHARED, np_pci_cdev_fd, 0);
		if(user_addr == MAP_FAILED){
			printf("%s\n", "mmap fail");
		}
//		printf("%d %p\n", i, user_addr);
		// 把i>=2放在前面提高代码性能
		if(i >= 2){
			printf("buf_addr_info %d\n", i-2);
			buf_addr_info->buf_addr[i-2].user_vir_addr = (uint64_t)user_addr;
		//	g_buf_addr[i-2] = (uint64_t)user_addr;
		}
		else if(i == 0){
			// bar0空间没有2M，但是映射了2M
			// 用结构体的方式访问不会出错
			bar0_registers = (struct bar0_regs *)user_addr;
			printf("%s\n", "++++++++++++++++++++ test1 ++++++++++++++++++++\n");
		}
		else if(i == 1){
			// np_show_pkt((char *)user_addr);
			buf_addr_info = (struct buf_addr_info *)user_addr;
			printf("testset!!!\n");
		}
	/**/
	}
	
	printf("test---------------------------\n");


/*
	for(i = 0; i < buf_addr_info->hw_buf_cnt; i ++){
		printf("----------------- buf_addr[%d]'s bus_addr is 		%#lx -----------------\n", i, buf_addr_info->buf_addr[i].bus_addr);
		printf("----------------- buf_addr[%d]'s phy_addr is 		%#lx -----------------\n", i, buf_addr_info->buf_addr[i].phy_addr);
		printf("----------------- buf_addr[%d]'s user_vir_addr is 	%#lx -----------------\n", i, buf_addr_info->buf_addr[i].user_vir_addr);
	}
*/

	// 求用户空间虚拟地址打头信息
	// 求buf_addr_info结构体的成员变量uint64_t vir_addr_first的值
	// 第一个内存块的用户空间虚拟地址 异或 第一个内存块的物理地址
	// 上述结果再与 第一个内存块的物理地址 做或操作
	// 上述结果右移32位
	buf_addr_info->vir_addr_first = ((buf_addr_info->buf_addr[0].user_vir_addr ^ buf_addr_info->buf_addr[0].phy_addr) \
									 |  buf_addr_info->buf_addr[0].phy_addr) >> 32;
	printf("~~~~~~~~~~~~~~~~~~~~ the user space virt_first is %#lx ~~~~~~~~~~~~~~~~~~~~ \n", buf_addr_info->vir_addr_first);

	// 初始化软件缓冲区
//	soft_pkt_ring_init();

	np_hw_init();


	// 启动接收（轮询）线程
	create_polling_thread();

	// 线程启动后的准备工作
	// (g_npe_adapter->hw.use_dma_cnt) 当前使用DMA通道数 同虚拟端口的数目
	// NPE_HARD_TAG_NUM 32
	// 板卡大小端信息，np打印为1，即小端
	sys_ctl =(0 << 15) | (1/*os_endian*/ << 14) | (32/*NPE_HARD_TAG_NUM*/ << 6) | 1/*(g_npe_adapter->hw.use_dma_cnt)*/;
	bar0_registers->hard_sys_set = sys_ctl;
	usleep(1000);


	// 这里map_base的类型是char *
	// 一定要先把bar0_registers的类型转换为char *
	// 如果没有转换，bar0_registers + 0x80的意思就是加上0x80 * sizeof(struct bar0_regs) * 8个地址单元
	*(uint64_t *)((char *)bar0_registers + 0x80) = 2;
	usleep(1000);

//	int j = 0;
	// 构造报文并向下发送
//	pkt = (char *)buf_addr_info->buf_addr[ALLOC_HW_BUF_NUM].user_vir_addr;
//	memcpy(((struct cp_packet *)pkt)->pkt_data, icmp_pkt, sizeof(icmp_pkt));
//	((struct cp_packet *)pkt)->cp.pkt_len = 64 + sizeof(icmp_pkt);
//	((struct cp_packet *)pkt)->cp.busy = 1;
//	np_show_pkt(pkt);
//	while( j < 10){
		
		// 向下发送报文
//		bar0_registers->thread_ctl = ((struct cp_packet *)pkt)->cs.ctl;
//		usleep(20000);
//		// 将busy位置为0
//		((struct cp_packet *)pkt)->cp.busy = 0;
//		j ++;
//	}
	
	signal(SIGINT, handler);
//	printf("start OK!\n");
	// 等待所有轮询线程结束wait_polling_thread();
	if(thread[0] != 0){
		pthread_join(thread[0], NULL);
		printf("%s\n", "polling thread is over");
	}

/*	while(1)
	{
		sleep(300);
	}
*/	
	//for(i = 0; i < ALLOC_HW_BUF_NUM + ALLOC_SW_BUF_NUM + 2; i++){
		// printf("%s\n", "++++++++++++++++++++ test0 ++++++++++++++++++++\n");
		//if( i < ALLOC_HW_BUF_NUM + ALLOC_SW_BUF_NUM){
	//		munmap((void *)buf_addr_info->buf_addr[i].user_vir_addr, (1 << 9) * getpagesize());
	//	}else if(i == ALLOC_HW_BUF_NUM + ALLOC_SW_BUF_NUM){
	//		munmap((void *)bar0_registers, (1 << 9) * getpagesize());
	//	}else if(i == ALLOC_HW_BUF_NUM + ALLOC_SW_BUF_NUM + 1){
	//		munmap((void *)buf_addr_info, (1 << 9) * getpagesize());
	//	}

	//}
	//printf("ummap success\n");
	
	

	return 0;

}



// 结束驱动的时候
// 1、ummap
// 2、关闭/dev/uioX文件
