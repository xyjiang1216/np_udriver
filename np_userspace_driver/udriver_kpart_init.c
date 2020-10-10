#include "udriver_kpart_init.h"
#include "udriver_kpart_pkt.h"
/******************* 全局变量 *******************/

// pci_device_id & pci_device
static struct pci_device_id np_pci_ids[] = {
	{PCI_DEVICE(0x0755,0x0755)},
	{0,},
};
// 热拔插的时候使用
MODULE_DEVICE_TABLE(pci, np_pci_ids);	

// pci_driver
static struct pci_driver udriver_kpart = {
	.name = DRIVER_NAME,
	.id_table =  np_pci_ids,
	.probe = np_kernel_probe,
	.remove = np_kernel_release,
};
unsigned long g_pages_addr = 0;
// 自定义的设备
static struct np_pci_cdev *np_pci_cdev;
// /dev目录下创建字符设备文件
static struct class *np_pci_cdev_class;
static struct device *dev;
// 字符设备的主设备号
dev_t cdev_no;

// 记录申请的所有页，释放时使用
static struct pages_info_4_release *pages_info_4_release;
// 记录地址三元组信息的结构体，会被映射到用户空间
static struct buf_addr_info *buf_addr_info;

// BAR0空间
static char* bar0_addr;

/******************* 函数定义 *******************/

static int np_pci_cdev_open(struct inode *inode, 
							struct file *filp)
{
	// 使设备文件的私有数据指向将全局变量np_pci_cdev
	struct np_pci_cdev *my_np_pci_cdev;	
	my_np_pci_cdev = container_of(inode->i_cdev, 
								struct np_pci_cdev, cdev);
	filp->private_data = my_np_pci_cdev;
	return 0;
}

static int np_pci_cdev_mmap(struct file *filp, 
							struct vm_area_struct *vma){
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
		printk("******************** please check the offset variable \
				********************\n");
		return -1;
	}

	// 检查传入的物理地址是否与页大小对齐
	if (mem_info[mem_index].phy_addr & ~PAGE_MASK)
		return -1;
	
	// BAR0空间采用noncacheable的方式映射
	if(mem_index == 0){
		vma->vm_page_prot = pgprot_noncached(vma->vm_page_prot);
		if(remap_pfn_range(vma, vma->vm_start, 
						mem_info[mem_index].phy_addr >> PAGE_SHIFT, 
						vma->vm_end - vma->vm_start, 
						vma->vm_page_prot)){// vma->vm_page_prot
			printk("******************** mmap error \
					********************\n");
			return -1;
		}
	// 数据内存空间采用cacheable的方式映射
	}else{
	//	vma->vm_page_prot = pgprot_noncached(vma->vm_page_prot);
		if(remap_pfn_range(vma, vma->vm_start, 
						mem_info[mem_index].phy_addr >> PAGE_SHIFT, 
						vma->vm_end - vma->vm_start, 
						PAGE_SHARED)){// vma->vm_page_prot
			printk("******************** mmap error \
					********************\n");
			return -1;
		}
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
static void record_pages_info(struct pages_info_4_release *p_release, 
							uint64_t base_address, 
							uint64_t bus_address, 
							uint64_t pages_order, 
							uint64_t dma_flag)
{
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
		pages_base = __get_free_pages(GFP_KERNEL | \
									__GFP_ZERO | GFP_DMA, 
									pages_order);
	}else if(dma_flag == 0){
		pages_base = __get_free_pages(GFP_KERNEL | \
									__GFP_ZERO, 
									pages_order);
	}

	if(pages_base == 0){
		return -1;
	}
	
	SetPageReserved(virt_to_page(pages_base));
	pages_size = (1 << pages_order) * PAGE_SIZE;
	memset((char*)pages_base, 0, pages_size);

	return pages_base;
}

// 软件缓冲区的初始化
static int np_mem_init(struct pci_dev *pdev){

	uint64_t pages_addr, phy_addr, bus_addr, pages_size;
	// 缓冲区计数器
	uint8_t i;				
	// 需要mmap的内存块计数器
	uint8_t mem_info_cnt;	

	// 申请16k存放pages_info_4_release结构体
	pages_addr = get_pages(ORDER_OF_PAGES_INFO_4_RELEASE, 0);
	if(pages_addr == -1){
		goto err_mem_init;
	}
	pages_info_4_release = (struct pages_info_4_release *)pages_addr;
	//全局变量pages_info_4_release初始化
	pages_info_4_release->cnt = 0;

	// 申请2M内存存放地址信息的结构体
	pages_addr = get_pages(ORDER_OF_BUF_ADDR_INFO, NOT_DMA_FLAG);
	if(pages_addr == -1){
		goto err_mem_init;
	};
	record_pages_info(pages_info_4_release, 
					pages_addr, 0, 
					ORDER_OF_BUF_ADDR_INFO, NOT_DMA_FLAG);
	buf_addr_info = (struct buf_addr_info *)pages_addr;
	phy_addr = virt_to_phys((void *)pages_addr);
	// 全局变量buf_addr_info初始化
	buf_addr_info->buf_cnt = 0;
	buf_addr_info->sw_buf_cnt = 0;
	buf_addr_info->hw_buf_cnt = 0;

	mem_info_cnt = np_pci_cdev->mem_info_cnt;
	np_pci_cdev->np_pci_cdev_mem_info[mem_info_cnt].phy_addr = \
															phy_addr;
	np_pci_cdev->np_pci_cdev_mem_info[mem_info_cnt].size = \
							(1 << ORDER_OF_BUF_ADDR_INFO) * PAGE_SIZE;
	np_pci_cdev->mem_info_cnt ++;

	// 申请128块硬件缓冲区 + 2块软件缓冲区
	// 规定前128块为硬件缓冲区，后2块为软件缓冲区
	for(i = 0; i < ALLOC_HW_BUF_NUM + ALLOC_SW_BUF_NUM; i++){
		pages_size = (1 << ORDER_OF_BUF)* PAGE_SIZE;
		// 一致性DMA映射
/*		pages_addr = (uint64_t)dma_alloc_coherent(&(pdev->dev), 
												pages_size, 
												&bus_addr, 
												GFP_KERNEL);
*/
		// 流式DMA映射
/*		if((pages_addr = get_pages(ORDER_OF_BUF, DMA_FLAG)) == 0){
			printk("hard dma buff gets error in func get_pages\n");
			goto err_mem_init;
		}
		phy_addr = virt_to_phys((void *)pages_addr);
		if(i == 0) g_pages_addr = pages_addr;	
		bus_addr = dma_map_single(&(pdev->dev), (void *)pages_addr, pages_size, DMA_BIDIRECTIONAL);
*/		
		if((pages_addr = get_pages(ORDER_OF_BUF, DMA_FLAG)) == 0){
			printk("hard dma buff gets error in func get_pages\n");
			goto err_mem_init;
		}
		if(i==0)g_pages_addr = pages_addr;
		phy_addr = virt_to_phys((void *)pages_addr);
		pages_size = (1 << ORDER_OF_BUF)* PAGE_SIZE;
		bus_addr = dma_map_single(&(pdev->dev), (void *)pages_addr, pages_size, DMA_BIDIRECTIONAL);
		
//		record_pages_info(pages_info_4_release, pages_addr, bus_addr, ORDER_OF_BUF, DMA_FLAG);
		
		
		record_pages_info(pages_info_4_release, 
						pages_addr, 
						bus_addr, 
						ORDER_OF_BUF, 
						DMA_FLAG);

		// 缓冲区的地址对信息记录在全局变量buff_addr_info当中
		buf_addr_info->buf_cnt ++;
		if( i < 128)
			buf_addr_info->hw_buf_cnt ++;
		else
			buf_addr_info->sw_buf_cnt ++;
		
		buf_addr_info->buf_addr[i].bus_addr = bus_addr;
		buf_addr_info->buf_addr[i].phy_addr = phy_addr;
		printk("----------------------%d the phy addr is %#llx\
				----------------------\n",i, 
				buf_addr_info->buf_addr[i].phy_addr);
		// 缓冲区需要被映射到用户空间
		// 因此信息要记录到np_pci_cdev->np_pci_cdev_mem_info[200]当中
		mem_info_cnt = np_pci_cdev->mem_info_cnt;
		np_pci_cdev->np_pci_cdev_mem_info[mem_info_cnt].phy_addr = \
															phy_addr;
		np_pci_cdev->np_pci_cdev_mem_info[mem_info_cnt].size = \
									(1 << ORDER_OF_BUF) * PAGE_SIZE;
		np_pci_cdev->mem_info_cnt ++;
	}
	return 0;
err_mem_init:
	free_all_pages(pdev, pages_info_4_release);
	return -1;
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
	// 向寄存器写buf_addr_info结构体的
	// 成员变量uint64_t vir_addr_first的信息
	writeq(buf_addr_info->vir_addr_first, bar0_addr + 0x018);

	// 下发DMA空间描述符信息：21位物理地址+21位虚拟地址
	for (i = 0; i < buf_addr_info->hw_buf_cnt; i++) {
		// 内核空间虚拟地址左移32位
		hw_desc = buf_addr_info->buf_addr[i].user_vir_addr << 32;
		// 上述结果右移32位
		hw_desc = hw_desc >> 32;
		// 上述结果左移10位 与 总线地址右移11位后 做或操作
		hw_desc = (hw_desc << 10 | \
				buf_addr_info->buf_addr[i].bus_addr >> 11);
		// 向0x10的位置写上述结果
	//	bar0_registers->hard_desc_dma = hw_desc;
		writeq(hw_desc, bar0_addr + 0x010);
	}
	printk("%s\n", "hardware reset success!");
	return;
}

void np_show_pkt(unsigned char *pkt)
{
        int i=0,len=0;
        printk("---------------------*******---------------------\n");
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
        printk("--------------------*******---------------------\n");
}


static int np_kernel_poll(void)
{
	struct cp_packet *pkt;
	char *current_buf;
	char *next_buf;
	
	current_buf = (char *)g_pages_addr;
	pkt = (struct cp_packet *)current_buf;

	while(1)
	{
		if(pkt->cp.busy)
		{

			if (pkt->um.inport == 0) {
				pkt->um.inport = pkt->um.outport;
				pkt->um.outport = 1;
			}
			else if (pkt->um.inport == 1) {
				pkt->um.inport = pkt->um.outport;
				pkt->um.outport = 0;
			}

			// 设置um_matadata的目标模块字段
			pkt->um.dmid = 0x45;
			// 设置um_metadata的pkt_dse字段
			pkt->um.pktdst = 0;

			// 设置cp_head的描述符控制字段
			pkt->cp.ctl = 0;
			// 设置um_matadata的报文类型字段			
			pkt->um.pkt_type = 0;

			// 将busy位置为0
			pkt->cp.busy = 0;
			// 发送报文
			// 把该报文的第一个八字节写到BAR0空间的第一个寄存器
			writeq(pkt->cs.ctl, bar0_addr);

			current_buf += 2048;
			pkt = (struct cp_packet *)current_buf;

		}else{

			schedule();
		}
	}
	return 0;
}

 void np_thread_start_up(void )
{	

	struct task_struct *tsk;
       		
	tsk = kthread_create(np_kernel_poll,NULL,"FAST");	
	// 将线程绑定在2号核上
	set_cpus_allowed_ptr(tsk, cpumask_of(2));			
	if(IS_ERR(tsk))
	{			
		tsk = NULL;
	}
	else
	{			
		wake_up_process(tsk);
	}	
}



static int np_kernel_probe(struct pci_dev *pdev, 
						const struct pci_device_id *pid_tbl){
	
	// io内存资源的存储器域物理地址、长度及内核空间地址
	uint64_t pci_io_base, pci_io_len;	
	uint64_t sys_ctl;
	
	int err;
	struct cdev *my_cdev;
	
	// 需要mmap的内存块的计数器
	uint8_t mem_info_cnt;				

	if((err = pci_enable_device(pdev))){
		printk("-----------------------pci device \
				enable fail!-----------------------\n");
		return err;
	}
	if((err = pci_request_regions(pdev, DRIVER_NAME))){
		printk("-----------------------pci device request \
				region fail!-----------------------\n");
		return err;
	}
	pci_set_master(pdev);
	printk("-----------------------pci device \
			enable success!-----------------------\n");
	
	
	// 读取BARO空间地址（存储器域物理地址）及长度
	pci_io_base = pci_resource_start(pdev, 0);
	pci_io_len = pci_resource_len(pdev, 0);
	// 不要将BAR0空间映射到内核地址空间
	bar0_addr = ioremap(pci_io_base, pci_io_len);

	// 为自定义设备np_pci_cdev申请内存
	np_pci_cdev = kzalloc(sizeof(struct np_pci_cdev), GFP_KERNEL);
	// 初始化np_pci_cdev->mem_info_cnt
	np_pci_cdev->mem_info_cnt = 0;

	// 将bar0空间的信息记录在np_pci_cdev->np_pci_cdev_mem_info[160]中
	mem_info_cnt = np_pci_cdev->mem_info_cnt;
	np_pci_cdev->np_pci_cdev_mem_info[mem_info_cnt].phy_addr = pci_io_base;
	np_pci_cdev->np_pci_cdev_mem_info[mem_info_cnt].size = pci_io_len;
	np_pci_cdev->mem_info_cnt ++;

	// 申请并注册字符设备，创建设备文件
	// 申请设备号
	alloc_chrdev_region(&cdev_no, 0, 1, "np_pci_cdev");
	// 在/sys/class/目录下创建np_pci_cdev类
	np_pci_cdev_class = class_create(THIS_MODULE, "np_pci_cdev");
	// 在/dev/目录下创建np_pci_cdev0设备
	dev = device_create(np_pci_cdev_class, 
						NULL, cdev_no, NULL, 
						"np_pci_cdev0");
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
	np_mem_init(pdev);

	// 创建写缓冲区的线程
//	np_thread_start_up();
	


	/***************** 调试用 ******************/
/*
	// 下发virt_first
	buf_addr_info->vir_addr_first = ((buf_addr_info->buf_addr[0].user_vir_addr ^ buf_addr_info->buf_addr[0].phy_addr) \
		| buf_addr_info->buf_addr[0].phy_addr) >> 32;

	// 硬件复位 下发地址
	np_hw_reset();

	// 创建轮询线程
	np_thread_start_up();

	// 下发控制信息   1：os_endian     32：NPE_HARD_TAG_NUM  1：g_npe_adapter->hw.use_dma_cnt
	sys_ctl = (0 << 15) | (1 << 14) | (32 << 6) | 1;
//	bar0_registers->hard_sys_set = sys_ctl;
	writeq(sys_ctl, bar0_addr + 0x008);
	msleep(1);

	writeq(2, bar0_addr + 0x080);
	msleep(1);
*/


//	np_show_pkt((char *)buf_addr_info);			// 调试用——查看buf_addr_info里面的信息

	return 0;
}


/*--------------------------------------------------驱动卸载---------------------------------------------------*/

static void free_all_pages(struct pci_dev *pdev, struct pages_info_4_release *p_release){
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
			dma_unmap_single(&(pdev->dev), bus_addr, page_size, DMA_BIDIRECTIONAL);
		//	dma_free_coherent(&(pdev->dev),page_size, (void *)base_addr, bus_addr);
		}
		//else{
		// 解除页保留
			for(page_reserve = base_addr; page_reserve < base_addr + page_size; page_reserve += PAGE_SIZE){
				ClearPageReserved(virt_to_page(page_reserve));
			}
		// 释放页
			free_pages(p_release->p_info[i].base_addr, p_release->p_info[i].order);
		//}
	}

	// 释放记录pages_info_4_release结构体的页
	page_size = (1 << ORDER_OF_PAGES_INFO_4_RELEASE) * PAGE_SIZE;
	for(page_reserve = (uint64_t)p_release; page_reserve < (uint64_t)p_release + page_size; page_reserve += PAGE_SIZE){
			ClearPageReserved(virt_to_page(page_reserve));
	}
	free_pages((uint64_t)p_release, ORDER_OF_PAGES_INFO_4_RELEASE);
}

static void np_kernel_release(struct pci_dev *pdev){
	free_all_pages(pdev, pages_info_4_release);				// 释放所有申请的页
	pci_release_regions(pdev);								// 对应pci_requesr_regions()
	pci_disable_device(pdev);								// 对应pci_enable_device()
	unregister_chrdev_region(cdev_no, 1);					// 对应alloc_chrdev_region()
	cdev_del(&np_pci_cdev->cdev);							// 对应cdev_add()
	kfree(np_pci_cdev);										// 对应kzalloc()
	device_destroy(np_pci_cdev_class, cdev_no);				// 对应device_create()
	class_destroy(np_pci_cdev_class);						// 对应class_create()
}

static int __init udriver_init_module(void){
//	printk(KERN_ERR "start init...\n");
	pci_register_driver(&udriver_kpart);
//	printk(KERN_ERR "init done!\n");
	return 0;
}

static void __exit udriver_cleanup_module(void){
	pci_unregister_driver(&udriver_kpart);
}

module_init(udriver_init_module);
module_exit(udriver_cleanup_module);
MODULE_LICENSE("GPL");
