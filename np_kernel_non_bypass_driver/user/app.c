#include <stdio.h>
#include <string.h>
#include <signal.h>
#include "user_pkt_transfer.h"
#include "udriver_usrpart_pkt.h"

// 接收和发送的文件句柄
int fd_snd, fd_rcv;
// 描述符环
// 环大小为2048
struct desc_ring *dr;
// 收发报文的指针
// struct desc *snd;

// 用于统计当前剩余缓冲区数量
uint32_t rcv_cnt;
uint32_t snd_cnt;

// 收发线程
pthread_t s_thrd, r_thrd;
// 描述符个数计数器
pthread_t cnt_thrd;

// 初始化描述符环
// 环大小为DESC_NUM2048
struct desc* np_init_desc_ring(void){
	struct desc *head, *current, *next;
	int i;
//	printf("the size of struct desc is %lu\n", sizeof(struct desc));
	head = (struct desc *)malloc(sizeof(struct desc));
	current = head;
	memset(current, 0, sizeof(struct desc));
	for(i = 0; i < DESC_NUM; i ++ ){
		next = (struct desc *)malloc(sizeof(struct desc));
		memset(next, 0, sizeof(struct desc));
		if(i < DESC_NUM-1){
			current->nxt = next;
		}else if(i == DESC_NUM-1)
		{
			current->nxt = head;
		}
		current = current->nxt;	
	}
	return head;
}

// write - send
// 0:success
// 文件描述符是全局变量
int np_write(char *user_ptr){
	int err;
	
	if(fd_snd < 0){
		printf("fail to open the send file\n");
		return -1;
	}
	err = write(fd_snd, user_ptr, 2048);
	if(err != 0){
		printf("fail to send pkt\n");
	}
	return err;
}

// read - recv
// 阻塞式读
// 0：接收成功
// -1：用户地址不合法
// -2：拷贝失败
// -3：缓冲区内无报文
// 内核中的read函数不是阻塞式的
// 当当前轮询的缓冲区无报文时，会立即返回
// 内核中的全局接收指针不会移动
// 文件描述符是全局变量
int np_read(char *user_ptr){
	int err;
		
	if(fd_rcv < 0){
		printf("fail to open the recv file\n");
		return -1;
	}
//	printf("start read ...\n");
	err = read(fd_rcv, user_ptr, 2048);
//	while(err != 0){
//		err = read(fd_rcv, user_ptr, 2048);
//	}
	return err;
}

// 发送报文的线程所执行的函数
// struct desc_ring是一个由发送和接收线程共同访问的结构体
// 在主函数中声明为全局变量
void np_send(){
	int err;
	struct desc *snd;
	
	snd_cnt = 0;
	snd = dr->snd;
	while(1){
		// 禁止同时读写某片内存
	//	if(dr->snd != dr->rcv){
			// 该描述符上有报文
			if(snd->busy == 1){
			//	printf("send!!\n");
				// 发送报文
				err = np_write((char *)(snd->pkt));
				// 释放pkt内存
				free((void *)snd->pkt);
				snd->pkt = 0;
				// 释放该描述符
				snd->busy = 0;
				// 发送指针后移
				snd = snd->nxt;
				dr->snd = snd;
				snd_cnt ++;
			}
	//	}
	}
}

// 接收报文的线程所执行的函数
// 从网卡接收上来的报文挂在描述符环上
// struct desc_ring是一个由发送和接收线程共同访问的结构体
// 在主函数中声明为全局变量
void np_recv(){
	int err;
	struct cp_packet *pkt;
	struct desc *rcv;
	
	rcv_cnt = 0;
//	printf("this is the np_recv func\n");
	rcv = dr->rcv;
//	printf("the rcv addr is %p\n", rcv);
//	printf("the rcv addr is %p\n", dr->snd);
//	if(dr->rcv != dr->snd){
//		printf("true\n");
//	}else{
//		printf("false\n");
//	}
	while(1){
	//	printf("this is the np_recv func\n");
		// 禁止同时读写某片内存
//		if(dr->rcv != dr->snd){
//			printf("busy status is %d\n", rcv->busy);
			// 该描述符是否空闲
			// 若空闲
			if(rcv->busy == 0){
//				printf("recv!!\n");
				pkt = malloc(PKT_SIZE);
				memset(pkt, 0, PKT_SIZE);
				// 接收报文
				err = np_read((char *)pkt);
				
				// 接收成功
				if(err == 0){
					// 把报文挂载描述符环上
					rcv->pkt = pkt;
				//	np_show_pkt((char *)pkt);
				//	usleep(1000000);
					rcv->busy = 1;
					// 接收指针后移
					rcv = rcv->nxt;
					dr->rcv = rcv;
					
					rcv_cnt ++;
				}
				// 接收失败
				else{
				//	printf("errno : %d", err);
					free((void *)pkt);
					pkt = 0;
				}
			}
//		}
	}
}

void np_desc_cnt(){
	uint32_t desc_num;
//	printf("this is np_desc_cnt\n");
	while(1){
		// 每隔2s打印一次剩余描述符
		usleep(10000000);
		
		desc_num = DESC_NUM - (rcv_cnt - snd_cnt);
		printf("remaining desc: %d\n", desc_num);
	//	if(dr->rcv != dr->snd){
	//		printf("dr->rcv != dr->snd\n");
	//	}else{
	//		printf("dr->rcv == dr->snd\n");
	//	}
	//	printf("rcv->busy %d\n", dr->rcv->busy);
	//	printf("snd->busy %d\n", dr->snd->busy);
	}
}

// 创建接收和发送线程
// 将收发线程通过传进来的参数返回给调用函数
int create_transfer_thread(){
	int err;
	printf("before init the thread\n");
	memset(&s_thrd, 0, sizeof(s_thrd));
	memset(&r_thrd, 0, sizeof(r_thrd));
	memset(&cnt_thrd, 0, sizeof(cnt_thrd));
	printf("after init the thread\n");
	// 先创建接收线程
	err = pthread_create(&r_thrd, NULL, 
							(void *)&np_recv, 
							NULL);
	if(err){
		printf("fail to create recv thread\n");
		goto err_thread_create;
	}
	else{
		printf("rcv thread being created successfully\n");
	}
	// 再创建发送线程
	err = pthread_create(&s_thrd, NULL, 
							(void *)&np_send, 
							NULL);
	if(err){
		printf("fail to create send thread\n");
		goto err_thread_create;
	}
	else{
		printf("snd thread being created successfully\n");
	}
/**/
	
	err = pthread_create(&cnt_thrd, NULL,
							(void *)np_desc_cnt,
							NULL);
	if(err){
		printf("fail to create desc cnt thread\n");
		goto err_thread_create;
	}
	else{
		printf("desc cnt thread being created successfully\n");
	}
	
	printf("creating the thread successfully\n");
	return 0;
err_thread_create:
	return -1;
}

void handler(int sig){
	if(fd_snd)
		close(fd_snd);
	if(fd_rcv)
		close(fd_rcv);
	printf("%s\n", "transferring is over!!!");
	exit(0);
}

int main(){
	int err;
	
//	printf("size of cp_packet %d\n", sizeof(struct cp_packet));
//	struct cp_packet *pkt = malloc(PKT_SIZE);
	
	// 打开收发文件
	fd_snd = open(NP_PCI_CDEV_FILE_PATH_send, O_RDWR);
	if(fd_snd < 0){
		printf("fail to open fd_snd\n");
	}
	fd_rcv = open(NP_PCI_CDEV_FILE_PATH_recv, O_RDWR);
	if(fd_rcv < 0){
		printf("fail to open fd_rcv");
	}

	// 初始化描述符环
	dr = (struct desc_ring *)malloc(sizeof(struct desc_ring));
	dr->ring = np_init_desc_ring();
	printf("the ring addr is %p\n", dr->ring);
	dr->d_num = DESC_NUM;
//	printf("%d\n", dr->d_num);
	dr->snd = dr->ring;
	printf("the send ptr addr is %p\n", dr->snd);
	dr->rcv = dr->ring;
	printf("the recv ptr addr is %p\n", dr->rcv);
	
	
	
	printf("before start up the transferring thread\n");
	// 初始化并启动收发线程
	err = create_transfer_thread(dr);
	printf("after start up the transfering thread\n");
	
	signal(SIGINT, handler);
	
	if(s_thrd != 0 && r_thrd != 0 && cnt_thrd != 0){
		pthread_join(s_thrd, NULL);
		pthread_join(r_thrd, NULL);
		pthread_join(cnt_thrd, NULL);
		printf("%s\n", "polling thread is over");
	}
/**/
/*	
	fd_snd = open(NP_PCI_CDEV_FILE_PATH_send, O_RDWR);
	fd_rcv = open(NP_PCI_CDEV_FILE_PATH_recv, O_RDWR);
	
	
	
//	memset(pkt, 0xf, PKT_SIZE);	
	while(1){
		err_recv = np_recv(fd_recv, (char *)pkt);
		// 成功收到报文
		if(err_recv == 0){
			// show pkt
		//	np_show_pkt((char *)pkt);
			err_send = np_send(fd_send, (char *)pkt);
		}else{
			printf("fail to recv a pkt\n");
		}
	}
//	memcpy(pkt->pkt_data, tcp_pkt, sizeof(tcp_pkt));
//	err = send((char *)pkt);
//	if(err == 0){
		// show pkt
//		np_show_pkt((char *)pkt);
//	}else{
//		printf("fail to send a pkt\n");
//	}

	signal(SIGINT, handler);
*/	
	printf("the main func is over\n");
	return 0;
}
