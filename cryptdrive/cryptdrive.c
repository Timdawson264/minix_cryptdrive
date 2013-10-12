#include "../drivers.h"
#include <sys/ioc_disk.h>
#include <minix/com.h>
#include <unistd.h>

#define major 23

FORWARD _PROTOTYPE( int do_rdwt, (struct driver *dr, message *mp) );
FORWARD _PROTOTYPE( int do_vrdwt, (struct driver *dr, message *mp) );

int device_caller; /*pid of caller*/




/*===========================================================================*
 *				driver_task				     *
 *===========================================================================*/
PUBLIC void driver_task(void)
{
	/* Main program of any device driver task. */

	int r, proc_nr;
	message mess;

	/* Here is the main loop of the disk task.  It waits for a message, carries
	* it out, and sends a reply.
	*/
	while (TRUE) {

		/* Wait for a request to read or write a disk block. */
		if(receive(ANY, &mess) != OK) continue;

		device_caller = mess.m_source;
		proc_nr = mess.PROC_NR;

		/* Now carry out the work. */
		switch(mess.m_type) {

			/*
			case DEV_READ:	
			case DEV_WRITE:	 
			case DEV_GATHER: 
			case DEV_SCATTER: 
			*/

			case TASK_REPLY:/* relay reply back to caller */
				mess.REP_PROC_NR = proc_nr;
				send(device_caller, &mess);

			default: 
				/*proxy message to at_wini*/
				mess.m_source = mess.PROC_NR; /*make this the source*/
				mess.PROC_NR = DRVR_PROC_NR	/* make at_wini the destination */
		}

		r = EDONTREPLY;
		/* Finally, prepare and send the reply message. */
		if (r != EDONTREPLY) {
			mess.m_type = TASK_REPLY;
			mess.REP_PROC_NR = proc_nr;
			/* Status is # of bytes transferred or error code. */
			mess.REP_STATUS = r;	
			send(device_caller, &mess);
		}
	}
}


PUBLIC int main(void)
{
/* Main program. Initialize the memory driver and start the main loop. */
	/* create nodes */

	mapdriver(major, getpid() , STYLE_DEV);
	driver_task();
	unmapdriver(major);		
	return(OK);				
}
