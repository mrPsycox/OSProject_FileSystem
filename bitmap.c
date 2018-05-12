#include <stdio.h>
#include <"bitmap.h">
#define bytes_dim = 8;


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
      if(start > bmap.num_bits) return -1;  //ovviamente, se la partenza è maggiore del numero dei bit, è un po inutile il tutto

      int i = 0;
      int j = 0;
      char status = status;     //prendo lo status e lo casto subito a bit, operazione che mi permette di riusarlo dopo per controllare se la posizione è occupata o meno
      for(i = 0; i < bmap.num_bits - start ; i++){
        BitMapEntryKey new_map = BitMap_blockToIndex(start+i);          //mi creo la bitmap entry ker partendo dalla posizione start+i
        if(bmap.entries[new_map.entry_num] >> new_map.bit_num) & status) //faccio shift logico destro mettendo la nuova entry, a condizione che lo status sia disponibile
            return BitMap_indexToBlock(new_map.entry_num,new_map.bit_num);
      }
      return -1;
  }

  // sets the bit at index pos in bmap to status
  int BitMap_set(BitMap* bmap, int pos, int status){
    if(pos > bmap.num_bits || pos < 0 || status < 0) return -1;         //faccio i controlli di routine
    BitMapEntryKey new_map = BitMap_blockToIndex(pos);

    unsigned int flag = status;     //Da chiarire...
    flag = flag << new_map.bit_num;
    bmap.entries[ new_map.entry_num] = bmap.entries[ new_map.entry_num] | status;  //capita l'assegnazione ma lo "|" non so
    return 0;   //operazione positiva, ritorno 0 per default
  }

  void BitMap_print(BitMap* bitmap){
    int i = 0;
    for(i =0; i < bitmap.entry_num; i++){
      BitMapEntryKey entry = BitMap_indexToBlock(i);                                                      //come in precedenza mi calcolo lo status con lo shift logico destro nella posizione della nuova entry.
      printf("Entry num ---> %d | bit_num ---> %d | status ---> %d\n",  entry.entry_num,  entry.bit_num,  bitmap.entries[entry.entry_num] >> entry.bit_num & 1);
    }
  }
