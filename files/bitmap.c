#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include "bitmap.h"
#define bytes_dim 8


// converts a block index to an index in the array,
// and a char that indicates the offset of the bit inside the array
BitMapEntryKey BitMap_blockToIndex(int num){
    //creo un BitMapEntryKey da restituire, ci metto l'index nell'array e l'offset
    BitMapEntryKey entry_key;
    int index = num / 8;     //l'indice è in bytes, divido per 8 per ottenerlo
    char offset = num % 8;   //l'offset mi permette capire in che blocco è posizionato il bit, lo ottengo con la divisione percentuale
    entry_key.entry_num = index;
    entry_key.bit_num = offset;
    return entry_key;
  }

  // converts a bit to a linear index
  int BitMap_indexToBlock(int entry, uint8_t bit_num){
    if(entry < 0 || bit_num < 0) return -1;   //ovviamente controllo l'input, caso negativo return -1
      return (entry * bytes_dim) + bit_num;   //moltiplico l'entry per 8, e aggiungo il bit_num da input per ottenere l'indice
  }
  // returns the index of the first bit having status "status"
  // in the bitmap bmap, and starts looking from position start
  int BitMap_get(BitMap* bmap, int start, int status){
      if(start > bmap->num_bits) return -1;  //ovviamente, se la partenza è maggiore del numero dei bit, è un po inutile il tutto



      while (start < bmap->num_bits) {
          if(BitMap_getBit(bmap, start) == status) return start;
          start++;
      }
      printf("Bitmap_Get: No more free blocks!\n");
      return -1;
  }

  // sets the bit at index pos in bmap to status
  int BitMap_set(BitMap* bmap, int pos, int status){
    if(pos > bmap->num_bits || pos < 0 || status < 0) return -1;         //faccio i controlli di routine
    BitMapEntryKey new_map = BitMap_blockToIndex(pos);

    unsigned char flag = 1 << new_map.bit_num;                          //mi creo questo flag per riusarlo dopo comtrollo
    if(status == 1){
        bmap->entries[new_map.entry_num] = bmap->entries[new_map.entry_num] | flag;
        return bmap->entries[new_map.entry_num] | flag;               // questa operazione copia un bit se esiste in entrambi gli operandi, stessa cosa che fa prima nell'assegnazione al bitmap dato in input
    }else{
        bmap->entries[new_map.entry_num] = bmap->entries[new_map.entry_num] & (~flag);
        return bmap->entries[new_map.entry_num] & (~flag);           // TILDE_FLAG: ha l'effetto di sfogliare i bit, perciò li comparo uno a uno.
    }
    return 0;

  }

  void BitMap_print(BitMap* bitmap){
    int i = 0;
    for(i =0; i < bitmap->num_bits; i++){
      BitMapEntryKey entry = BitMap_blockToIndex(i);                                                      //come in precedenza mi calcolo lo status con lo shift logico destro nella posizione della nuova entry.
      printf("Entry num ---> %d | bit_num ---> %d | status ---> %d\n",  entry.entry_num,  entry.bit_num,  bitmap->entries[entry.entry_num] >> entry.bit_num & 1);
    }
  }

  int BitMap_getBit(BitMap* bmap, int pos){
    if(pos >= bmap->num_bits) return -1;                                //error, start not in range of bitmap blocks

    BitMapEntryKey map = BitMap_blockToIndex(pos);                      //convert to easy use shift
    return bmap->entries[map.entry_num] >> map.bit_num & 0x01;
}
