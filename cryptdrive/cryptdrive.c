#include "../drivers.h"
#include <sys/ioc_disk.h>
#include <minix/com.h>
#include <unistd.h>
#include <stdio.h>

#define CD_MAJOR 23

int device_caller=0,proc_nr=0; /*pid of caller*/
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
		printf("CD: waiting for messages\n");
		/* Wait for a request to read or write a disk block. */
		if(receive(ANY, &mess) != OK) continue;
	
		device_caller = mess.m_source;
        proc_nr = mess.PROC_NR;
		printf("CD: message from %d, proc_nr\n",device_caller,proc_nr);
		/* Now carry out the work. */
		
		if(DRVR_PROC_NR==mess.m_source){
			/*from disk driver*/
			mess.REP_PROC_NR = proc_nr;
			send(device_caller, &mess);
		}else{
			/* prob from fs */
			/*proxy some message to at_wini*/
			
			switch(mess.m_type) {
				case DEV_OPEN:		
				case DEV_CLOSE:		
				case DEV_IOCTL:		
				case CANCEL:		
				case DEV_SELECT:	
				
				case DEV_READ:	
				case DEV_WRITE:	 				
				case DEV_GATHER: 
				case DEV_SCATTER:
				
					device_caller = mess.m_source;
					mess.m_source = thispid; /*make this the source*/
					send(DRVR_PROC_NR,&mess);




				case HARD_INT:
				case SYS_SIG:
				case SYN_ALARM:	continue;	/* don't reply */
							
			
		}
	}
}


PUBLIC int main(void){
/* Main program. Initialize the memory driver and start the main loop. */
	/* create nodes */
	thispid=getpid();
	printf("CryptDrive Started with pid %d\n",thispid);
	mapdriver(CD_MAJOR, thispid , STYLE_DEV);
	driver_task();
	unmapdriver(CD_MAJOR);		
	return(OK);				
}
