#include "disk_driver.h"
#include <stdio.h>

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
      //metto l'header appena creato dentro l'header che mi passa la funzione
      disk->header = diskheader;
      disk->bitmap_data = (char*)diskheader + sizeof(DiskHeader);

      printf("Disk free blocks: %d\n",disk->free_blocks );
      printf("Disk first free block:%d\n",disk->first_free_block );
      printf("Disk's Bitmap:\n",disk->bitmap_data[0] );    //DA CHIEDERE A SIMONE

    }else{  //da qua parte il flusso che prevede che il file non esista

        printf("File doesn't exists\n");

        //apro il descrittore e gestisco l'eventuale errore
        fd = open(filename,O_RDWR | O_CREAT | O_TRUNC, (mode_t)0666);
        ERROR_HELPER(fd,"Impossible to init: Error when open the file descriptor");

        //la posix_fallocate ritorna 0 in caso di successo, un numero maggiore di zero in caso di errore
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

        diskheader->num_blocks = num_blocks;
        diskheader->bitmap_blocks = num_blocks;
        diskheader->bitmap_entries = bitmap_size;
        diskheader->free_blocks = num_blocks;
        diskheader->first_free_block = 0;
        //ora metto tutti i bit di bitmap data con lunghezza bitmap_size a 0
        bzero(disk->bitmap_data,bitmap_size);

        printf("Disk free blocks: %d\n",disk->free_blocks );
        printf("Disk first free block:%d\n",disk->first_free_block );
        printf("Disk's Bitmap:\n",disk->bitmap_data[0] );    //DA CHIEDERE A SIMONE
    }
    //la fase piu importante, cioè l'assegnazione al disco del descrittore usato per la creazione del disk header
    disk->fd = fd;
}


// reads the block in position block_num
// returns -1 if the block is free accrding to the bitmap
// 0 otherwise
int DiskDriver_readBlock(DiskDriver* disk, void* dest, int block_num){
  


}
