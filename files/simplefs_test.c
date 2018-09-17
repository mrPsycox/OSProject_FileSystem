#include "simplefs.h"
#include "bitmap.h"
#include "disk_driver.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main(int agc, char** argv) {
  //DA QUA PARTONO I TEST PER OGNI FUNZIONE BITMAP, IN PIU UTILIZZO LA BITMAP PRINT FATTA DA ME
  int i=0;
  char entries1[8] = "11101000";
  unsigned char block1 = 0;
  char entries2[8] = "00011000";
  unsigned char block2 = 0;
  char entries3[8] = "00000000";
  unsigned char block3 = 0;
  printf("\n ------------ STARTING BITMAP TEST -----------------\n\n");
  for(int i = 0; i < 8; i++){
    block1 |= (entries1[i] == '1') << (7 - i);
    block2 |= (entries2[i] == '1') << (7 - i);
    block3 |= (entries3[i] == '1') << (7 - i);
  }
  printf("\n BLOCK[1]: %d , BLOCK[2]: %d, BLOCK[3]: %d\n",block1,block2,block3 );
  BitMap* bitmap = malloc(sizeof(BitMap));
  bitmap->entries = entries1;
  bitmap->num_bits = 32;
  BitMap_print(bitmap);
  printf("\n ----------------- BITMAP_TEST: BLOCK_TO_INDEX --------------\n" );
  int j;
  for(j=1; j < 20; j++){
    BitMapEntryKey entry_key = BitMap_blockToIndex(j);
    printf("block:%d entry:%d index: %d\n" ,j, entry_key.entry_num, entry_key.bit_num);
  }
  printf("\n ----------------- BITMAP_TEST: INDEX_TO_BLOCK --------------\n" );
  for(i = 0; i < 4; i++){
    for(j = 0; j < 8; j++){
      printf(" entry:%d + index:%d = BLOCK:%d\n",i,j,BitMap_indexToBlock(i,j));
    }
  }
  printf("\n ----------------- BITMAP_TEST: BITMAP_GET & SET --------------\n" );
  printf("GET: get at start[%d],status[%d]: %d, EXPECTED_INDEX[1]\n",0,0,BitMap_get(bitmap,0,0));
  printf("SET: set with pos[1] and status[1]\n");
  BitMap_set(bitmap,1,1);
  printf("GET: get at start[%d],status[%d]: %d, EXPECTED_INDEX[0]\n",1,1,BitMap_get(bitmap,0,1));
  printf("SET with pos[2] and status[0]\n");
  BitMap_set(bitmap,2,0);
  printf("GET: get at start[%d],status[%d]: %d, EXPECTED_INDEX[2]\n\n",0,0,BitMap_get(bitmap,0,0));
  BitMap_print(bitmap);


  printf("\n ------------------- ENDING BITMAP_TEST ------------------------\n\n" );

  printf("--------------------- STARTING DISK_DRIVER TEST---------------------\n");

  DiskDriver* disk_driver = (DiskDriver*)malloc(sizeof(DiskDriver));
  const char* filename = "./disk.txt";
  BlockHeader block_header;
      block_header.previous_block = 2;
      block_header.next_block = 2;
      block_header.block_in_file = 2;

  //popolerò 3 blocchi su cui testerò le operazioni di lettura/scrittura
  printf("...Populating blocks[1]\n");
  FileBlock* file_block1 = (FileBlock*)malloc(sizeof(FileBlock));
  file_block1->header = block_header;
  char data1[BLOCK_SIZE-sizeof(BlockHeader)];
  for(i = 0; i < BLOCK_SIZE - sizeof(BlockHeader); i++)
    data1[i] = '1';  //populo il vettore con '1'
  data1[BLOCK_SIZE-sizeof(BlockHeader)-1] = '\0';
  strcpy(file_block1->data,data1);

  printf("...Populating blocks[2]\n");
  FileBlock* file_block2 = (FileBlock*)malloc(sizeof(FileBlock));
  file_block2->header = block_header;
  char data2[BLOCK_SIZE-sizeof(BlockHeader)];
  for(i = 0; i < BLOCK_SIZE - sizeof(BlockHeader); i++)
    data2[i] = '1';  //populo il vettore con '1'
  data2[BLOCK_SIZE-sizeof(BlockHeader)-1] = '\0';
  strcpy(file_block2->data,data2);

  printf("...Populating blocks[3]\n");
  FileBlock* file_block3 = (FileBlock*)malloc(sizeof(FileBlock));
  file_block3->header = block_header;
  char data3[BLOCK_SIZE-sizeof(BlockHeader)];
  for(i = 0; i < BLOCK_SIZE - sizeof(BlockHeader); i++)
    data3[i] = '1';  //populo il vettore con '1'
  data3[BLOCK_SIZE-sizeof(BlockHeader)-1] = '\0';
  strcpy(file_block3->data,data3);

  printf("Inizializing the disk with 3 blocks\n");
  DiskDriver_init(disk_driver,filename,3);
  printf("We have %d num blocks\n",disk_driver->header->num_blocks);
    printf("We have %d bitmap_blocks\n",disk_driver->header->bitmap_blocks);
    printf("We have %d bitmap_entries\n",disk_driver->header->bitmap_entries);
    printf("We have %d free blocks\n",disk_driver->header->free_blocks);
    printf("we have %d first_free_block\n",disk_driver->header->first_free_block);
  printf("we have Bitmap:%d\n\n",disk_driver->bitmap_data[0]);


  printf("\n...we start to writing the blocks\n");

  printf("--- WRITING BLOCK 0 ---\n" );
  DiskDriver_writeBlock(disk_driver,file_block1,0);
  printf("Free Blocks:%d\n",disk_driver->header->free_blocks);
  printf("First free block: %d\n",disk_driver->header->first_free_block);
  printf("Bitmap:%d\n",disk_driver->bitmap_data[0]);

  printf("--- WRITING BLOCK 1 ---\n" );
  DiskDriver_writeBlock(disk_driver,file_block2,DiskDriver_getFreeBlock(disk_driver,0));
  printf("Free Blocks:%d\n",disk_driver->header->free_blocks);
  printf("First free block: %d\n",disk_driver->header->first_free_block);
  printf("Bitmap:%d\n",disk_driver->bitmap_data[0]);

  printf("--- WRITING BLOCK 2 ---\n" );
  DiskDriver_writeBlock(disk_driver,file_block3,DiskDriver_getFreeBlock(disk_driver,1));

  printf("Free Blocks:%d\n",disk_driver->header->free_blocks);
  printf("First free block: %d\n",disk_driver->header->first_free_block);
  printf("Bitmap:%d\n",disk_driver->bitmap_data[0]);

  printf("... reading the blocks\n");


  FileBlock* test = malloc(sizeof(FileBlock));
  printf("Reading Block 0\n");
  DiskDriver_readBlock(disk_driver,test,0);
  printf("%s\n",test->data);
  printf("Reading Block 1\n");
  DiskDriver_readBlock(disk_driver,test,1);
  printf("%s\n",test->data);
  printf("Reading Block 2\n");
  DiskDriver_readBlock(disk_driver,test,2);
  printf("%s\n",test->data);

  printf("... testing the free blocks\n\n");


  DiskDriver_freeBlock(disk_driver,0);
  printf("set free the 0 block\n");
  DiskDriver_readBlock(disk_driver,test,0);
  printf("free blocks:%d\n",disk_driver->header->free_blocks);
  printf("first free block: %d \n",disk_driver->header->first_free_block);
  printf("bitmap: %d\n",disk_driver->bitmap_data[0]);

  DiskDriver_freeBlock(disk_driver,1);
  printf("set free the 1 block\n");
  DiskDriver_readBlock(disk_driver,test,1);
  printf("free blocks:%d\n",disk_driver->header->free_blocks);
  printf("first free block: %d \n",disk_driver->header->first_free_block);
  printf("bitmap: %d\n",disk_driver->bitmap_data[0]);

  DiskDriver_freeBlock(disk_driver,2);
  printf("set free the 2 block\n");
  DiskDriver_readBlock(disk_driver,test,2);
  printf("free blocks:%d\n",disk_driver->header->free_blocks);
  printf("first free block: %d \n",disk_driver->header->first_free_block);
  printf("bitmap: %d\n",disk_driver->bitmap_data[0]);


  free(disk_driver);
  free(test);
  free(file_block1);
  free(file_block2);
  free(file_block3);


    printf("\n ------------ ENDING DISK_DRIVER TEST -----------------\n\n");

}
