#include "../drivers.h"
#include <sys/ioc_disk.h>
#include <minix/com.h>
#include <unistd.h>
#include <stdio.h>

#define major 23

int device_caller; /*pid of caller*/
int thispid;



/*===========================================================================*
 *				driver_task				     *
 *===========================================================================*/
PUBLIC void driver_task(void)
{
	/* Main program of any device driver task. */

	int r;
	message mess;

	/* Here is the main loop of the disk task.  It waits for a message, carries
	* it out, and sends a reply.
	*/
	while (TRUE) {

		/* Wait for a request to read or write a disk block. */
		if(receive(ANY, &mess) != OK) continue;
	
		device_caller = mess.m_source;
		printf("CD: message from %d\n",device_caller);
		/* Now carry out the work. */
		switch(mess.m_type) {

			/*
			case DEV_READ:	
			case DEV_WRITE:	 
			case DEV_GATHER: 
			case DEV_SCATTER: 
			*/
			
			case TASK_REPLY:/* relay  task reply back to caller */
				mess.REP_PROC_NR = thispid;
				send(device_caller, &mess);
				 break;
			default: 
				/*proxy message to at_wini*/
				mess.m_source = thispid; /*make this the source*/
				send(DRVR_PROC_NR,&mess);
		}
		
		r = EDONTREPLY;
		/* Finally, prepare and send the reply message. */
		if (r != EDONTREPLY) {
			mess.m_type = TASK_REPLY;
			mess.REP_PROC_NR = device_caller;
			/* Status is # of bytes transferred or error code. */
			mess.REP_STATUS = r;	
			send(device_caller, &mess);
		}
	}
}


PUBLIC int main(void){
/* Main program. Initialize the memory driver and start the main loop. */
	/* create nodes */
	thispid=getpid();
	printf("CryptDrive Started with pid %d\n",thispid);
	mapdriver(major, thispid , STYLE_DEV);
	driver_task();
	unmapdriver(major);		
	return(OK);				
}
