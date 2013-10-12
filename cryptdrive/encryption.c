#include <stdio.h>
#include <stdlib.h>
#include <minix/minlib.h>
#include "rijndael-api-fst.h"

int main(int *argc,char *argv[]) {
  BYTE direction;
  keyInstance keyInst;
  cipherInstance cipherInst;
  FILE *fp,*outFile;
  int i,j,keyLength,blockLength, bytesRead;
  char *initBlock,*initKey,*iv;
  char inBuffer[1204],outBuffer[1024],keyMaterial[320];

  initKey = "abcdeabcdeabcdeabcdeabcdeabbaabb";
  iv = "abcdefghijkl";
  /*initKey = getpass("Password: ");*/
  keyLength = 128;
  blockLength = 128;

  /*Open File*/
  fp = fopen("test.txt","rb");
  if(fp == NULL) {
    printf("File not found \n");
    return;
  }
  else {
    outFile = fopen("output.out","wbx");
    for(i=0; i<32; i++)
      keyMaterial[i] = initKey[i];
    keyInst.keyLen = blockLength;
    makeKey(&keyInst, direction, keyLength, keyMaterial);
    /*Setting the Encryption Mode*/
    cipherInit(&cipherInst, MODE_ECB, iv);
    while ((bytesRead = fread(inBuffer,1,16,fp)) > 0) {
      /*      printf("Key is := ");
      for(i=0;i<32;i++)
	printf("%c", keyMaterial[i]);
	printf("\n");*/
      /*XORing with the previous block
      for(i = 0; i<1024; i++)
	inBuffer[i] ^= outBuffer[i];
      */
      if(bytesRead >= (blockLength / 8))
	blockEncrypt(&cipherInst, &keyInst, (BYTE*) inBuffer, blockLength, (BYTE*) outBuffer);
      else
	padEncrypt(&cipherInst, &keyInst, (BYTE*) inBuffer, bytesRead * 8, (BYTE*) outBuffer);
      
      fwrite(outBuffer,1,blockLength/8,outFile);
    }
    fclose(fp);
    fclose(outFile);
    return;
  }
}
