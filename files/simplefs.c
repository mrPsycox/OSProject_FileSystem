#include "bitmap.h"
#include "disk_driver.h"
#include "simplefs.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "error.h"


// initializes a file system on an already made disk
// returns a handle to the top level directory stored in the first block
DirectoryHandle* SimpleFS_init(SimpleFS* fs, DiskDriver* disk){
	if(fs == NULL || disk == NULL){
		ERROR_HELPER(-1,"SimpleFS_init: Bad parameters on input.\n");
	}
	//assegno al file system il disco 
	fs->disk = disk;
	
	//creo la prima struct da assegnare poi al directory handle di ritorno
	FirstDirectoryBlock* fdb = malloc(sizeof(FirstDirectoryBlock));
	int ret;
	ret = DiskDriver_readBlock(disk, fdb, 0); //leggo il primo blocco
	if(ret == -1){  //ciò sta a signicare che il blocco è libero
		free(fdb);		//libero il firstdirectoryblock, ritorno NULL
		return NULL;
	}
	DirectoryHandle* dh = malloc(sizeof(DirectoryHandle*));
	dh->sfs = fs;
	dh->dcb = fdb;
	dh->directory = NULL;
	dh->pos_in_block = 0;
	
	return dh;
	
}

// creates the inital structures, the top level directory
// has name "/" and its control block is in the first position
// it also clears the bitmap of occupied blocks on the disk
// the current_directory_block is cached in the SimpleFS struct
// and set to the top level directory
void SimpleFS_format(SimpleFS* fs){
	//controllo l'input
	if(fs  == NULL){
		printf("SimpleFS_format: Bad Parameters\n");
		return;
	}
	
	FirstDirectoryBlock fdb = {0};  //questo parametro viene settato a 0 cosi che pulisca tutti i dati vecchi su quella porzione di disco
	//comincio a popolare la struct di fdb che sarebbe LA ROOT DIRECTORY
	fdb.header.previous_block = -1;  // 
	fdb.header.block_in_file = 0;    // VALORI DI DEFAULT
	fdb.header.next_block = -1; 	//
	
	fdb.fcb.directory_block = -1;
	fdb.fcb.is_dir = 1;				//significa che è una directory
	fdb.fcb.block_in_disk = 0;
	strcpy(fdb.fcb.name,"/");		//copia la stringa "/" nel nome della cartella di root
	
	//ora comincio a prepapare l'header del disco associato al file system
	fs->disk->header->first_free_block = 0;
	fs->disk->header->free_blocks = fs->disk->header->num_blocks;			//logicamente i blocchi liberi devono essere uguali ai blocchi presenti
	
	int size_bitmap = fs->disk->header->bitmap_entries;
	//ora metto a 0 tutto quando i dati del disco
	memset(fs->disk->bitmap_data,'\0',size_bitmap);		//metto a '\0' == equivalente di char == 0
	
	int ret;
	ret = DiskDriver_writeBlock(fs->disk, &fdb , 0); 	//metto al blocco 0 la root directory!
	if(ret == -1){
		printf("SimpleFS_format: Cannot format! Cannot write on the block!\n");
		return;
	}
	//la funzione non ritorna nulla perchè è void, operazione di formattazione completata
}

// creates an empty file in the directory d
// returns null on error (file existing, no free blocks)
// an empty file consists only of a block of type FirstBlock
FileHandle* SimpleFS_createFile(DirectoryHandle* d, const char* filename){
	//mi prendo dall'input le struct necessarie
	
	FirstDirectoryBlock* fdb = d->dcb;
	DiskDriver* disk = d->sfs->disk;
	
	//mi calcolo il massimo spazio necessario per le due struct:
		//-First Directory Block
		//-Directory Block
		
	int max_req_space_fdb = (BLOCK_SIZE - sizeof(BlockHeader) - sizeof(FileControlBlock) - sizeof(int))/sizeof(int);
	int max_req_space_db = (BLOCK_SIZE - sizeof(BlockHeader) - sizeof(int))/sizeof(int); 
	
	//ora devo controllare che il file non esista già il file
	//cominciamo a controllare nel FirstDirectoryBlock
	if(fdb->num_entries > 0){		//se le num entries sono < 0 logicamente salta queste istruzioni
		
		FirstFileBlock ffb;	//lo userò per fare il confronto nella read block, lo faccio scorrere controllando tutti gli elementi
							//di file_blocks[i]
		int i;
		for(i = 0; i < max_req_space_fdb; i++){
			if((DiskDriver_readBlock(disk, &ffb, fdb->file_blocks[i]) && fdb->file_blocks[i] >0) != -1){		
					if(strcmp(fdb->fcb.name,filename) == 0){					//controllo se esiste un nome uguale
							ERROR_HELPER(-1,"SimpleFS_createfile: file already exists\n");
							return NULL;																								
				}
			}
		}
	//continuo a cercare se esiste un file con nome uguale
	//vado avanti al prossimo blocco (DirectoryBlock)
	int next_block = fdb->header.next_block;
	DirectoryBlock db;
	
	while(next_block != -1){			//next block è settato a -1 nel caso non abbia successivi
			int ret2;
			ret2 = DiskDriver_readBlock(disk, &db, next_block);
			if(ret2 == -1){
				printf("SimpleFS_createFile: error reading the block of Directory Block!\n");
				return NULL;
			}
			int i;
			for(i = 0; i < max_req_space_db; i++){
					if((db.file_blocks[i] > 0 && DiskDriver_readBlock(disk, &ffb, db.file_blocks[i]) != -1){
						if(strcmp(fdb->fcb.name,filename) == 0){
							printf("SimpleFS_createFile: file already exists :(\n");
							return NULL;
						}
					}
				}
			next_block = db.header.next_block;
		}
	}
	//arrivato a questo punto , vuol dire che il file non esiste già e passiamo alla sua
	// CREAZIONE!!
	
}
