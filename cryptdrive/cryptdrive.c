/* This file contains the device dependent part of the drivers for the
 * following special files:
 *     /dev/upper		- Uppercasing RAM disk 
 *
 */

#include "../drivers.h"
#include "../libdriver/driver.h"
#include <sys/ioc_memory.h>
#include "../../kernel/const.h"
#include "../../kernel/config.h"
#include "../../kernel/type.h"
#include "assert.h"


FORWARD _PROTOTYPE( char *u_name, (void) 				);
FORWARD _PROTOTYPE( struct device *u_prepare, (int device) 		);
FORWARD _PROTOTYPE( int u_transfer, (int proc_nr, int opcode, off_t position,
					iovec_t *iov, unsigned nr_req) 	);
FORWARD _PROTOTYPE( int u_do_open, (struct driver *dp, message *m_ptr) 	);
FORWARD _PROTOTYPE( void u_init, (void) );
FORWARD _PROTOTYPE( int u_ioctl, (struct driver *dp, message *m_ptr) 	);
FORWARD _PROTOTYPE( void u_geometry, (struct partition *entry) 		);

/* Entry points to this driver. */
PRIVATE struct driver u_dtab = {
  u_name,	/* current device's name */
  u_do_open,	/* open or mount */
  do_nop,	/* nothing on a close */
  do_diocntl,	/* specify ram disk geometry */
  u_prepare,	/* prepare for I/O on a given minor device */
  u_transfer,	/* do the I/O */
  nop_cleanup,	/* no need to clean up */
  u_geometry,	/* memory device "geometry" */
  nop_signal,
  nop_alarm,
  nop_cancel,  
  nop_select,
  NULL,
  NULL
};

/*#define SECTOR_SIZE 512*/
#define UPPERFS_SIZE 4096*20

PRIVATE struct device dv;

/*===========================================================================*
 *				   main 				     *
 *===========================================================================*/
PUBLIC int main(void)
{
/* Main program. Initialize the memory driver and start the main loop. */
  u_init();			
  driver_task(&u_dtab);		
  return(OK);				
}

/*===========================================================================*
 *				 u_name					     *
 *===========================================================================*/
PRIVATE char *u_name()
{
/* Return a name for the current device. */
	return "Upper Cassing MemDisk";
}

/*===========================================================================*
 *				u_prepare				     *
 *===========================================================================*/
PRIVATE struct device *u_prepare(device)
     int device;                /* Minor device number */
{
  /* Prepare for I/O on a device: check if the minor device number is ok. */
  return &dv;
}

/*===========================================================================*
 *				u_transfer				     *
 *===========================================================================*/
PRIVATE int u_transfer(proc_nr, opcode, position, iov, nr_req)
int proc_nr;			/* proc slot nr of proc makeing the sys call*/
int opcode;			/* DEV_GATHER (read) or DEV_SCATTER (write) */
off_t position;			/* offset on device to read or write        */
iovec_t *iov;			/* pointer to read or write request vector  */
unsigned nr_req;		/* length of request vector                 */
{
	
  
	
  while (nr_req > 0) {   /* Loop through vector of IO operations */
	vir_bytes user_vir = iov->iov_addr; /*User program mem addresss*/
	unsigned count = iov->iov_size; /* number of byted to copy */
	
	printf("UPPER: Size:%d , DST:%d , POS:%d \n",count,user_vir,position);

	if (position >= UPPERFS_SIZE) return(OK);        /* check for EOF */
	if (position + count > UPPERFS_SIZE) count = UPPERFS_SIZE - position; /* boundy read/write */
	
	/*sys_vircopy(proc_nr,D,user_vir, SELF,seg,position, count);*/
	
	if(opcode == DEV_GATHER){
		/*Read*/
		sys_vircopy(SELF, D, MemBlocks+position , proc_nr, D, user_vir, count);
	}
	if(opcode == DEV_SCATTER){
		/*Write*/
		sys_vircopy(proc_nr, D, user_vir, SELF, D, MemBlocks+position , count);
	}
 

        /* Book the number of bytes transferred. */
        position += count;
        iov->iov_addr += count;
        if ((iov->iov_size -= count) == 0) { iov++; nr_req--; }


  }
  return(OK); 
}


/*===========================================================================*
 *				u_do_open				     *
 *===========================================================================*/
PRIVATE int u_do_open(dp, m_ptr)
struct driver *dp;
message *m_ptr;
{
  return(OK);    /*Don't need to do anythign except say we're good */
}

/*===========================================================================*
 *				u_init					     *
 *===========================================================================*/
PRIVATE void u_init()
{
  dv.dv_base = cvul64((vir_bytes)MemBlocks);
  dv.dv_size = cvul64(UPPERFS_SIZE);
  printf("upper: started\n");
}

/*===========================================================================*
 *				u_ioctl					     *
 *===========================================================================*/
PRIVATE int u_ioctl(dp, m_ptr)
struct driver *dp;			/* pointer to driver structure */
message *m_ptr;				/* pointer to control message */
{
  do_diocntl(dp, m_ptr);
  printf("upper: ioctl call\n");

}

/*===========================================================================*
 *				u_geometry				     *
 *===========================================================================*/
/* Memory devices don't have a geometry, but the outside world insists so we */
/* fake it here.                                                             */
PRIVATE void u_geometry(entry)
struct partition *entry; /* Pointer to struct to fill with the geometry */
{                        /* We only need to fill the cylinders, heads   */
                         /* and sectors here.                           */
  /* Memory devices don't have a geometry, but the outside world insists. */
	entry->heads = 1;
	entry->sectors = 32;
	entry->cylinders = UPPERFS_SIZE / (entry->heads * entry->sectors * SECTOR_SIZE);
  	printf("upper: geometry call C:%d, H:%d , S:%d  \n",entry->cylinders,entry->heads,entry->sectors = 32);
}

