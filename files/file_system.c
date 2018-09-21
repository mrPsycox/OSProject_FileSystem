#include "simplefs.h"
#include "disk_driver.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "bitmap.h"

int main(int argc, char** argv){
    DiskDriver disk;
    const char* filename = "./disko.txt";
	

    printf("------------------- inizializzo DiskDriver -----------------\n");
    DiskDriver_init(&disk,filename,256);  //inizializzo disco
    //printf("\nDisk_header_firstfreeblock PRIMA: %d",disk.header->first_free_block);

    SimpleFS fs;

    printf("------------------- inizializzo SimpleFS -----------------\n");
    DirectoryHandle* dir_handle = SimpleFS_init(&fs,&disk);
    //printf("\nDisk_header_firstfreeblock DOPO: %d",disk.header->first_free_block);

    if(dir_handle == NULL){
      printf("------------------- formatto file system -----------------\n");
      DiskDriver_init(&disk,filename,256);
      SimpleFS_format(&fs);
      dir_handle = SimpleFS_init(&fs,&disk);

    }

    DirectoryHandle* root_dir_handle = dir_handle;  //creo la root directory con le strutture dati necessarie
    FileHandle* fh;

    //file necessari per le risposte utente
    int opzione;//opzione scelta dell'utente tramite scanf
    char testo[264]; //testo file
    char nomefile[128];
    int flag_file[128];
    char** files_directory = malloc(sizeof(char*)*128);
    int i = 0;
    int ret;
    for(i = 0; i < 128; i++){
      files_directory[i] = malloc(sizeof(char*)*128);
    }


    do{
      printf("\n@@@@@@@@@@@@@@@@@@ SELEZIONARE UN'OPERAZIONE @@@@@@@@@@@@@@@\n");
      printf("\nDirectory corrente: %s\n",dir_handle->dcb->fcb.name);
      printf("\n@@@@@\n1) Crea File\n 2) Leggi File\n 3) Scrivi File\n 4) Crea Directory\n 5)Cancella file e/o directory\n 6) Stampa directory (ls)\n 7) Cambia Directory @@@@@@\n"  );
      printf("\nInserisci un'opzione: \n ");
      scanf("%d",&opzione);

      switch (opzione) {
				  case 1://creo file
					  printf("\n\nInserire nome del file:    \n");
					  scanf("%s",nomefile);
					  fh = SimpleFS_createFile(dir_handle,nomefile);
					  if(fh == NULL){
						printf("@@ Impossible to create file @@\n" );
						break;
					  }
					  printf("-------- FILE CREATO --------\n");
					  free(fh);
					  break;

				  case 2:// Leggi FILE
					  printf("\n\nLista dei file disponibili in lettura: \n");
					  SimpleFS_readDir(files_directory,flag_file,dir_handle);
					  for(i = 0; i < dir_handle->dcb->num_entries; i++){
						  if(flag_file[i] == 0){
							printf("FILE:\n  %s\n",files_directory[i]);
						  }else{ //caso trovo una directory
							  printf("DIRECTORY:\n NOME:  %s\n",files_directory[i]);
						 }
					  }
					  printf("\n\nInserire il nome del file\n");
					  scanf("%s",nomefile);
					  //printf("\n\nsono qui prima della openfile\n");

					  fh = SimpleFS_openFile(dir_handle,nomefile);
					  //printf("superata la open file!");
					  if(fh == NULL){
						printf("\n\nImpossibile aprire il file!\n@@@@@@@@@@@@@\n");
						break;
					  }

					   testo[512];
					   ret = SimpleFS_read(fh,testo,fh->fcb->fcb.written_bytes);
					  if(ret == -1){
						printf("[!] IMPOSSIBILE LEGGERE FILE\n@@@@@@@@@\n");
						break;
					  }
					  printf("\n\nContenuto del file:\n %s \n",testo );
					  free(fh);


					  break;

      case 3:
          printf("\n\nLista dei file disponibili in lettura: \n");
          SimpleFS_readDir(files_directory,flag_file,dir_handle);
          for(i = 0; i < dir_handle->dcb->num_entries; i++){
              if(flag_file[i] == 0){
                printf("FILE:\n   %s",files_directory[i]);
              }
          }
          printf("\n\nInserire il nome del file\n");
          scanf("%s",nomefile);

          fh = SimpleFS_openFile(dir_handle,nomefile);
          if(fh == NULL){
            printf("\n\nImpossibile aprire il file!\n@@@@@@@@@@@@@\n");
            break;
          }
          printf("\n\napertura file con successo\n");
           testo[512];
          printf("\n\nInserire testo da inserire nel file: \n");
          scanf("%s",testo);

           ret = SimpleFS_write(fh,testo,strlen(testo));
          if(ret == -1){
            printf("\n\n [!] Impossibile to write file!\n");
            break;
          }
          printf("\n\n!!!! SCRITTURA FILE ESEGUITA CON SUCCESSO!\n");
          free(fh);
          break;

      case 4:

        printf("\n\nInserire nome directory: \n");
        scanf("%s",nomefile);
         ret = SimpleFS_mkDir(dir_handle,nomefile);
        if(ret == -1){ //caso negativo
          printf("\n\n[!] Impossibile creare directory\n");
          break;
        }else if (ret == 0) { //caso positivo
          printf("\n\n!!!! DIRECTORY CREATA CON SUCCESSO!\n");
        }
        break;

      case 5:
        printf("\n\nLista dei file disponibili in lettura: \n");
        SimpleFS_readDir(files_directory,flag_file,dir_handle);
        for(i = 0; i < dir_handle->dcb->num_entries; i++){
            if(flag_file[i] == 0){
              printf("FILE:\n NOME:  %s\n",files_directory[i]);
            }
            else{
              printf("DIRECTORY:\n NOME:  %s\n",files_directory[i]);
            }
          }
          printf("\n\nInserire nomefile\n");
          scanf("%s",nomefile);
           ret = SimpleFS_remove(dir_handle,nomefile);
          if(ret == -1){
            printf("\n\n[!] IMPOSSIBILE RIMUOVERE FILE O DIRECTORY\n");
          }else if(ret == 0){
            printf("\n\n!!!!!! RIMOZIONE EFFETTUATA CON SUCCESSO!\n");
          }
          break;

      case 6:
        printf("\n\nLista dei file disponibili in lettura: \n");
        SimpleFS_readDir(files_directory,flag_file,dir_handle);
        for(i = 0; i < dir_handle->dcb->num_entries; i++){
            if(flag_file[i] == 0){
              printf("FILE:\n NOME:  %s\n",files_directory[i]);
            }
            else if(flag_file[i] ==  1){
              printf("DIRECTORY:\n NOME:  %s\n",files_directory[i]);
            }
          }
          break;

      case 7:
          printf("\n\nLista dei file disponibili in lettura: \n");
          SimpleFS_readDir(files_directory,flag_file,dir_handle);
          for(i = 0; i < dir_handle->dcb->num_entries; i++){
              if(flag_file[i] == 1){
                printf("DIRECTORY:\n NOME:  %s",files_directory[i]);
              }


            }
          printf("\n\nInserire directory (.. per tornare indietro alla directory precedente)\n");
          scanf("%s",nomefile);
         ret = SimpleFS_changeDir(dir_handle,nomefile);
          if(ret == -1){
            printf("\n\nImpossibile cambiare cartella\n");
          }else if(ret == 0){
          printf("\n\n!!!!!! CAMBIO DIRECTORY CON SUCCESSO EFFETTUATA CON SUCCESSO!\n");
          }
		  break;
      }
    }
    while(opzione != 0);
  }
