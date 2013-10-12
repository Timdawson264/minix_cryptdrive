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
  fp = fopen("output.out","rb");
  if(fp == NULL) {
    printf("File not found \n");
    return;
  }
  else {
    outFile = fopen("output2.out","wbx");
    direction = 1;
    for(i=0; i<32; i++)
      keyMaterial[i] = initKey[i];

    /*Setting the Decryption Mode*/
    cipherInit(&cipherInst, MODE_ECB, iv);
    keyInst.keyLen = blockLength;
    makeKey(&keyInst, direction, keyLength, keyMaterial);
    while ((bytesRead = fread(inBuffer,1,16,fp)) > 0) {
      /*      printf("Key is := ");
      for(i=0;i<32;i++)
	printf("%c", keyMaterial[i]);
	printf("\n");*/
      if(bytesRead >= blockLength/8)
	blockDecrypt(&cipherInst, &keyInst, (BYTE*) inBuffer, blockLength, (BYTE*) outBuffer);
      else {
	bytesRead = padDecrypt(&cipherInst, &keyInst, (BYTE*) inBuffer, bytesRead * 8, (BYTE*) outBuffer);
	printf("Bytes: %i\n",bytesRead);
      }
      /*
      XORing with the previous block
      for(i = 0; i<1024; i++)
	inBuffer[i] ^= outBuffer[i];
      */
      fwrite(outBuffer,1,bytesRead,outFile);
    }
    fclose(fp);
    fclose(outFile);
    return;
  }
}
