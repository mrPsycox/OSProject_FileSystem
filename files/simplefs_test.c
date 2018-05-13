#include "simplefs.h"
#include "bitmap.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main(int agc, char** argv) {
  //
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
  printf("\n BLOCK[1]: %d , BLOCK[2]: %d, BLOCK[3]: %d",block1,block2,block3 );
  BitMap* bitmap = malloc(sizeof(BitMap));
  bitmap->entries = entries1;
  bitmap->num_bits = 64;
  BitMap_print(bitmap);

}
