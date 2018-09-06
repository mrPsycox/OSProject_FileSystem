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
	if(ret == -1){  //ciò sta a signicare che il blocco è libero, perciò non ho piu bisogno del fdb
		free(fdb);
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
	//orainizializzo il mettendo a '\0' tutti quanti i dati del disco,
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
	//calcolo questo spazio_max controllando nel .h le allocazioni di struct necessarie da fare
	int max_req_space_fdb = (BLOCK_SIZE - sizeof(BlockHeader) - sizeof(FileControlBlock) - sizeof(int))/sizeof(int);
	int max_req_space_db = (BLOCK_SIZE - sizeof(BlockHeader) - sizeof(int))/sizeof(int);

	//PRIMO STEP: ora devo controllare che il file non esista già
	//cominciamo a controllare nel FirstDirectoryBlock
	if(fdb->num_entries > 0){		//se le num entries sono < 0 non vado nemmeno a cercare

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
					if((db.file_blocks[i] > 0 && DiskDriver_readBlock(disk, &ffb, db.file_blocks[i]) != -1)){
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
	// prendiamo il primo blocco libero
	int new_freeblock = DiskDriver_getFreeBlock(disk,disk->header->first_free_block);
	if(new_freeblock == -1){
		printf("SimpleFS_createFile: cannot have a new free block from the disk :(\n");
		return NULL;
	}
	FirstFileBlock* newfile_created = calloc(1,sizeof(FirstFileBlock));   //uso la calloc per preparare l'allocazione
	//già pronta per l'uso, senza l'utilizzo di una memset successiva
	//popolo la struct
	newfile_created->header.block_in_file = 0;	//setto ai valori di default
	newfile_created->header.next_block = -1;
	newfile_created->header.previous_block = -1;

	strcpy(newfile_created->fcb.name,filename);  // copio il nome del filename
	newfile_created->fcb.block_in_disk = new_freeblock;  //	assegno il blocco libero ottenuto in precedenza
	newfile_created->fcb.directory_block = fdb->fcb.block_in_disk;   //assegno al nuovo file il blocco nel disco nel FileControlBlock ottenuto da input
	newfile_created->fcb.is_dir = 0; //0 file, 1 directory

	//ora provo a scrivere il blocco sul disco
	int ret;
	ret = DiskDriver_writeBlock(disk,newfile_created, new_freeblock);
	if(ret == -1){
		printf("SimpleFS_createFile: cannot write the new free block on disk");
		return NULL;
	}





	//ora comincio a controllare la presenza di blocchi liberi sia nel first directoryblock che nel directory block
	int block_found = 0; //0 in caso negativo, 1 in caso positivo cioe spazio libero trovato
	int block_entry = 0; //mi salvo la entry del blocco in modo da tracciarne la posizione
	int not_spacein_firstdirectoryblock = 0; //0 se sta nel firstdirectoryblock, 1 directory block
	int block_number = fdb->fcb.block_in_disk;
	int blockinFile = 0; //numero delle iterazioni?
	int file_saved_on_fdb_or_db = 0; // 0 se il file viene salvato nel first directory block,
									// 1 altrimenti


	DirectoryBlock last_db; //la utilizzo come appoggio

	if(fdb->num_entries < max_req_space_fdb){
			int* blocks = fdb->file_blocks;
			for(int j = 0; j < max_req_space_fdb; j++){
				if(blocks[j] == 0){  //significa che tra i blocchi ce ne è uno libero
					block_found = 1;
					block_entry = j;
					break;
				}
			}
	}else{  //in caso che lo spazio non è presente nel firstdirectory, provo nel directory blocks
			not_spacein_firstdirectoryblock = 1;   //mi salvo questa informazione
			int next_block = fdb->header.next_block;

			while(next_block != -1 && !block_found){ //finche ho un nuovo blocco libero e finche non ho trovato uno spazio
				ret = DiskDriver_readBlock(disk,&last_db,next_block);
				if(ret == -1){
					printf("SimpleFS_createFile: cannot read a block searching in DirectoryBlock");
					DiskDriver_freeBlock(disk, fdb->fcb.block_in_disk);
					return NULL;
				}
				int* blocks = fdb->file_blocks;		//cerco nello stesso modo di prima
				block_number = next_block;
				blockinFile++;
				for(int i = 0; i < max_req_space_db;i++){
					if(blocks[i] == 0){
						block_found = 1;
						block_entry = i;
						break;
					}
				}
				file_saved_on_fdb_or_db = 1;
				next_block = last_db.header.next_block;
			}
		}
		if(!block_found){ //in caso non ci sia piu spazio
			//devo allocare un un nuovo directory block e popolare il suo header
			DirectoryBlock new_db = {0};
			new_db.header.next_block = -1;
			new_db.header.block_in_file = blockinFile;
			new_db.header.previous_block = block_number;
			new_db.file_blocks[0] = new_freeblock;

			int new_dir_block = DiskDriver_freeBlock(disk,disk->header->first_free_block); //ora mi trovo il blocco necessario libero
			if(new_dir_block == -1){
				printf("SimpleFS_CreateFile: impossibile to create file\n");
				DiskDriver_freeBlock(disk,fdb->fcb.block_in_disk);
				return NULL;
			}

			int ret;
			ret = DiskDriver_writeBlock(disk, &new_db, new_dir_block); //scrivo sul blocco
			if(ret == -1){
				printf("SimpleFS_createFile: error writing in a block\n");
				DiskDriver_freeBlock(disk,fdb->fcb.block_in_disk);
				return NULL;
			}

			if(file_saved_on_fdb_or_db == 0 ){		//ora controllo se il blocco l'ho posizionato nel
					fdb->header.next_block = new_dir_block;//fdb o nel db, a seconda di cio altero
			}else{									// la posizione del next block
				last_db.header.next_block = new_dir_block;
			}
			last_db = new_db;			//aggiorno il directory block al nuovo directory block
			block_number = new_dir_block;
		}

		if(not_spacein_firstdirectoryblock == 0){ //il file è in fdb
			fdb->num_entries++;
			fdb->file_blocks[block_entry] = new_freeblock;
			DiskDriver_freeBlock(disk,fdb->fcb.block_in_disk);
			DiskDriver_writeBlock(disk, fdb, fdb->fcb.block_in_disk);
		}else{//il file è nei directory blocks
			fdb->num_entries++;
			DiskDriver_freeBlock(disk,fdb->fcb.block_in_disk);
			DiskDriver_writeBlock(disk,fdb,fdb->fcb.block_in_disk);
			last_db.file_blocks[block_entry] = new_freeblock;
			DiskDriver_freeBlock(disk,block_number);
			DiskDriver_writeBlock(disk,&last_db,block_number);
		}

		FileHandle* file_h = malloc(sizeof(FileHandle));
		file_h->sfs = d->sfs;
		file_h->fcb = newfile_created;
		file_h->directory = fdb;
		file_h->pos_in_file = 0;

		return file_h;


}

// reads in the (preallocated) blocks array, the name of all files in a directory
int SimpleFS_readDir(char** names, DirectoryHandle* d){
		if(names == NULL || d == NULL){
			printf("Bad parameters on input\n");
			return -1;
		}

		int total_blocks = 0;
		int ret = 0;
		FirstDirectoryBlock* fdb = d->dcb;
		DiskDriver* disk = d->sfs->disk;

		int max_free_space_fdb = (BLOCK_SIZE - sizeof(BlockHeader) - sizeof(FileControlBlock) - sizeof(int))/sizeof(int);
		int max_free_space_db = (BLOCK_SIZE - sizeof(BlockHeader))/sizeof(int);

		if(fdb->num_entries > 0){ //directory non vuota
			int i;
			FirstFileBlock to_check; //firstfileblock di appoggio per controllare in tutti i blocchi

			int* blocks = fdb->file_blocks;
			for(i = 0; i < max_free_space_fdb; i++){
					if(blocks[i]>0 && DiskDriver_readBlock(disk,&to_check,blocks[i]) != -1 ){
						names[total_blocks] = strndup(to_check.fcb.name,128);		//duplico la stringa con strndup e lo metto nell'array dei nomi
						total_blocks++;
					}
			}

			if(fdb->num_entries > i){
				int next = fdb->header.next_block;
				DirectoryBlock db;
					while (next != -1) {
						ret = DiskDriver_readBlock(disk,&db,next);
						if(ret == -1){
							printf("Cannot read the next block: SimpleFS_read\n");
							return -1;
						}
						int* blocks = db.file_blocks;
						for(i = 0; i< max_free_space_db; i++){
							if(blocks[i]>0 && DiskDriver_readBlock(disk,&to_check,blocks[i]) != -1){
								        names[total_blocks] = strndup(to_check.fcb.name, 128); // come sopra
                        total_blocks++;
							}
						}
						next = db.header.next_block; // va al prossimo directory block
					}
				}
			}
			return 0;
}

// opens a file in the  directory d. The file should be exisiting
FileHandle* SimpleFS_openFile(DirectoryHandle* d, const char* filename){
		if(d == NULL || filename == NULL){
			printf("Bad parameters on input\n");
			return NULL;
		}
		int ret = 0;
		FirstDirectoryBlock* fdb = d->dcb;
		DiskDriver* disk = d->sfs->disk;
		int max_free_space_fdb = (BLOCK_SIZE - sizeof(BlockHeader) - sizeof(FileControlBlock) - sizeof(int))/sizeof(int);
		int max_free_space_db = (BLOCK_SIZE - sizeof(BlockHeader))/sizeof(int);

		if(fdb->num_entries > 0){ //directory non vuota
			FileHandle* fh = malloc(sizeof(FileHandle));
			fh->sfs = d->sfs;
			fh->directory = fdb;
			fh->pos_in_file = 0;

			int found = 0; //file trovato = 1; file non trovato = 0;
			FirstFileBlock* check = malloc(sizeof(FirstFileBlock));
			int i,final;
			for(i = 0; i < max_free_space_fdb; i++){
				if(fdb->file_blocks[i] > 0 && (DiskDriver_readBlock(disk,&check,fdb->file_blocks[i]) != -1)){
					if(strncmp(check->fcb.name,filename,128) == 0){
						found = 1;
						final = i;
						DiskDriver_readBlock(disk,check,fdb->file_blocks[i]);
						fh->fcb = check;
						break;
					}
				}
			}
			//ora controllo l'esistenza del file nel directory block
			int next = fdb->header.next_block;
			DirectoryBlock db;
			while(next!= -1 && found != 1){
				ret = DiskDriver_readBlock(disk,&db,next);
				if(ret == -1){
						printf("Cannot read the next block: SimpleFS_openFile\n");
						return NULL;
				}
				for(i = 0; i < max_free_space_db; i++){
					if(db.file_blocks[i]>0 && (DiskDriver_readBlock(disk,check,db.file_blocks[i]) != -1 )){
						if(strncmp(check->fcb.name,filename,128) == 0){
							found = 1;
							final = i;
							DiskDriver_readBlock(disk,check,db.file_blocks[i]);
							fh->fcb = check;
							break;
					}
				}
			}
			next = db.header.next_block;
		}
		if(found){
			return fh;
		}else{
			printf("Cannot open File: Filename doesnt exists\n" );
			free(fh);
			return NULL;
		}
	}else{ //directory is empty
		printf("Cannot open File: Directory is empty\n");
		return NULL;
	}
	return NULL;
}


// writes in the file, at current position for size bytes stored in data
// overwriting and allocating new space if necessary
// returns the number of bytes written
int SimpleFS_write(FileHandle* f, void* data, int size){
	FirstFileBlock* ffb = f->fcb;
	//preparo allo scrittura del file
	int off = f->pos_in_file;
	int to_write = size;
	int bytes_written = 0;
	int max_free_space_ffb = BLOCK_SIZE - sizeof(FileControlBlock) - sizeof(BlockHeader);
  int max_free_space_fb = BLOCK_SIZE - sizeof(BlockHeader);

	if(off < max_free_space_ffb && to_write <=  max_free_space_ffb-off){  //se lo spazio necessario è minore delo spaazio disponibile
		memcpy(ffb->data + off, (char*)data, max_free_space_ffb-off);  //(dest,source(castato a char*),size necessario)
		bytes_written += to_write;

		if(f->pos_in_file+bytes_written > ffb->fcb.written_bytes)
			ffb->fcb.written_bytes = f->pos_in_file+bytes_written;
        // update the block
    DiskDriver_freeBlock(f->sfs->disk,ffb->fcb.block_in_disk);
    DiskDriver_writeBlock(f->sfs->disk, ffb, ffb->fcb.block_in_disk);
		return bytes_written;
		}
		else if(off < max_free_space_ffb && to_write >  max_free_space_ffb-off){ //se i byte da scrivere sono maggiorni dei byte disponibili
			memcpy(ffb->data+off, (char*)data, max_free_space_fb-off);
			bytes_written += max_free_space_ffb-off;
			to_write = size - bytes_written;
			DiskDriver_freeBlock(f->sfs->disk,ffb->fcb.block_in_disk);
		 	DiskDriver_writeBlock(f->sfs->disk, ffb, ffb->fcb.block_in_disk);
			off = 0;
		}else
			off -= max_free_space_ffb;

		int block_in_disk = ffb->fcb.block_in_disk;
		int next_block = ffb->header.next_block;
		int block_in_file = ffb->header.block_in_file;
		FileBlock tmp_fb;
		int one_block = 0;
		if(next_block == -1)
			one_block = 1;

			while (bytes_written < size) {
				if(next_block == -1){ //se non ci sono piu blocchi ne creiamo uno nuovo
					FileBlock n_fb = {0};
					n_fb.header.block_in_file = block_in_file + 1;
					n_fb.header.next_block = -1;
					n_fb.header.previous_block = block_in_disk;

					next_block = DiskDriver_getFreeBlock(f->sfs->disk,block_in_disk); //dopo come start il blocco presente del disco
					if(one_block == 1){
						ffb->header.next_block = next_block;
						//aggiorno le informazioni sul disco
						DiskDriver_freeBlock(f->sfs->disk,ffb->fcb.block_in_disk);
						DiskDriver_writeBlock(f->sfs->disk,ffb,ffb->fcb.block_in_disk);
						one_block = 0;
					}else{
						tmp_fb.header.next_block = next_block; //
							//aggiorno le informazioni su disco
							DiskDriver_freeBlock(f->sfs->disk,ffb->fcb.block_in_disk);
							DiskDriver_writeBlock(f->sfs->disk,&tmp_fb,ffb->fcb.block_in_disk);
					}
					DiskDriver_writeBlock(f->sfs->disk,&n_fb,next_block);

					tmp_fb = n_fb;
				}
				else
					if(DiskDriver_readBlock(f->sfs->disk,&tmp_fb,next_block) == -1) return -1;

					if(off < max_free_space_fb && to_write <= max_free_space_fb - off){
						memcpy(tmp_fb.data+off,(char*)data,max_free_space_fb-off);
						bytes_written += to_write;
						if(f->pos_in_file+bytes_written > ffb->fcb.written_bytes)
							ffb->fcb.written_bytes = f->pos_in_file+bytes_written;

						DiskDriver_freeBlock(f->sfs->disk,ffb->fcb.block_in_disk);
					  DiskDriver_writeBlock(f->sfs->disk, ffb, ffb->fcb.block_in_disk);
						DiskDriver_freeBlock(f->sfs->disk,ffb->fcb.block_in_disk);
						DiskDriver_writeBlock(f->sfs->disk,&tmp_fb,next_block);
						return bytes_written;
					}else if(off < max_free_space_fb && to_write >  max_free_space_fb-off){
							memcpy(tmp_fb.data+off,(char*)data+bytes_written,max_free_space_fb-off);
							bytes_written += max_free_space_fb - off;
							to_write = size - bytes_written;
							DiskDriver_freeBlock(f->sfs->disk,ffb->fcb.block_in_disk);
							DiskDriver_writeBlock(f->sfs->disk,&tmp_fb,next_block);
							off = 0;
					}else
						off -= max_free_space_fb;


						block_in_disk = next_block;	// update index of current_block
						next_block = tmp_fb.header.next_block;
						block_in_file = tmp_fb.header.block_in_file; // update index of next_block
				}
				return bytes_written;
			}
