/*
 * Sample disk driver, from the beginning.

 */

#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/init.h>

#include <linux/sched.h>
#include <linux/kernel.h>	/* printk() */
#include <linux/slab.h>		/* kmalloc() */
#include <linux/fs.h>		/* everything... */
#include <linux/errno.h>	/* error codes */
#include <linux/timer.h>
#include <linux/types.h>	/* size_t */
#include <linux/fcntl.h>	/* O_ACCMODE */
#include <linux/hdreg.h>	/* HDIO_GETGEO */
#include <linux/kdev_t.h>
#include <linux/vmalloc.h>
#include <linux/genhd.h>
#include <linux/blkdev.h>
#include <linux/buffer_head.h>	/* invalidate_bdev */
#include <linux/bio.h>

// The crypto api source
//***********************
#include <linux/crypto.h>
//***********************


MODULE_LICENSE("Dual BSD/GPL");
static int sbull_major = 0;
module_param(sbull_major, int, 0);
static int hardsect_size = 512;
module_param(hardsect_size, int, 0);
static int nsectors = 16336;	/* How big the drive is */
module_param(nsectors, int, 0);
static int ndevices = 1;
module_param(ndevices, int, 0);

/*
 * The different "request modes" we can use.
 */
enum {
	RM_SIMPLE  = 0,	/* The extra-simple request function */
	RM_FULL    = 1,	/* The full-blown version */
	RM_NOQUEUE = 2,	/* Use make_request */
};
static int request_mode = RM_SIMPLE;
module_param(request_mode, int, 0);

/*
 * Minor number and partition management.
 */
#define SBULL_MINORS	16
#define MINOR_SHIFT	4
#define DEVNUM(kdevnum)	(MINOR(kdev_t_to_nr(kdevnum)) >> MINOR_SHIFT

/*
 * We can tweak our hardware sector size, but the kernel talks to us
 * in terms of small sectors, always.
 */
#define KERNEL_SECTOR_SHIFT	9
#define KERNEL_SECTOR_SIZE	(1<<KERNEL_SECTOR_SHIFT)

/*
 * After this much idle time, the driver will simulate a media change.
 */
#define INVALIDATE_DELAY	30*HZ


//Crypto def - Arman 5/18/17
//*********************************

//static char *crypto_key = "hello";
//
static char *key = "hello";
module_param(key, charp, S_IRUGO);

#define KEY_SIZE 32
static char crypto_key[KEY_SIZE];
static int key_size = 0;


struct crypto_cipher *cipher;

//*********************************


/*
 * The internal representation of our device.
 */
struct sbull_dev {
        int size;                       /* Device size in sectors */
        u8 *data;                       /* The data array */
        short users;                    /* How many users */
        short media_change;             /* Flag a media change? */
        spinlock_t lock;                /* For mutual exclusion */
        struct request_queue *queue;    /* The device request queue */
        struct gendisk *gd;             /* The gendisk structure */
        struct timer_list timer;        /* For simulated media changes */
};

static struct sbull_dev *Devices = NULL;


//Show
//*****************************************************************************
ssize_t key_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	printk(KERN_DEBUG "[HW3] crypt: Copying key\n");
	return scnprintf(buf, PAGE_SIZE, "%s\n", crypto_key);
}
//*****************************************************************************


//store
//*****************************************************************************
ssize_t key_store(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
	if(count != 16 && count !=24 && count != 32)
	{
		printk(KERN_WARNING "[HW3] Crpyt: invalid key size %d\n", count);
		return -EINVAL;
	}

	printk(KERN_DEBUG " [HW3] crpyt: storing key\n");
	snprintf(crypto_key, sizeof(crypto_key), "%.*s", (int)min(count, sizeof(crypto_key) - 1), buf);
	key_size = count;
	return count;
}
//*****************************************************************************


// Store key
//*****************************************************************************
DEVICE_ATTR(key, 0600, key_show, key_store);
//*****************************************************************************


// Handle an I/O request.
// Main location where driver handles encryption - Arman 5/18/17
//*****************************************************************************
static void sbull_transfer(struct sbull_dev *dev, unsigned long sector,
		unsigned long nsect, char *buffer, int write)
{
	int i;
	unsigned long offset = sector*KERNEL_SECTOR_SIZE;
	unsigned long nbytes = nsect*KERNEL_SECTOR_SIZE;
	char* safe_buffer = kmalloc(nbytes + nbytes%crypto_cipher_blocksize(cipher), GFP_KERNEL);

	memset(safe_buffer, 0, nbytes + nbytes%crypto_cipher_blocksize(cipher));
	memcpy(safe_buffer, buffer, nbytes);
	//Forcing the key to be set
	// // // // // // // // // // // // // // // // // //
	if(key_size == 0)
	{
		crypto_cipher_clear_flags(cipher, ~0);
		crypto_cipher_setkey(cipher, key, strlen(key));
		key_size = strlen(key);
		printk("[HW3] Key size %d\n", key_size);

	}
	else
	{
		crypto_cipher_clear_flags(cipher, ~0);
		crypto_cipher_setkey(cipher, crypto_key, key_size);
	}
	// // // // // // // // // // // // // // // // // //

	if ((offset + nbytes) > dev->size)
	{
		printk (KERN_NOTICE "Beyond-end write (%ld %ld)\n", offset, nbytes);
		return;
	}



	//Encrypting each block one by one from the Write/Read IFELSE statement
	//Takes the cipher block, and encypts each byte then writes it in the buffer
	// // // // // // // // // // // // // // // // // // // // // // // //
	if(write)
	{
		printk("[HW3] Writing\n");
		if(key_size != 0)
		{
			//Encryption each byte for each byte
			printk("[HW3] Encrypting\n");
			printk("[HW3] Storying Encryption in %p\n", dev->data);
			for(i = 0; i < nbytes; i += crypto_cipher_blocksize(cipher))
			{
				crypto_cipher_encrypt_one(cipher, dev->data+offset+i, safe_buffer+i);
			}
		}
		else// Pass process without encryption for 0 values
		{
			memcpy(dev->data + offset, buffer, nbytes);
		}
	}

	else
	{
		printk("[HW3] Reading\n");
		if(key_size != 0)
		{
			//Dencryption byte by byte
			printk("[HW3] Decrypting\n");
			for(i = 0; i < nbytes; i += crypto_cipher_blocksize(cipher))
			{
				crypto_cipher_decrypt_one(cipher, safe_buffer+i, dev->data+offset+i);
			}
		}
		else // Pass process without encryption for 0 values
		{
			memcpy(buffer, dev->data + offset, nbytes); //f
		}
	}
	kfree(safe_buffer);
	// // // // // // // // // // // // // // // // // // // // // // // //

}
//*****************************************************************************







//
//*****************************************************************************
static int bytes_to_sectors_checked(unsigned long bytes)
{
	if( bytes % KERNEL_SECTOR_SIZE )
	{
		printk("***************WhatTheFuck***********************\n");
	}
	return bytes / KERNEL_SECTOR_SIZE;
}
//*****************************************************************************



/*
 * The simple form of the request function.
 */
static void sbull_request(struct request_queue *q)
{
	struct request *req;
	int ret;

	//Get next incomplete request in the queue
	req = blk_fetch_request(q);
	while (req) {
		struct sbull_dev *dev = req->rq_disk->private_data;

		//Make sure its a filesystem request
		if (req->cmd_type != REQ_TYPE_FS) {
			printk (KERN_NOTICE "Skip non-fs request\n");
			ret = -EIO;
			goto done;
		}
		printk (KERN_NOTICE "Req dev %u dir %d sec %ld, nr %d\n",
			(unsigned)(dev - Devices), rq_data_dir(req),
			blk_rq_pos(req), blk_rq_cur_sectors(req));
		//Pass the request off the transfer
		sbull_transfer(dev, blk_rq_pos(req), blk_rq_cur_sectors(req),
				req->buffer, rq_data_dir(req));
		ret = 0;
	done:
		//Error handling and going to the next request
		if(!__blk_end_request_cur(req, ret)){
			req = blk_fetch_request(q);
		}
	}
}



/*
 * Transfer a single BIO.
 */
static int sbull_xfer_bio(struct sbull_dev *dev, struct bio *bio)
{
	struct bio_vec bvec;
	struct bvec_iter iter;
	sector_t sector = bio->bi_iter.bi_sector;

	//Iterate throught each section
	bio_for_each_segment(bvec, bio, iter) {
		char *buffer = __bio_kmap_atomic(bio, iter);
		sbull_transfer(dev, sector,bytes_to_sectors_checked(bio_cur_bytes(bio)),
				buffer, bio_data_dir(bio) == WRITE);
		sector += (bytes_to_sectors_checked(bio_cur_bytes(bio)));
		__bio_kunmap_atomic(bio);
	}
	return 0; /* Always "succeed" */
}


/*
 * Transfer a full request.
 */
static int sbull_xfer_request(struct sbull_dev *dev, struct request *req)
{
	struct bio *bio;
	int nsect = 0;

	//steps through each request and sends it to xfer_bio
	__rq_for_each_bio(bio, req) {
		sbull_xfer_bio(dev, bio);
		nsect += bio->bi_iter.bi_size/KERNEL_SECTOR_SIZE;
	}
	return nsect;
}



/*
 * Smarter request function that "handles clustering".
 */
static void sbull_full_request(struct request_queue *q)
{
	struct request *req;
	struct sbull_dev *dev = q->queuedata;
	int ret;

	//Takes request and sends i to xfer_reqest
	while ((req = blk_fetch_request(q)) != NULL) {
		if (req->cmd_type != REQ_TYPE_FS) {
			printk (KERN_NOTICE "Skip non-fs request\n");
			ret = -EIO;
			goto done;
		}
		sbull_xfer_request(dev, req);
		ret = 0;
	done:
		__blk_end_request_all(req, ret);
	}
}



/*
 * The direct make request version.
 */
static void sbull_make_request(struct request_queue *q, struct bio *bio)
{
	struct sbull_dev *dev = q->queuedata;
	int status;

	status = sbull_xfer_bio(dev, bio);		//Goes through the bio request without a request queue
	bio_endio(bio, status);
}



// Open device / Sbull_open / incr
//*****************************************************************************
static int sbull_open(struct block_device *bdev, fmode_t mode)
{
	struct sbull_dev *dev = bdev->bd_disk->private_data;


	del_timer_sync(&dev->timer);			//Remove the media timer

	spin_lock(&dev->lock);					//Get the lock on the drive
	if (! dev->users)
		check_disk_change(bdev);			//Check if media has changed
	dev->users++;							//Increase users
	spin_unlock(&dev->lock);   				//Release lock
	return 0;
}
//*****************************************************************************

// Release Device / opposite of Sbull_open / decr
//*****************************************************************************
static void sbull_release(struct gendisk *disk, fmode_t mode)
{
	struct sbull_dev *dev = disk->private_data;

	spin_lock(&dev->lock);
	dev->users--;

	//Timer started to invalidate if not in use.
	if (!dev->users) {
		dev->timer.expires = jiffies + INVALIDATE_DELAY;
		add_timer(&dev->timer);
	}
	spin_unlock(&dev->lock);

}
//*****************************************************************************



/*
 * Look for a (simulated) media change.
 */
int sbull_media_changed(struct gendisk *gd)
{
	struct sbull_dev *dev = gd->private_data;
	
	return dev->media_change;
}

/*
 * Revalidate.  WE DO NOT TAKE THE LOCK HERE, for fear of deadlocking
 * with open.  That needs to be reevaluated.
 */
int sbull_revalidate(struct gendisk *gd)
{
	struct sbull_dev *dev = gd->private_data;
	
	if (dev->media_change) {
		dev->media_change = 0;
		memset (dev->data, 0, dev->size);
	}
	return 0;
}

/*
 * The "invalidate" function runs out of the device timer; it sets
 * a flag to simulate the removal of the media.
 */
void sbull_invalidate(unsigned long ldev)
{
	struct sbull_dev *dev = (struct sbull_dev *) ldev;

	spin_lock(&dev->lock);
	if (dev->users || !dev->data) 
		printk (KERN_WARNING "sbull: timer sanity check failed\n");
	else
		dev->media_change = 1;
	spin_unlock(&dev->lock);
}

// ioctl() implementation / Handles request for drives geometry / virtual but needed for partitioning
//*****************************************************************************
int sbull_ioctl (struct block_device *bdev,
		 fmode_t mode,
                 unsigned int cmd, unsigned long arg)
{
	long size;
	struct hd_geometry geo;
	struct sbull_dev *dev = bdev->bd_disk->private_data;
	//printk("[HW3] HERE IS THE ADDRESS %p", sbull_dev->dev);
	
	switch(cmd) {
	    case HDIO_GETGEO:
        	/*
		 * Get geometry: since we are a virtual device, we have to make
		 * up something plausible.  So we claim 16 sectors, four heads,
		 * and calculate the corresponding number of cylinders.  We set the
		 * start of data at sector four.
		 */
		size = dev->size*(hardsect_size/KERNEL_SECTOR_SIZE);
		geo.cylinders = (size & ~0x3f) >> 6;
		geo.heads = 4;
		geo.sectors = 16;
		geo.start = 4;
		if (copy_to_user((void __user *) arg, &geo, sizeof(geo)))
			return -EFAULT;
		return 0;
	}

	return -ENOTTY; /* unknown command */
}
//*****************************************************************************


/*
 * The device operations structure.
 */
static struct block_device_operations sbull_ops = {
	.owner           = THIS_MODULE,
	.open 	         = sbull_open,
	.release 	 = sbull_release,
	.media_changed   = sbull_media_changed,
	.revalidate_disk = sbull_revalidate,
	.ioctl	         = sbull_ioctl
};



/*
 * Set up our internal device.
 */
static void setup_device(struct sbull_dev *dev, int which)
{



	memset (dev, 0, sizeof (struct sbull_dev));				//Get some memory.
	dev->size = nsectors*hardsect_size;// + nsectors*hardsect_size%crypto_cipher_blocksize(cipher);
	dev->data = vmalloc(dev->size);
	if (dev->data == NULL) {
		printk (KERN_NOTICE "vmalloc failure.\n");
		return;
	}
	spin_lock_init(&dev->lock);
	

	init_timer(&dev->timer);								//The timer which "invalidates" the device.
	dev->timer.data = (unsigned long) dev;
	dev->timer.function = sbull_invalidate;
	

	 //The I/O queue, depending on whether we are using our own
	 //make_request function or not.

	switch (request_mode) {
	    case RM_NOQUEUE:
		dev->queue = blk_alloc_queue(GFP_KERNEL);
		if (dev->queue == NULL)
			goto out_vfree;
		blk_queue_make_request(dev->queue, sbull_make_request);
		break;

	    case RM_FULL:
		dev->queue = blk_init_queue(sbull_full_request, &dev->lock);
		if (dev->queue == NULL)
			goto out_vfree;
		break;

	    default:
		printk(KERN_NOTICE "Bad request mode %d, using simple\n", request_mode);
	
	    case RM_SIMPLE:
		dev->queue = blk_init_queue(sbull_request, &dev->lock);
		if (dev->queue == NULL)
			goto out_vfree;
		break;
	}
	blk_queue_logical_block_size(dev->queue, hardsect_size);
	dev->queue->queuedata = dev;
	/*
	 * And the gendisk structure.
	 */
	dev->gd = alloc_disk(SBULL_MINORS);
	if (! dev->gd) {
		printk (KERN_NOTICE "alloc_disk failure\n");
		goto out_vfree;
	}
	dev->gd->major = sbull_major;
	dev->gd->first_minor = which*SBULL_MINORS;
	dev->gd->fops = &sbull_ops;
	dev->gd->queue = dev->queue;
	dev->gd->private_data = dev;
	snprintf (dev->gd->disk_name, 32, "sbull%c", which + 'a');
	set_capacity(dev->gd, nsectors*(hardsect_size/KERNEL_SECTOR_SIZE));
	add_disk(dev->gd);
	return;

  out_vfree:
	if (dev->data)
		vfree(dev->data);
}


//sbull init
//*****************************************************************************
static int __init sbull_init(void)
{
	int i;
	// // // // // // // // // // // // // // // //
	cipher = crypto_alloc_cipher("aes", 0, 16);
	key_size = strlen(crypto_key);
	// // // // // // // // // // // // // // // //

	sbull_major = register_blkdev(sbull_major, "sbull");
	if (sbull_major <= 0) {
		printk(KERN_WARNING "sbull: unable to get major number\n");
		return -EBUSY;
	}

	// Allocate the device array, and initialize each one.
	Devices = kmalloc(ndevices*sizeof (struct sbull_dev), GFP_KERNEL);
	if (Devices == NULL)
		goto out_unregister;
	for (i = 0; i < ndevices; i++) 
		setup_device(Devices + i, i);
    
	return 0;

  out_unregister:
	unregister_blkdev(sbull_major, "sbd");
	return -ENOMEM;
}
//*****************************************************************************


// EXIT
static void sbull_exit(void)
{
	int i;

	for (i = 0; i < ndevices; i++) {
		struct sbull_dev *dev = Devices + i;

		del_timer_sync(&dev->timer);
		if (dev->gd) {
			del_gendisk(dev->gd);
			put_disk(dev->gd);
		}
		if (dev->queue) {
			if (request_mode == RM_NOQUEUE)
				blk_put_queue(dev->queue);
			else
				blk_cleanup_queue(dev->queue);
		}
		if (dev->data)
			vfree(dev->data);
	}
	unregister_blkdev(sbull_major, "sbull");
	kfree(Devices);
}
	
module_init(sbull_init);
module_exit(sbull_exit);
