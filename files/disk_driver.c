#include "disk_driver.h"
#include <stdio.h>
#include "error.h"
#include <unistd.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <stdlib.h>



// opens the file (creating it if necessary)
// allocates the necessary space on the disk
// calculates how big the bitmap should be
// if the file was new
// compiles a disk header, and fills in the bitmap of appropriate size
// with all 0 (to denote the free space);
void DiskDriver_init(DiskDriver* disk, const char* filename, int num_blocks){
    //faccio i controlli iniziali sugli input
    if(disk == NULL || filename == NULL || num_blocks < 1){
      ERROR_HELPER(-1,"Impossible to init: Bad parameters\n");
    }
    printf("Started the DiskDriver initialization.\n");
    //mi calcolo la grandezza del bitmap corrispondente al diskdriver
    int bitmap_size = num_blocks/8;
    if(bitmap_size == 0) bitmap_size++; //la bitmap size non può essere 0, almeno assegno un blocco ovviamente

    int fd;   //descrittore
    int file_exists = access(filename,F_OK) == 0;  //attraverso la access,controllo se il filename fa riferimento veramente
                                                // a un file (F_OK). Quel bit mi permette di dividere il flusso (se il file esiste,
                                                // se il file non esiste).
    if(file_exists){
      printf("File already exists\n");

      fd = open(filename,O_RDWR,(mode_t)0666);                                  //apro il descrittore
      ERROR_HELPER(fd,"Impossible to init: Error when open the file descriptor\n");//gestisco l'errore in caso negativo

      //alloco il diskheader, dando come permessi che la pagina può essere sia scritta che letta
      DiskHeader* diskheader = (DiskHeader*)mmap(0,sizeof(DiskHeader)+bitmap_size,PROT_READ | PROT_WRITE, MAP_SHARED, fd ,0);
      //nel caso la map fallisca, chiudo il descrittore e gestisco il fallimento dell'operazione
      if(diskheader == MAP_FAILED){
          close(fd);
          ERROR_HELPER(-1,"Impossible to init: map failed in disk header\n");
      }
      //metto l'header appena creato dentro l'il DiskDriver che ho in input
      disk->header = diskheader;
      disk->bitmap_data = (char*)diskheader + sizeof(DiskHeader);     //la bitmap_data invece fa riferimento al ???

    }else{  //da qua parte il flusso che prevede che il file non esista

        printf("File doesn't exists\n");

        //apro il descrittore e gestisco l'eventuale errore,
        //stavolta con il flag O_CREAT (creazione) e O_TRUNC(nel caso il file fosse gia
        //aperto chiudo eventuali connessioni con altri descrittori)
        fd = open(filename,O_RDWR | O_CREAT | O_TRUNC, (mode_t)0666);
        ERROR_HELPER(fd,"Impossible to init: Error when open the file descriptor");

        //la posix_fallocate ritorna 0 in caso di successo, un numero maggiore di zero in caso di errore
        //questa funzione mi assicura che l'allocazione di memoria che dovrò fare per quel descrittore dato in input
        //avrà sufficientemente spazio
        if(posix_fallocate(fd, 0 , sizeof(DiskHeader) + bitmap_size) > 0){
            printf("Impossible to init: Error in posix_fallocate\n");
            return;
        }

        //riscrivo l'allocazione del disk DiskHeader
        DiskHeader* diskheader = (DiskHeader*)mmap(0, sizeof(DiskHeader) + bitmap_size, PROT_READ | PROT_WRITE, MAP_SHARED, fd ,0);

        //ricontrollo se la mmap avviene con successo o meno
        if(diskheader == MAP_FAILED){
          close(fd);
          ERROR_HELPER(-1,"Impossible to init: map failed in disk header\n");
        }

        //ora assegno l'header al disco, e poi assegno ogni campo della struct Diskheader con le informazioni ottenute
        disk->header = diskheader;
        disk->bitmap_data = (char*)diskheader + sizeof(DiskHeader);
        //nel caso il file esista quest'operazione non era necessaria, in questo caso si perchè il file che poggia il file system
        //non ha le informazioni necessarie, naturalmente riempiamo:
        diskheader->num_blocks = num_blocks;    //dato in input
        diskheader->bitmap_blocks = num_blocks; //  ""
        diskheader->bitmap_entries = bitmap_size;// ottenuto prima per conoscere quante entries devo mettere
        diskheader->free_blocks = num_blocks;
        diskheader->first_free_block = 0;
        //ora metto tutti i bit di bitmap data con lunghezza bitmap_size a 0
        bzero(disk->bitmap_data,bitmap_size);


    }
    //la fase piu importante, cioè l'assegnazione al disco del descrittore usato per la creazione del disk header
    disk->fd = fd;
    printf("Finished the disk_driver initialization\n");
}


// reads the block in position block_num
// returns -1 if the block is free accrding to the bitmap
// 0 otherwise
int DiskDriver_readBlock(DiskDriver* disk, void* dest, int block_num){
  // checking if params are ok
    if(block_num > disk->header->bitmap_blocks || block_num < 0 || dest == NULL || disk == NULL ){
		printf("DiskDriver_readBlock: bad parameters");
        return -1;
	}
    //setting the bitmap
    BitMap bit_map;
    bit_map.num_bits = disk->header->bitmap_blocks;
    bit_map.entries = disk->bitmap_data;

    // check if that block is free
    if(block_num >= bit_map.num_bits) return -1;   // invalid block
    BitMapEntryKey entry_key = BitMap_blockToIndex(block_num);
    if(!(bit_map.entries[entry_key.entry_num] >> entry_key.bit_num & 0x01)){ // controllo se il blocco è libero
        printf("Cannot read block: is free!\n");
        return -1;
    }

    int fd = disk->fd;
    // lseek on DISKHEADER+BITMAPENTRIES+MYBLOCKOFFSET
    //Shifto il descrittore di tanti bit quanto l'offset del blocco che devo leggere
    //Leggo il file partendo da block num
    off_t off = lseek(fd,sizeof(DiskHeader)+disk->header->bitmap_entries+(block_num*BLOCK_SIZE), SEEK_SET);
    if(off == -1){
        printf("DiskDriver_readBlock: lseek error\n");
        return -1;
    }

    int ret, bytes_reads = 0;
    // read until the whole BLOCK_SIZE is covered
	while(bytes_reads < BLOCK_SIZE){
        // save bytes_read in dest location
		ret = read(fd, dest + bytes_reads, BLOCK_SIZE - bytes_reads);

		if (ret == -1 && errno == EINTR) continue;
		else if(ret==-1 && errno !=EINTR) return -1;
		bytes_reads +=ret;

	}

    return 0;
}


  // returns the first free blockin the disk from position (checking the bitmap)
  int DiskDriver_getFreeBlock(DiskDriver* disk, int start){
    if(disk == NULL || start < 0)
    ERROR_HELPER(-1,"DiskDriver_getFreeBlock: Bad parameters in input");



    //mi creo la bitmap
    BitMap bitmap;
    bitmap.num_bits = disk->header->bitmap_blocks;
    bitmap.entries = disk->bitmap_data;

    //utilizzo la BitMap_get per prendermi il primo blocco con status 0
    return BitMap_get(&bitmap,start,0);     //dato che la bitmap get prende un puntatore a bitmap,
                                            //gli passo il suo indirizzo logico
  }

  // writes a block in position block_num, and alters the bitmap accordingly
  // returns -1 if operation not possible
  int DiskDriver_writeBlock(DiskDriver* disk, void* src, int block_num){
      if(disk == NULL || src == NULL || block_num < 0)
      ERROR_HELPER(-1,"DiskDriver_writeBlock: Bad parameters in input");

      //mi creo la bitmap come in precedenza
      BitMap bitmap;
      bitmap.num_bits = disk->header->bitmap_blocks;
      bitmap.entries = disk->bitmap_data;

      //controllo se il blocco è libero
      if(block_num > disk->header->num_blocks) return -1;     //controllo di base
      BitMapEntryKey entry = BitMap_blockToIndex(block_num);
      if((bitmap.entries[entry.entry_num] >> entry.bit_num & 1)){
        return -1;   //ritorno -1 se il blocco è già scritto, cioè non faccio nulla
      }
      //sapendo che il blocco su cui sto per scrivere è libero, devo aggiornare la bitmap sul primo blocco dispondibile
      if(block_num == disk->header->first_free_block)
        disk->header->first_free_block = DiskDriver_getFreeBlock(disk,block_num+1);

      //ora devo decrescere nella bitmap il numero di blocchi liberi, dato che ne sto scrivendo uno
      //inoltre setto lo status a 1 del Bitmap corrispondente
      BitMap_set(&bitmap,block_num,1);
      disk->header->free_blocks - 1;

      //ora mi prendo il descrittore
      int fd = disk->fd;
      //uso la lseek per posizionare l'indice di scrittura al blocco corrispondente
      off_t off = lseek(fd, sizeof(DiskHeader) + disk->header->bitmap_entries + (block_num * BLOCK_SIZE), SEEK_SET);
      if(off == -1){
        printf("DiskDriver_writeBlock: error setting the index to write the block (lseek)\n");
        return -1;
      }
      // ora vado a scrivere il file
      int bytes_written, ret;
      while( bytes_written < BLOCK_SIZE){
        ret = write(fd, src + bytes_written, BLOCK_SIZE - bytes_written);
        if(ret == -1 && errno == EINTR) continue;
        else if(ret == -1 && errno == EINTR) return -1;
        bytes_written += ret;
      }
      return 0;
  }

	// frees a block in position block_num, and alters the bitmap accordingly
	// returns -1 if operation not possible
int DiskDriver_freeBlock(DiskDriver* disk, int block_num){
  if(disk == NULL || block_num < 0 || block_num > disk->header->bitmap_blocks){
        printf("DiskDriver_freeBlock: bad parameters\n");
        return -1;
    }
    //setting the bitmap
    BitMap bit_map;
    bit_map.num_bits = disk->header->bitmap_blocks;
    bit_map.entries = disk->bitmap_data;

    // check if that block is already free
    BitMapEntryKey entry_key = BitMap_blockToIndex(block_num);
    if(!(bit_map.entries[entry_key.entry_num] >> entry_key.bit_num & 0x01)){ // se è =0 ritorno -1 poichè è già libero
    		printf("This block is already free\n");
        return -1;
        }

	int fd = disk->fd;
    // lseek on DISKHEADER+BITMAPENTRIES+MYBLOCKOFFSET
    //Shifto il descrittore di tanti bit quanto l'offset del blocco che devo scrivere
    //scrivo il file partendo da block num
    off_t off = lseek(fd,sizeof(DiskHeader)+disk->header->bitmap_entries+(block_num*BLOCK_SIZE), SEEK_SET);
    if(off == -1){
        printf("DiskDriver_readBlock: lseek error\n");
        return -1;
    }

	// filling block with '0'
	char zero_buffer[BLOCK_SIZE] = {0};
    int ret, bytes_written = 0;
    // read until the whole BLOCK_SIZE is covered
	while(bytes_written < BLOCK_SIZE){
        // write src on block_num pos
		ret = write(fd, zero_buffer + bytes_written, BLOCK_SIZE - bytes_written);

		if (ret == -1 && errno == EINTR) continue;

		bytes_written +=ret;
	}

	ret = BitMap_set(&bit_map, block_num, 0);//setto il blocco a 0 poichè ora è libero
    if(ret == -1){
        printf("DiskDriver_freeBlock: BitMap_set error\n");
        return -1;
    }

    // aggiorno il first_free_block
    // 1. è prima dell'attuale first_free_block?
    // 2. l'attuale first_free_block non è settato (-1)?
    if(block_num < disk->header->first_free_block || disk->header->first_free_block == -1)																//update first_free_block
        disk->header->first_free_block = block_num;
        disk->header->free_blocks += 1;

    return 0;
}

// writes the data (flushing the mmaps)
// completo le precedenti operazioni e faccio update del file
int DiskDriver_flush(DiskDriver* disk){
	printf("Disk_Driver_Flush : flush started\n");
	int bitmap_size = disk->header->num_blocks/8+1;
	int ret = msync(disk->header, sizeof(DiskHeader)+bitmap_size, MS_SYNC);
		if (ret==-1){
    	printf(" ERROR!!!! Could not sync the file to disk\n");
    }
    printf("Disk_Driver_Flush : flush end\n");
	return 0;
}
