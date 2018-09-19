#pragma once

#include <stdint.h>

typedef struct{
  int num_bits;
  char* entries;
}  BitMap;

typedef struct {
  int entry_num;
  char bit_num;
} BitMapEntryKey;

//codesta funziona stampa i famosi bitmap magici
void BitMap_print(BitMap* bitmap);

// converts a block index to an index in the array,
// and a char that indicates the offset of the bit inside the array
BitMapEntryKey BitMap_blockToIndex(int num);

// converts a bit to a linear index
int BitMap_indexToBlock(int entry, uint8_t bit_num);

// returns the index of the first bit having status "status"
// in the bitmap bmap, and starts looking from position start
int BitMap_get(BitMap* bmap, int start, int status);

// sets the bit at index pos in bmap to status
int BitMap_set(BitMap* bmap, int pos, int status);

//returns the value of the bit in position pos
int BitMap_getBit(BitMap* bmap, int pos);
