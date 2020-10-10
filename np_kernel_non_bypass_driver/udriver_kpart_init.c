/*
在probe函数中初始化了一个互斥锁
使得线程互斥地调用np_send()函数
*/


#include "udriver_kpart_init.h"
#include "udriver_pkt.h"
#include <linux/kthread.h>
#include <asm/uaccess.h>
/******************* 全局变量 *******************/

// 自定义的设备
static struct np_pci_cdev *np_pci_cdev;
// 字符设备的主设备号
dev_t cdev_no;
dev_t cdev_no_2;
// 字符设备的主设备号
struct cdev *my_cdev;
struct cdev *my_cdev_2;
static struct class *np_pci_cdev_class;
static struct device *dev;
static struct device *dev_2;

// 记录申请的所有页，释放时使用
static struct pages_info_4_release *pages_info_4_release;
// 记录地址三元组信息的结构体，会被映射到用户空间
static struct buf_addr_info *buf_addr_info;

static char* bar0_addr;

unsigned long g_pages_addr = 0;
// read
// 在申请128块硬件缓冲区时初始化
char *g_recv_ptr;
// write
// 在申请2块软件缓冲区时初始化
char *g_send_ptr;

// 互斥锁
// 使多个线程互斥地调用np_send()函数
struct mutex mtx;

// pci_device_id & pci_device
static struct pci_device_id np_pci_ids[] = {
	{PCI_DEVICE(0x0755,0x0755)},
	{0,},
};
MODULE_DEVICE_TABLE(pci, np_pci_ids);	// 热拔插的时候使用

// pci_driver
static struct pci_driver udriver_kpart = {
	.name 		= 	DRIVER_NAME,
	.id_table 	=  	np_pci_ids,
	.probe 		= 	np_kernel_probe,
	.remove 	= 	np_kernel_release,
};

/******************* 函数定义 *******************/
void npe_show_pkt(unsigned char *pkt)
{
        int i=0,len=0;
        printk("-----------------------*******-----------------------\n");
        printk("Packet Addr:%lX\n",(unsigned long)pkt);
        for(i=0;i<16;i++)
        {
                if(i % 16 == 0)
                        printk("      ");
                printk(" %X ",i);
                if(i % 16 == 15)
                        printk("\n");
        }
        len=256;
        //len=32;
        for(i=0;i<len;i++)
        {
                if(i % 16 == 0)
                        printk("%04X: ",i);
                printk("%02X ",*(pkt+i));
                if(i % 16 == 15)
                        printk("\n");
        }
        if(len % 16 !=0)
                printk("\n");
        printk("-----------------------*******-----------------------\n\n");
}

void send_pkt(struct cp_packet *pkt, int ctl){
	
	if (pkt->um.inport == 0) {
	//	pkt->um.inport = pkt->um.outport;
		pkt->um.inport = 12;
		pkt->um.outport = 1;
	}
	else if (pkt->um.inport == 1) {
	//	pkt->um.inport = pkt->um.outport;
		pkt->um.inport = 12;
		pkt->um.outport = 0;
	}

	// 设置um_matadata的目标模块字段
	pkt->um.dmid = 0x45;
	// 设置um_metadata的pkt_dse字段
	pkt->um.pktdst = 0;

	// 设置cp_head的描述符控制字段
	// 根据枚举类型设置
	pkt->cp.ctl = ctl;
	// 设置um_matadata的报文类型字段			
	pkt->um.pkt_type = 0;

	// 将busy位置为0
	pkt->cp.busy = 0;

	mutex_lock(&mtx);
	// 回收缓冲区：
	// 把该报文的第一个八字节写到BAR0空间的第一个寄存器
	writeq(pkt->cs.ctl, bar0_addr);
	mutex_unlock(&mtx);
}

// 接收函数
// 将设备内存中的数据拷贝到用户空间
// 0：缓冲区内有报文； -3：缓冲区内无报文
static ssize_t np_pci_cdev_read(struct file *filp, 
							char __user *user_ptr, 
							size_t len, loff_t *off){
	struct cp_packet* pkt;							
	
	// 检查用户地址是否合法
	if(!access_ok(VERIFY_READ, user_ptr, len))
		return -1;
	
	pkt = (struct cp_packet*)g_recv_ptr;
	
	// 缓冲区内有报文
	if(pkt->cp.busy == 1){
		if(__copy_to_user((void __user *)user_ptr, 
					(const void *)g_recv_ptr, len)){
			return -2;	// 拷贝失败
		}
		send_pkt((struct cp_packet *)g_recv_ptr, NPE_CTR_DROP);
		g_recv_ptr = (char *)pkt->cp.next;
		return 0;
	}
	else{
		return -3;
	}
}

// 发送函数
// 将用户空间的数据拷贝到设备内存
static ssize_t np_pci_cdev_write(struct file *filp, 
							char __user *user_ptr, 
							size_t len, loff_t *off){
	struct cp_packet* pkt;							
	
	// 检查用户地址是否合法
	if(!access_ok(VERIFY_WRITE, user_ptr, len))
		return -1;
	
	pkt = (struct cp_packet*)g_send_ptr;
//	npe_show_pkt((char *)pkt);
	if(__copy_from_user((void *)g_send_ptr,
						(void __user *)user_ptr, len)){
		printk(KERN_ERR "send func: copy err\n");
		return -2;
	}
//	npe_show_pkt((char *)pkt);
	// 发送报文
	send_pkt((struct cp_packet *)g_send_ptr, NPE_CTR_OSND);
	g_send_ptr = (char *)pkt->cp.next;
	return 0;
}

// static int np_pci_cdev_open(struct inode *inode, struct file *filp){
	// // 使设备文件的私有数据指向将全局变量np_pci_cdev
	// struct np_pci_cdev *my_np_pci_cdev;	
	// my_np_pci_cdev = container_of(inode->i_cdev, 
								// struct np_pci_cdev, cdev);
	// filp->private_data = my_np_pci_cdev;
	// return 0;
// }


static const struct file_operations np_pci_cdev_fops = {
	.owner 	= 	THIS_MODULE,
//	.open 	= 	np_pci_cdev_open,
	.read 	= 	np_pci_cdev_read,
	.write 	= 	np_pci_cdev_write,
};

// 把申请的页的信息记录到struct pages_info_4_release结构体当中
static void record_pages_info(struct pages_info_4_release *p_release, uint64_t base_address, uint64_t bus_address, uint64_t pages_order, uint64_t dma_flag){
	uint64_t i = p_release->cnt;
	p_release->p_info[i].base_addr = base_address;
	p_release->p_info[i].bus_addr = bus_address;
	p_release->p_info[i].order = pages_order;
	p_release->p_info[i].is_dma_map = dma_flag;
	p_release->cnt ++;
}

// 申请内存，内存驻留，内存清零
// dma_flag 1:有GFP_DMA 0:无GFP_DMA
static uint64_t get_pages(uint64_t pages_order, uint8_t dma_flag){	
	uint64_t pages_base = 0, pages_size = 0;
	if(dma_flag == 1){
		pages_base = __get_free_pages(GFP_KERNEL | __GFP_ZERO | GFP_DMA, pages_order);
	}else if(dma_flag == 0){
		pages_base = __get_free_pages(GFP_KERNEL | __GFP_ZERO, pages_order);
	}

	if(pages_base == 0){
		return -1;
	}
	
	SetPageReserved(virt_to_page(pages_base));
	pages_size = (1 << pages_order) * PAGE_SIZE;
	memset((char*)pages_base, 0, pages_size);

	return pages_base;
}

void np_alloc_cdev(void){
	// 申请并注册字符设备，创建设备文件
	// 申请设备号
	// 最后一个参数可理解为该设备属于np_pci_cedv类
	alloc_chrdev_region(&cdev_no, 0, 1, "np_pci_cdev");
	alloc_chrdev_region(&cdev_no_2, 0, 1, "np_pci_cdev");
	// 在/sys/class/目录下创建np_pci_cdev类
	np_pci_cdev_class = class_create(THIS_MODULE, "np_pci_cdev");
	// 在/dev/目录下创建np_pci_cdev0设备
	dev = device_create(np_pci_cdev_class, 
						NULL, cdev_no, NULL, "np_pci_cdev0");
	dev_2 = device_create(np_pci_cdev_class, 
						NULL, cdev_no_2, NULL, "np_pci_cdev1");
	// 分配设备结构，初始化并添加cdev结构体
	my_cdev = cdev_alloc();
	my_cdev_2 = cdev_alloc();
	
	my_cdev->ops = &np_pci_cdev_fops;
	my_cdev_2->ops = &np_pci_cdev_fops;
	
	cdev_init(my_cdev, &np_pci_cdev_fops);
	cdev_init(my_cdev_2, &np_pci_cdev_fops);
	
	cdev_add(my_cdev, cdev_no, 1);
	cdev_add(my_cdev_2, cdev_no_2, 1);
	
	// 设备文件被打开后，使设备文件的私有数据指向这个结构体
	// 在mmap中，根据设备文件的私有数据，得到这个结构体
//	np_pci_cdev->cdev = *my_cdev;
//	cdev_init(&np_pci_cdev->cdev, &np_pci_cdev_fops);
//	
//	np_pci_cdev->cdev.owner = THIS_MODULE;
//	
//	cdev_add(&np_pci_cdev->cdev, cdev_no, 1);
}

// 申请缓冲区
static int np_alloc_buffs(struct pci_dev *pdev){

	uint64_t pages_addr, phy_addr, bus_addr, pages_size;
	uint8_t i;				// 缓冲区计数器
	// 需要mmap的内存块计数器

	// 申请16k存放pages_info_4_release结构体
	if((pages_addr = get_pages(ORDER_OF_PAGES_INFO_4_RELEASE, 0)) == -1){
		goto err_sw_init;
	}
	pages_info_4_release = (struct pages_info_4_release *)pages_addr;
	//全局变量pages_info_4_release初始化
	pages_info_4_release->cnt = 0;

	// 申请2M内存存放地址信息的结构体
	if((pages_addr = get_pages(ORDER_OF_BUF_ADDR_INFO, NOT_DMA_FLAG)) == -1){
		goto err_sw_init;
	};
	
	record_pages_info(pages_info_4_release, pages_addr, 0, ORDER_OF_BUF_ADDR_INFO, NOT_DMA_FLAG);
	buf_addr_info = (struct buf_addr_info *)pages_addr;
	phy_addr = virt_to_phys((void *)pages_addr);
	// 全局变量buf_addr_info初始化
	buf_addr_info->buf_cnt = 0;
	buf_addr_info->sw_buf_cnt = 0;
	buf_addr_info->hw_buf_cnt = 0;

	// 申请128块硬件缓冲区 + 128块软件缓冲区
	// 规定前128块为硬件缓冲区，后128块为软件缓冲区
	for(i = 0; i < ALLOC_HW_BUF_NUM + ALLOC_SW_BUF_NUM; i++){
		if((pages_addr = get_pages(ORDER_OF_BUF, DMA_FLAG)) == 0){
			printk("hard dma buff gets error in func get_pages\n");
			goto err_sw_init;
		}
		
		// 初始化接收和发送报文的指针
		if(i == 0)
			g_recv_ptr = (char *)pages_addr;
		//	g_pages_addr = pages_addr;
		if(i == ALLOC_HW_BUF_NUM)
			g_send_ptr = (char *)pages_addr;
		
		phy_addr = virt_to_phys((void *)pages_addr);
		pages_size = (1 << ORDER_OF_BUF)* PAGE_SIZE;
		printk(KERN_ERR "before dma_map_single %d\n", i);
		bus_addr = dma_map_single(&(pdev->dev), 
								(void *)pages_addr, 
								pages_size, 
								DMA_BIDIRECTIONAL);
		printk(KERN_ERR "after dma_map_single %d\n", i);
		
		record_pages_info(pages_info_4_release, pages_addr, bus_addr, ORDER_OF_BUF, DMA_FLAG);

		// 缓冲区的地址对信息记录在全局变量buff_addr_info当中
		buf_addr_info->buf_cnt ++;
		if( i < 128)
			buf_addr_info->hw_buf_cnt ++;
		else
			buf_addr_info->sw_buf_cnt ++;
		
		buf_addr_info->buf_addr[i].bus_addr = bus_addr;
	//	printk("----------------------the bus addr is %#llx----------------------\n",buf_addr_info->buf_addr[i].bus_addr);
		buf_addr_info->buf_addr[i].phy_addr = phy_addr;
		printk("----------------------%d the phy addr is %#llx----------------------\n",i, buf_addr_info->buf_addr[i].phy_addr);
		buf_addr_info->buf_addr[i].user_vir_addr = pages_addr;
	//	printk("----------------------the kernel vir addr is %#llx----------------------\n", buf_addr_info->buf_addr[i].user_vir_addr);

	}
	// 申请的数量与头文件里定义的数量是否相同
	if(buf_addr_info->hw_buf_cnt != ALLOC_HW_BUF_NUM || 
		buf_addr_info->sw_buf_cnt != ALLOC_SW_BUF_NUM){
		printk("buf_addr_info->hw_buf_cnt != ALLOC_HW_BUF_NUM or \
			buf_addr_info->sw_buf_cnt != ALLOC_SW_BUF_NUM");
		return -1;
	}

	return 0;

err_sw_init:
	free_all_pages(pdev, pages_info_4_release);
	return -1;
}

void np_init_send_buf_ring(void){
	uint8_t *current_buf_addr, *next_buf_addr;
	uint32_t current_dma_addr, next_dma_addr;
	struct cp_packet *pkt;
	int i,j;
	printk(KERN_ERR "current_vir_addr is %llx\n", buf_addr_info->
						buf_addr[ALLOC_HW_BUF_NUM].user_vir_addr);
	current_buf_addr = 
		(uint8_t *)buf_addr_info-> \
		buf_addr[ALLOC_HW_BUF_NUM].user_vir_addr;
	current_dma_addr = 
		buf_addr_info-> \
		buf_addr[ALLOC_HW_BUF_NUM].bus_addr;
	for(i = ALLOC_HW_BUF_NUM; 
		i < ALLOC_HW_BUF_NUM + ALLOC_SW_BUF_NUM; 
		i ++){
		for(j = 0; j < 1024; j ++){
			if(j < 1023){
				next_buf_addr = current_buf_addr + 2048;
				next_dma_addr = current_dma_addr + 2048;
			}
			// 下一个128M软件块
			else if(j == 1023 && 
					i < ALLOC_HW_BUF_NUM + ALLOC_SW_BUF_NUM - 1){
				next_buf_addr = 
					(uint8_t *)buf_addr_info-> \
					buf_addr[i + 1].user_vir_addr;
				next_dma_addr = 
					buf_addr_info-> \
					buf_addr[i + 1].bus_addr;
			}
			// 又环回到第一个（第128个）128M软件块
			else if(j == 1023 && 
					i == ALLOC_HW_BUF_NUM + ALLOC_SW_BUF_NUM - 1){
				next_buf_addr = 
					(uint8_t *)buf_addr_info-> \
					buf_addr[ALLOC_HW_BUF_NUM].user_vir_addr;
				next_dma_addr = 
					buf_addr_info-> \
					buf_addr[ALLOC_HW_BUF_NUM].bus_addr;
			}
			
			pkt = (struct cp_packet *)current_buf_addr;
			pkt->cp.next = (uint64_t)next_buf_addr;
			pkt->cp.dma = current_dma_addr >> 11;
			pkt->cp.pkt_offset = 64;
			pkt->cp.card_id = 15;
			pkt->cp.block_idx = 0;
			pkt->cp.ctl = 1;
			
			current_buf_addr = next_buf_addr;
		}
	}
}

void np_hw_reset(void){
	uint64_t hw_desc;
	int i;
	writeq(1, bar0_addr + 0x028);
	msleep(1);
	writeq(0, bar0_addr + 0x028);
	
	writeq(7, bar0_addr + 0x020);
	msleep(1);
	writeq(0, bar0_addr + 0x020);

	// 向bar0_vir_base+0x018下发报文高32位虚拟地址信息
	// 下发virt_first
	buf_addr_info->vir_addr_first = 
		((buf_addr_info->buf_addr[0].user_vir_addr ^ 
		buf_addr_info->buf_addr[0].phy_addr) 
		| buf_addr_info->buf_addr[0].phy_addr) >> 32;

	// 向寄存器写buf_addr_info结构体的成员变量
	// uint64_t vir_addr_first的信息
	writeq(buf_addr_info->vir_addr_first, bar0_addr + 0x018);

	// 下发DMA空间描述符信息：21位物理地址+21位虚拟地址
	for (i = 0; i < ALLOC_HW_BUF_NUM; i++) {
		// 内核空间虚拟地址左移32位
		hw_desc = buf_addr_info->buf_addr[i].user_vir_addr << 32;
		// 上述结果右移32位
		hw_desc = hw_desc >> 32;
		// 上述结果左移10位 与 总线地址右移11位后 做或操作
		hw_desc = (hw_desc << 10 | \
				buf_addr_info->buf_addr[i].bus_addr >> 11);
		// 向0x10的位置写上述结果
		writeq(hw_desc, bar0_addr + 0x010);
	}
	
	printk("%s\n", "hardware reset success!");
	return;
}

void np_hw_start_up(void){
	uint64_t sys_ctl;
	// 下发控制信息   
	// 1：os_endian     
	// 32：NPE_HARD_TAG_NUM  
	// 1：g_npe_adapter->hw.use_dma_cnt
	sys_ctl = (0 << 15) | (1 << 14) | (32 << 6) | 1;
	writeq(sys_ctl, bar0_addr + 0x008);
	msleep(1);

	writeq(2, bar0_addr + 0x080);
	msleep(1);
	
	return;
}

static int np_kernel_probe(struct pci_dev *pdev,
							const struct pci_device_id *pid_tbl){

	// io内存资源的存储器域物理地址、长度及内核空间地址
	uint64_t pci_io_base, pci_io_len;
	int err;


	if((err = pci_enable_device(pdev))){
		printk("-----------------------pci device \
		enable fail!-----------------------\n");
		return err;
	}
	if((err = pci_request_regions(pdev, DRIVER_NAME))){
		printk("-----------------------pci device \
		request region fail!-----------------------\n");
		return err;
	}
	pci_set_master(pdev);
	printk("-----------------------pci device \
	enable success!-----------------------\n");
	
	
	// 读取BARO空间地址（存储器域物理地址）及长度
	pci_io_base = pci_resource_start(pdev, 0);
	pci_io_len = pci_resource_len(pdev, 0);
	// 将BAR0空间映射到内核地址空间
	bar0_addr = ioremap(pci_io_base, pci_io_len);

	// 为自定义设备np_pci_cdev申请内存
	np_pci_cdev = kzalloc(sizeof(struct np_pci_cdev), GFP_KERNEL);
	// 初始化np_pci_cdev->mem_info_cnt
	np_pci_cdev->mem_info_cnt = 0;

	// 注册字符设备
	np_alloc_cdev();
	printk(KERN_ERR "alloc sdev successfully\n");

	// 申请缓冲区
	np_alloc_buffs(pdev);	
	printk(KERN_ERR "alloc buffers successfully\n");
	
	// 初始化软件管理的缓冲区（发送报文的缓冲区）
	np_init_send_buf_ring();
	
	// 用默认属性初始化互斥锁
	mutex_init(&mtx);

	// 硬件复位 下发地址
	np_hw_reset();
	
	// 硬件开始接收报文
	np_hw_start_up();

	return 0;
}

// 释放申请的所有内存资源
static void free_all_pages(struct pci_dev *pdev, 
						struct pages_info_4_release *p_release){
	int i;
	uint64_t page_size;
	uint64_t base_addr;
	uint64_t bus_addr;
	uint64_t page_reserve;

	for(i = 0; i < p_release->cnt; i++){
		page_size = (1 << p_release->p_info[i].order) * PAGE_SIZE;
		base_addr = p_release->p_info[i].base_addr;
		bus_addr = p_release->p_info[i].bus_addr;
		// 解除dma映射
		if(p_release->p_info[i].is_dma_map == 1){
			dma_unmap_single(&(pdev->dev), bus_addr, 
							page_size, DMA_BIDIRECTIONAL);
		}
		// 解除页保留
		for(page_reserve = base_addr; 
			page_reserve < base_addr + page_size; 
			page_reserve += PAGE_SIZE){
			ClearPageReserved(virt_to_page(page_reserve));
		}
		// 释放页
		free_pages(p_release->p_info[i].base_addr, 
					p_release->p_info[i].order);
	}

	// 释放记录pages_info_4_release结构体的页
	page_size = (1 << ORDER_OF_PAGES_INFO_4_RELEASE) * PAGE_SIZE;
	for(page_reserve = (uint64_t)p_release; 
		page_reserve < (uint64_t)p_release + page_size; 
		page_reserve += PAGE_SIZE){
			ClearPageReserved(virt_to_page(page_reserve));
	}
	free_pages((uint64_t)p_release, ORDER_OF_PAGES_INFO_4_RELEASE);
}

void np_pci_dev_release(struct pci_dev *pdev){
	// 对应pci_requesr_regions()	
	pci_release_regions(pdev);	
	// 对应pci_enable_device()
	pci_disable_device(pdev);	
}

// 注销字符设备及其类
void np_cdev_release(void){
	// 对应alloc_chrdev_region()	
	unregister_chrdev_region(cdev_no, 1);		
	unregister_chrdev_region(cdev_no_2, 1);	
	// 对应cdev_add()
	cdev_del(my_cdev);
	cdev_del(my_cdev_2);
	// 对应kzalloc()	
	kfree(np_pci_cdev);	
	// 对应device_create()
	device_destroy(np_pci_cdev_class, cdev_no);	
	device_destroy(np_pci_cdev_class, cdev_no_2);	
	// 对应class_create()
	class_destroy(np_pci_cdev_class);
}

static void np_kernel_release(struct pci_dev *pdev){
	
	// 释放所有申请的页
	free_all_pages(pdev, pages_info_4_release);	
	
	// 释放pci设备相关资源
	np_pci_dev_release(pdev);
	
	// 释放字符设备
	np_cdev_release();	
}

static int __init udriver_init_module(void){
	pci_register_driver(&udriver_kpart);
	return 0;
}

static void __exit udriver_cleanup_module(void){
	pci_unregister_driver(&udriver_kpart);
}

module_init(udriver_init_module);
module_exit(udriver_cleanup_module);
MODULE_LICENSE("GPL");
