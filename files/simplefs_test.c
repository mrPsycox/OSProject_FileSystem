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
}
