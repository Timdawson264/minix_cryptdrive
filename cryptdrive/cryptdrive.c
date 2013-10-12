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

	int r,s;
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
        
		printf("CD: message from %u,\n",device_caller,proc_nr);
		/* Now carry out the work. */
		
		if(DRVR_PROC_NR==mess.m_source){
			/*from disk driver to other*/
			mess.m_source = thispid; /* make it from here */
			if(OK != send(device_caller, &mess))
				panic("CryptDrive","1 Message not sent back",s);
				printf("CD: message to %u\n",device_caller);
		}else{
			/* prob from fs */
			/*from other to diskdriver*/
			
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
					if(OK != send(DRVR_PROC_NR, &mess))
						panic("CryptDrive","2 Message not sent back",s);
					printf("CD: waiting for diskdriver\n");	
					
					if(receive(DRVR_PROC_NR, &mess) != OK) continue;
					/*from disk driver to other*/
					mess.m_source = thispid; /* make it from here */
					if(OK != send(device_caller, &mess))
						panic("CryptDrive","3 Message not sent back",s);
					printf("CD: message to %u\n",device_caller);
					

				case HARD_INT:
				case SYS_SIG:
				case SYN_ALARM:	continue;	/* don't reply */
				
			}
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
