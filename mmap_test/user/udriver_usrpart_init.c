#include "udriver_mmap_struct.h"
#include "udriver_usrpart_init.h"
#include <pthread.h>
#include <sys/time.h>
#include <sys/wait.h>


/******************* 全局变量 *******************/
// 记录地址三元组信息的结构体，指向map映射到用户空间的地址
struct buf_addr_info *buf_addr_info;
uint64_t g_user_addr;
// 线程标识
pthread_t thread[THREAD_NUM];			// 一个线程


void handler(int sig){
	
	printf("%s\n", "polling is over!!!");
	exit(0);
}


void polling_buffer(uint64_t first_buf_addr){	

	int i, j;
	char *buf;
	uint64_t busy;
	char *pkt;

	struct timeval probe_time;
	int buf_cnt = 0;

	buf = (char *)first_buf_addr;

	i = 0;
	j = 0;
	while(1){

		// busy位是否为1
		if(buf[0] == 1){
			buf_cnt ++;
			gettimeofday(&probe_time, 0);
			printf("               [sec: %10ld usec: %ld]: %d : %p\n", probe_time.tv_sec, probe_time.tv_usec, buf_cnt, buf);
			buf += 2048;

		}
	}


	return;
}

int create_polling_thread(void){
	int temp;
	int i;
	memset(&thread, 0, sizeof(thread));		// 清零
	/*创建线程*/
	for(i = 0; i < THREAD_NUM; i++){
	//	temp = pthread_create(&thread[i], NULL, (void *)&polling_buffer, (void *)buf_addr_info->buf_addr[0].user_vir_addr);
		temp = pthread_create(&thread[i], NULL, (void *)&polling_buffer, (void *)g_user_addr);
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

int main(void){

	int np_pci_cdev_fd;

	void *user_addr;
	int addr_cnt, size_cnt;
	int i;
	char *pkt;



	np_pci_cdev_fd = open(NP_PCI_CDEV_FILE_PATH, O_RDWR | O_SYNC);
	


	/*------------------ 将BAR0空间、buf_addr_info结构体、128块硬件管理的缓冲区和2块软件管理的缓冲区映射到用户空间 ------------------*/
	
	for(i = 0; i < ALLOC_HW_BUF_NUM + ALLOC_SW_BUF_NUM; i++){
		// printf("%s\n", "++++++++++++++++++++ test0 ++++++++++++++++++++\n");
	//	传入i作为索引
		user_addr = mmap((void *)0x06100000, (1 << 9) * getpagesize(), PROT_READ | PROT_WRITE, MAP_SHARED, np_pci_cdev_fd, (i+1)*getpagesize());
		printf("user addr is %p\n", user_addr);
		if(user_addr == MAP_FAILED){
			printf("%s\n", "mmap fail");
		}
		g_user_addr = (uint64_t)user_addr;
/*
//		printf("%d %p\n", i, user_addr);
		// 把i>=2放在前面提高代码性能
		if(i >= 1){
		//	buf_addr_info->buf_addr[i-1].user_vir_addr = (uint64_t)user_addr;
			g_user_addr = (uint64_t)user_addr;
		}else if(i == 0){
			// np_show_pkt((char *)user_addr);
		//	buf_addr_info = (struct buf_addr_info *)user_addr;

		}
*/	}
	

	// 启动接收（轮询）线程
	create_polling_thread();


	signal(SIGINT, handler);

	// 等待所有轮询线程结束wait_polling_thread();
	if(thread[0] != 0){
		pthread_join(thread[0], NULL);
		printf("%s\n", "polling thread is over");
	}


}

