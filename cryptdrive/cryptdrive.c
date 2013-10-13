#include "rijndael-api-fst.h"
#include "../drivers.h"
#include <minix/const.h>
#include <minix/minlib.h>
#include <sys/ioc_memory.h>
#include <sys/ioc_disk.h>
#include <stdio.h>
#include <assert.h>
#include <unistd.h>

#include <string.h>
#include <signal.h>
#include <stdlib.h>
#include <limits.h>
#include <stddef.h>


#define CD_MAJOR 23
#define BUF_LEN 4096

int device_caller=0,proc_nr=0; /*pid of caller*/
int thispid,s;

char * buffer[BUF_LEN];

/*Encryption/Decryption stuff*/
keyInstance keyInst;
cipherInstance cipherInst;
char *keyMaterial =  "abcdeabcdeabcdeabcdeabcdeabbaabb"; /* 128bit key */

/*===========================================================================*
 *				do_rdwt					     *
 *===========================================================================*/
PUBLIC int do_rdwt(message *mp)
/* mp - pointer to read or write message */
{
	/* Carry out a single read or write request. */
	iovec_t iovec1;
	int r, opcode;
	phys_bytes phys_addr;
	vir_bytes user_vir;
	/* Disk address?  Address and length of the user buffer? */
	if (mp->COUNT < 0) return(EINVAL);

	/* Check the user buffer. */
	sys_umap(mp->m_source, D, (vir_bytes) mp->ADDRESS, mp->COUNT, &phys_addr);
	if (phys_addr == 0) return(EFAULT);
	
	if(mp->COUNT>BUF_LEN) panic("CryptDrive","buffer is too small",s);
	printf("Request size is , %u bytes",mp->COUNT); /*debug*/
		
	if(opcode == DEV_READ){
		/*from here to caller*/
		user_vir = (vir_bytes) mp->ADDRESS;
		mp->ADDRESS = (vir_bytes) buffer; /* use my buffer */
		mp->m_source=thispid;
	
		if(OK != sendrec(DRVR_PROC_NR, mp))
			panic("CryptDrive","do_rd messaging failed",s);
		
		/* decrypt here  - this line here */
		sys_vircopy(SELF, D, buffer, device_caller, D, user_vir, mp->COUNT);
		
		mp->m_source=thispid;
		if(OK != send(device_caller, mp))
			panic("CryptDrive","do_wt messaging failed",s);
		
	}
	if(opcode == DEV_WRITE){
		/*from caller to here*/
		sys_vircopy(device_caller, D, mp->ADDRESS, SELF, D, buffer, mp->COUNT);
		
		user_vir = mp->ADDRESS;
		mp->ADDRESS= (vir_bytes) buffer; /* use my buffer */
		mp->m_source=thispid;
	
		if(OK != sendrec(DRVR_PROC_NR, mp))
			panic("CryptDrive","do_wt messaging failed",s);
			
		mp->m_source=thispid;
		if(OK != send(device_caller, mp))
			panic("CryptDrive","do_wt messaging failed",s);
	}

	return(OK);
}


/*==========================================================================*
 *				do_vrdwt				    *
 *==========================================================================*/
PRIVATE int do_vrdwt(message* mp)
 /* mp - pointer to read or write message */
{
/* Carry out an device read or write to/from a vector of user addresses.
 * The "user addresses" are assumed to be safe, i.e. FS transferring to/from
 * its own buffers, so they are not checked.
 */
  static iovec_t iovec[NR_IOREQS];
  iovec_t *iov;
  phys_bytes iovec_size;
  unsigned nr_req, position;
  int r;
  message m_dd; /*message for disk driver*/
  nr_req = mp->COUNT;	/* Length of I/O vector */
  position = mp->POSITION;

		/* Copy the vector from the caller to kernel space. */
		if (nr_req > NR_IOREQS) nr_req = NR_IOREQS;
			iovec_size = (phys_bytes) (nr_req * sizeof(iovec[0]));

		if (OK != sys_datacopy(mp->m_source, (vir_bytes) mp->ADDRESS, 
				SELF, (vir_bytes) iovec, iovec_size))
			panic("Crypt Drive","bad I/O vector by", s);
		iov = iovec;
  
	while(nr_req>0){
		vir_bytes user_vir = iov->iov_addr; /*User program mem addresss*/
		unsigned count = iov->iov_size; /* number of byted to copy */
		
		printf("CryptDrive: Size:%d , DST:%d , POS:%d, REQ: %d/%d \n",count, user_vir, position, nr_req, mp->COUNT);
		
		if(mp->m_type == DEV_GATHER){
			m_dd.m_type=DEV_READ;
			m_dd.DEVICE=mp->DEVICE;
			m_dd.m_source=thispid;
			m_dd.COUNT=count;
			m_dd.POSITION=position;
			m_dd.ADDRESS=buffer;	
			if(OK != sendrec(DRVR_PROC_NR, &m_dd))
				panic("CryptDrive","do_rdv messaging failed",s);
			
			/* decrypt here  - this line here */
			/*from here to caller*/
			sys_vircopy(SELF, D, buffer, device_caller, D, user_vir, m_dd.COUNT);
			
		}
		if(mp->m_type == DEV_SCATTER){
			/*from caller to here*/
			sys_vircopy(device_caller, D, user_vir, SELF, D, buffer, count);
			/*ENCYPT Here - this line no more no less*/
			
			m_dd.m_type=DEV_WRITE;
			m_dd.DEVICE=mp->DEVICE;
			m_dd.m_source=thispid;
			m_dd.COUNT=count;
			m_dd.POSITION=position;
			m_dd.ADDRESS=buffer;	
			if(OK != sendrec(DRVR_PROC_NR, &m_dd))
				panic("CryptDrive","do_wtv messaging failed",s);
			count=m_dd.REP_STATUS;
		}
		
		/* Book the number of bytes transferred. */
        position += count;
        iov->iov_addr += count;
        if ((iov->iov_size -= count) == 0) { iov++; nr_req--; } /* vector done; next request */
	}


		/* Copy the I/O vector back to the caller. */
		if (mp->m_source >= 0) {
			sys_datacopy(SELF, (vir_bytes) iovec, 
				mp->m_source, (vir_bytes) mp->ADDRESS, iovec_size);
		}
	
		mp->m_type = TASK_REPLY;
		mp->REP_PROC_NR = proc_nr;
		mp->m_source=thispid;
		/* Status is ok */
		mp->REP_STATUS = OK;	
		send(device_caller, mp);
		
		return(OK);
}

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
        
		printf("CD: message from %u, type %u\n",device_caller,proc_nr,mess.m_type);
		/* Now carry out the work. */
		
		switch(mess.m_type) {
				case DEV_OPEN:		
				case DEV_CLOSE:		
				case DEV_IOCTL:		
				case CANCEL:		
				case DEV_SELECT:	
					/* forwards message to diskdriver and  forwards response to caller*/
					device_caller = mess.m_source;
					mess.m_source = thispid; /*make this the source*/
					if(OK != sendrec(DRVR_PROC_NR, &mess))
						panic("CryptcDrive","2 Message not sent back",s);
					printf("CD: waiting for diskdriver\n");	
					
					mess.m_source = thispid; /*make this the source*/
					if(OK != send(device_caller, &mess))
						panic("CryptDrive","3 Message not sent back",s);
					printf("CD: message to %u\n",device_caller);
					break;

				case DEV_READ:	
				case DEV_WRITE:	 		
						do_rdwt(&mess);
						break;
				case DEV_GATHER: 
				case DEV_SCATTER:
						do_vrdwt(&mess);				
						break;

				case HARD_INT:
				case SYS_SIG:
				case SYN_ALARM:	break;	/* don't reply */
				
			
		}
	}
}

void encryptBuffer(char* buffer,int bufferSize){
  int i;
  char *currentBlock;
  if(bufferSize % 16 != 0) {
    /*Panic*/
    return;
  }
  /*Setting the key direction to Enpcrypt*/
  keyInst.direction = DIR_ENCRYPT;
  makeKey(&keyInst, DIR_ENCRYPT, 128,keyMaterial);
  cipherInit(&cipherInst, MODE_ECB, NULL);
  /*Encrypt the buffer*/
  for(i = 0; i< bufferSize; i+=16) {
    /*Copying out a block*/
    memcpy(currentBlock,buffer+i,16);
    /*Encrypting block*/
    blockEncrypt(&cipherInst, &keyInst, currentBlock, 16 * 8, currentBlock);
    /*Copying in the encrypted block*/
    memcpy(buffer+i,currentBlock,16);
  }
  return;
}

void decryptBuffer(char* buffer,int bufferSize){
  int i;
  char *currentBlock;
  if(bufferSize % 16 != 0) {
    /*Panic*/
    return;
  }
  /*Setting the key direction to Depcrypt*/
  keyInst.direction = DIR_DECRYPT;
  makeKey(&keyInst, DIR_DECRYPT, 128,keyMaterial);
  cipherInit(&cipherInst, MODE_ECB, NULL);
  /*Decrypt the buffer*/
  for(i = 0; i< bufferSize; i+=16) {
    /*Copying out a block*/
    memcpy(currentBlock,buffer+i,16);
    /*Encrypting block*/
    blockDecrypt(&cipherInst, &keyInst, currentBlock, 16 * 8, currentBlock);
    /*Copying in the Decrypted block*/
    memcpy(buffer+i,currentBlock,16);
  }
  return;
}

PUBLIC int main(void){
/* Main program. Initialize the memory driver and start the main loop. */
	/* create nodes */
	thispid=getpid();
	printf("CryptDrive Started with pid %d\n",thispid);
	printf("max number of requests in vector=%d\n",NR_IOREQS);
	mapdriver(CD_MAJOR, thispid , STYLE_DEV);
	driver_task();
	unmapdriver(CD_MAJOR);		
	return(OK);				
}
