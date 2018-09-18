#include "simplefs.h"
#include "disk_driver.h"
#include <stdio.h>

int main(int argc, char** argv){
    DiskDriver disk;
    const char* filename = "./disk.txt";

    DiskDriver_init(&disk,filename,512);  //inizializzo disco
    printf("------------------- inizializzo DiskDriver -----------------\n");

    SimpleFS fs;

    printf("------------------- inizializzo SimpleFS -----------------\n");
    DirectoryHandle* dir_handle = SimpleFS_init(&fs,&disk);
    if(dir_handle == NULL){
      printf("------------------- formatto file system -----------------\n");
      DiskDriver_init(&disk,filename,512);
      SimpleFS_format(&fs);
      dir_handle = SimpleFS_init(&fs,&disk);
    }

    DirectoryHandle* root_dir_handle = dir_handle;  //creo la root directory con le strutture dati necessarie
    FileHandle* fh;

    //file necessari per le risposte utente
    int opzione;//opzione scelta dell'utente tramite scanf
    char testo[264]; //testo file
    char nomefile[64];
    int flag_file[128];
    char** files_directory = malloc(sizeof(char*)*128);
    int i = 0;
    for(i = 0; i < 128; i++){
      files_directory[i] = malloc(sizeof(char*)*128);
    }


    do{
      printf("@@@@@@@@@@@@@@@@@@ SELEZIONARE UN'OPERAZIONE @@@@@@@@@@@@@@@\n");
      printf("\nDirectory corrente: %s\n",dir_handle->dcb->fcb.name);
      printf("@@@@@\n1) Crea File\n 2) Leggi File\n 3) Scrivi File\n 4) Crea Directory\n 5)Cancella file e/o directory\n 6) Stampa directory (ls)\n 7) Cambia Directory @@@@@@\n"  );
      scanf("%d\n",&opzione);

      switch (risposta) {
        case 1://creo file
          printf("\n\nInserire nome del file:    \n");
          scanf("%s\n",nomefile);
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
          SimpleFS_readDir(files_directory,dir_handle,flag_file);
          for(i = 0; i < dir_handle->dcb->num_entries; i++){
              if(flag_file[i] == 0){
                printf("FILE:\n NOME:  %s",files_directory[i]);
              }
          }
          printf("\n\nInserire il nome del file\n");
          scanf("%s\n",nomefile);

          fh = SimpleFS_openFile(dir_handle,nomefile);
          if(fh == NULL){
            printf("\n\nImpossibile aprire il file!\n@@@@@@@@@@@@@\n");
            break;
          }
          char testo[512];
          int ret = SimpleFS_read(fh,testo,fh->fcb->fcb.written_bytes);
          if(ret == -1){
            printf("[!] IMPOSSIBILE LEGGERE FILE\n@@@@@@@@@\n");
          }
          printf("\n\nContenuto del file:\n %s \n",testo );
          free(fh);
          break;
      }
    }
}
