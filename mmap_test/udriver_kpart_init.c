#include "udriver_kpart_init.h"
#include <linux/kthread.h>
/******************* 全局变量 *******************/

unsigned long g_pages_addr = 0;
// 自定义的设备
static struct np_pci_cdev *np_pci_cdev;
// 字符设备的主设备号
dev_t cdev_no;
// 记录申请的所有页，释放时使用
static struct pages_info_4_release *pages_info_4_release;
// 记录地址三元组信息的结构体，会被映射到用户空间
static struct buf_addr_info *buf_addr_info;

static struct class *np_pci_cdev_class;
static struct device *dev;

/******************* 函数定义 *******************/

static int np_pci_cdev_open(struct inode *inode, struct file *filp){
	// 使设备文件的私有数据指向将全局变量np_pci_cdev
	struct np_pci_cdev *my_np_pci_cdev;	
	my_np_pci_cdev = container_of(inode->i_cdev, struct np_pci_cdev, cdev);
	filp->private_data = my_np_pci_cdev;
	return 0;
}

static int np_pci_cdev_mmap(struct file *filp, struct vm_area_struct *vma){

	struct np_pci_cdev *my_np_pci_cdev;
	struct np_pci_cdev_mem *mem_info;
	uint8_t mem_index;								

	// 根据设备文件的私有数据指针找到全局变量np_pci_cdev
	my_np_pci_cdev = filp->private_data;
	mem_info = my_np_pci_cdev->np_pci_cdev_mem_info;

	if (vma->vm_end < vma->vm_start)
		return -1;

	// 用户传入的偏移作为内存块的索引
	mem_index = vma->vm_pgoff;
	if(mem_index >= my_np_pci_cdev->mem_info_cnt){
		printk("******************** please check the offset variable ********************\n");
		return -1;
	}

	// 检查传入的物理地址是否与页大小对齐
	if (mem_info[mem_index].phy_addr & ~PAGE_MASK)
		return -1;

	vma->vm_page_prot = pgprot_noncached(vma->vm_page_prot);

	if(remap_pfn_range(vma, vma->vm_start, mem_info[mem_index].phy_addr >> PAGE_SHIFT, 
						vma->vm_end - vma->vm_start, 
						vma->vm_page_prot)){
		printk("******************** mmap error ********************\n");
		return -1;
	}
	printk("mmap sucess!\n");	
	return 0;

}

static const struct file_operations np_pci_cdev_fops = {
	.owner = THIS_MODULE,
	.open = np_pci_cdev_open,
	.mmap = np_pci_cdev_mmap,
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
static uint64_t get_pages(uint64_t pages_order, uint8_t dma_flag){	//dma_flag 1:有GFP_DMA 0:无GFP_DMA
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

// 缓冲区的初始化
static int np_sw_init(void){

	uint64_t pages_addr, phy_addr, bus_addr, pages_size;
	uint8_t i;		// 缓冲区计数器
	uint8_t mem_info_cnt;	// 需要mmap的内存块计数器

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

	mem_info_cnt = np_pci_cdev->mem_info_cnt;
	np_pci_cdev->np_pci_cdev_mem_info[mem_info_cnt].phy_addr = phy_addr;
	np_pci_cdev->np_pci_cdev_mem_info[mem_info_cnt].size = (1 << ORDER_OF_BUF_ADDR_INFO) * PAGE_SIZE;
	np_pci_cdev->mem_info_cnt ++;

	// 申请1块缓冲区
	for(i = 0; i < ALLOC_HW_BUF_NUM; i++){
		if((pages_addr = get_pages(ORDER_OF_BUF, DMA_FLAG)) == 0){
			printk("hard dma buff gets error in func get_pages\n");
			goto err_sw_init;
		}
		if(i==0)
			g_pages_addr = pages_addr;

		phy_addr = virt_to_phys((void *)pages_addr);
		pages_size = (1 << ORDER_OF_BUF)* PAGE_SIZE;
		
		record_pages_info(pages_info_4_release, pages_addr, bus_addr, ORDER_OF_BUF, DMA_FLAG);

		// 缓冲区的地址对信息记录在全局变量buff_addr_info当中
		buf_addr_info->buf_cnt ++;
		if( i < 128)
			buf_addr_info->hw_buf_cnt ++;
		else
			buf_addr_info->sw_buf_cnt ++;
		
		buf_addr_info->buf_addr[i].bus_addr = bus_addr;
		buf_addr_info->buf_addr[i].phy_addr = phy_addr;
		printk("----------------------the phy addr is %#llx----------------------\n",buf_addr_info->buf_addr[i].phy_addr);
		printk("----------------------the kernel vir addr is %#llx----------------------\n", buf_addr_info->buf_addr[i].user_vir_addr);

		// 缓冲区需要被映射到用户空间，因此信息要记录到np_pci_cdev->np_pci_cdev_mem_info[200]当中
		mem_info_cnt = np_pci_cdev->mem_info_cnt;
		np_pci_cdev->np_pci_cdev_mem_info[mem_info_cnt].phy_addr = phy_addr;
		np_pci_cdev->np_pci_cdev_mem_info[mem_info_cnt].size = (1 << ORDER_OF_BUF) * PAGE_SIZE;
		np_pci_cdev->mem_info_cnt ++;

	}

	return 0;

err_sw_init:
	free_all_pages(pages_info_4_release);
	return -1;
}


static int npe_recv_poll_netdev(void *data)
{
	char *current_buf_addr = (char *)g_pages_addr;
	char* next_buf_addr;
	int buf_cnt = 0;

	struct timeval probe_time;

	int j;
	char* buf;
	buf = (char*)g_pages_addr; //1K * 2048
	printk(KERN_ERR "kernel wait....\n");
	while (1) {
		buf_cnt ++;
		msleep(10000);
		buf[0] = 1;
		do_gettimeofday(&probe_time);
		buf += 2048;
		printk(KERN_ERR "[sec: %10ld usec: %ld]: kernel write %d : %llx\n", probe_time.tv_sec, probe_time.tv_usec, buf_cnt, buf);
	}

	return 0;
}

 void npe_start_thread(void )
{	

	struct task_struct *tsk;
       		
	tsk = kthread_create(npe_recv_poll_netdev,NULL,"FAST");	
	// 
	// set_cpus_allowed_ptr(tsk, cpumask_of(2));			
	if(IS_ERR(tsk))
	{			
		tsk = NULL;
	}
	else
	{			
		wake_up_process(tsk);
	}	
}




/*------------------------------------------------- remove module --------------------------------------------------*/

static void free_all_pages(struct pages_info_4_release *p_release){
	int i;
	uint64_t page_size;
	uint64_t base_addr;
	uint64_t bus_addr;
	uint64_t page_reserve;

	for(i = 0; i < p_release->cnt; i++){
		page_size = (1 << p_release->p_info[i].order) * PAGE_SIZE;
		base_addr = p_release->p_info[i].base_addr;
		bus_addr = p_release->p_info[i].bus_addr;

		// 解除页保留
		for(page_reserve = base_addr; page_reserve < base_addr + page_size; page_reserve += PAGE_SIZE){
			ClearPageReserved(virt_to_page(page_reserve));
		}
		// 释放页
		free_pages(p_release->p_info[i].base_addr, p_release->p_info[i].order);
	}

	// 释放记录pages_info_4_release结构体的页
	page_size = (1 << ORDER_OF_PAGES_INFO_4_RELEASE) * PAGE_SIZE;
	for(page_reserve = (uint64_t)p_release; page_reserve < (uint64_t)p_release + page_size; page_reserve += PAGE_SIZE){
			ClearPageReserved(virt_to_page(page_reserve));
	}
	free_pages((uint64_t)p_release, ORDER_OF_PAGES_INFO_4_RELEASE);
}


static int __init udriver_init_module(void){

	struct cdev *my_cdev;
	uint8_t mem_info_cnt;				// 需要mmap的内存块的计数器

	printk(KERN_ERR "start init...\n");

	// 为自定义设备np_pci_cdev申请内存
	np_pci_cdev = kzalloc(sizeof(struct np_pci_cdev), GFP_KERNEL);
	// 初始化np_pci_cdev->mem_info_cnt
	np_pci_cdev->mem_info_cnt = 0;

	// 申请并注册字符设备，创建设备文件
	// 申请设备号
	alloc_chrdev_region(&cdev_no, 0, 1, "np_pci_cdev");
	// 在/sys/class/目录下创建np_pci_cdev类
	np_pci_cdev_class = class_create(THIS_MODULE, "np_pci_cdev");
	// 在/dev/目录下创建np_pci_cdev0设备
	dev = device_create(np_pci_cdev_class, NULL, cdev_no, NULL, "np_pci_cdev0");
	// 分配设备结构，初始化并添加cdev结构体
	my_cdev = cdev_alloc();
	my_cdev->ops = &np_pci_cdev_fops;

	// 设备文件被打开后，使设备文件的私有数据指向这个结构体
	// 在mmap中，根据设备文件的私有数据，得到这个结构体
	np_pci_cdev->cdev = *my_cdev;
	cdev_init(&np_pci_cdev->cdev, &np_pci_cdev_fops);
	np_pci_cdev->cdev.owner = THIS_MODULE;
	cdev_add(&np_pci_cdev->cdev, cdev_no, 1);

	// 软件缓冲区初始化
	np_sw_init();

	// 创建写缓冲区的线程
	npe_start_thread();

	printk(KERN_ERR "init done!\n");

	return 0;
}

static void __exit udriver_cleanup_module(void){

	free_all_pages(pages_info_4_release);				// 释放所有申请的页
	unregister_chrdev_region(cdev_no, 1);					// 对应alloc_chrdev_region()
	cdev_del(&np_pci_cdev->cdev);						// 对应cdev_add()
	kfree(np_pci_cdev);							// 对应kzalloc()
	device_destroy(np_pci_cdev_class, cdev_no);				// 对应device_create()
	class_destroy(np_pci_cdev_class);					// 对应class_create()

	return;

}

module_init(udriver_init_module);
module_exit(udriver_cleanup_module);
MODULE_LICENSE("GPL");
