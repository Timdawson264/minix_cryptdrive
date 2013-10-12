#include "../drivers.h"
#include <sys/ioc_disk.h>
#include <minix/com.h>
#include <unistd.h>
#include <stdio.h>

#define CD_MAJOR 23

int device_caller=0; /*pid of caller*/
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
		if(device_caller==0)
			if(receive(ANY, &mess) != OK) continue;
		else
			if(receive(DRVR_PROC_NR, &mess) != OK) continue;
	
		
		printf("CD: message from %d\n",device_caller);
		/* Now carry out the work. */
		
		if(DRVR_PROC_NR==mess.m_source){
			/*from disk driver*/
			mess.REP_PROC_NR = thispid;
			send(device_caller, &mess);
			device_caller = 0;
		}else{
			/* prob from fs */
			/*proxy message to at_wini*/
			device_caller = mess.m_source;
			mess.m_source = thispid; /*make this the source*/
			send(DRVR_PROC_NR,&mess);
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
