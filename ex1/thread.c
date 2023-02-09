#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>
#include <unistd.h>

typedef struct {
   char nom_fichier[80];
   char mot_recherche[20];
   int nb_tab;
}struct_threads;

void *fctThread(void *param);

pthread_t tid1, tid2, tid3, tid4;


int main(){
   puts("Thread principal demarre");
   int ret;
   int* retThread;
   struct_threads struct_thread1, struct_thread2, struct_thread3, struct_thread4;
   strcpy(struct_thread1.nom_fichier, "Serveur.cpp");
   strcpy(struct_thread1.mot_recherche, "open");
   struct_thread1.nb_tab = 0;

   //Création des threads
   pthread_create(&tid1, NULL, fctThread, (void *) &struct_thread1);
   puts("Threads 1 lance !");

   strcpy(struct_thread2.nom_fichier, "Serveur.cpp");
   strcpy(struct_thread2.mot_recherche, "return");
   struct_thread2.nb_tab = 1;
   pthread_create(&tid2, NULL, fctThread, (void *) &struct_thread2);
   puts("Threads 2 lance !");

   strcpy(struct_thread3.nom_fichier, "Serveur.cpp");
   strcpy(struct_thread3.mot_recherche, "close");
   struct_thread3.nb_tab = 2;
   pthread_create(&tid3, NULL, fctThread, (void *) &struct_thread3);
   puts("Threads 3 lance !");

   strcpy(struct_thread4.nom_fichier, "Serveur.cpp");
   strcpy(struct_thread4.mot_recherche, "printf");
   struct_thread4.nb_tab = 3;
   pthread_create(&tid4, NULL, fctThread, (void *) &struct_thread4);
   puts("Threads 4 lance !");

   //Attente des threads
   puts("Attente de la fin du thread 1");
   pthread_join(tid1,(void **) &retThread);
   printf("Valeur renvoyee par le thread 1 = %d\n", *retThread);
   free(retThread);
   
   puts("Attente de la fin du thread 2");
   pthread_join(tid2,(void **) &retThread);
   printf("Valeur renvoyee par le thread 2 = %d\n", *retThread);
   free(retThread);
   puts("Attente de la fin du thread 3");
   pthread_join(tid3,(void **) &retThread);
   printf("Valeur renvoyee par le thread 3 = %d\n", *retThread);
   free(retThread);

   puts("Attente de la fin du thread 4");
   pthread_join(tid4,(void **) &retThread);
   printf("Valeur renvoyee par le thread 4 = %d\n", *retThread);
   free(retThread);

   //fin prog
   puts("Fin du thread principal");
   return 0;
}

void print_tab(int nb){
   for(int i =0; i<nb; i++)
      printf("\t");
}

void *fctThread(void *param){
   struct_threads *struct_param = (struct_threads *) param;
   int *nbr_occurence = (int*)malloc(sizeof(int*));
   *nbr_occurence = 0;
   int nombre = 0;
   printf("nbr occurence init : %d\n", *nbr_occurence);
   int fd;
   int taille_mot = strlen(struct_param->mot_recherche);
   int ret_read = taille_mot;

   printf("taille mot : %d\n", taille_mot);
   printf("nom fichier : %s\n", struct_param->nom_fichier);
   char mot[20];
   for(int i = 0;;i++){
      print_tab(struct_param->nb_tab);
      printf("*\n");

      if((fd = open(struct_param->nom_fichier, O_RDONLY)) == -1){
         perror("Erreur de open");
         return NULL;
      }
      
      if(lseek(fd, i, SEEK_SET) == -1){
         perror("erreur de lseek");
         close(fd);
         return NULL;
      }
      ret_read = read(fd, mot, taille_mot);
      if(ret_read<taille_mot){
         close(fd);
         break;
      }
      if(strcmp(mot, struct_param->mot_recherche) == 0)
         nombre+=1;

      if(close(fd) == -1){
         perror("Erreur de close");
         return NULL;
      }
   }
   *nbr_occurence = nombre;
   pthread_exit((void *) nbr_occurence);
   return 0;
}