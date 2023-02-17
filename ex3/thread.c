#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <time.h>

typedef struct{
   char nom[20];
   int nbSecondes;
}DONNEE;

//Variables globales
DONNEE data[] = { "MATAGNE",15,
                  "WILVERS",10,
                  "WAGNER",17,
                  "QUETTIER",8,
                  "",0 };

DONNEE Param;                  
pthread_t tid[4];
pthread_mutex_t mutex_donnee, mutex_compteur;
pthread_cond_t condCompteur;
int compteur = 0;
pthread_key_t cle;

//fonction threads
void destructeur(void *p){
   free(p);
}

void fctFinThread(void *p){
   printf("Thread %d.%d se termine\n", getpid(), pthread_self());
   pthread_mutex_lock(&mutex_compteur);
   compteur--;
   pthread_mutex_unlock(&mutex_compteur);
   pthread_cond_signal(&condCompteur);
}

void *fctThread(void *p){

   struct timespec time, remaining;
   time.tv_nsec = 0;
   time.tv_sec = ((DONNEE*)p)->nbSecondes;
   char *nom = (char*)malloc(sizeof(char)*(strlen(((DONNEE*)p)->nom)+1));
   strcpy(nom, ((DONNEE*)p)->nom);
   pthread_setspecific(cle, (void*)nom);
   //Fin section critique
   pthread_mutex_unlock(&mutex_donnee);

   //set fonction de fin de thread
   pthread_cleanup_push(fctFinThread, NULL);
   pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL);

   
   //set mask empty
   sigset_t mask;
   sigemptyset(&mask);
   sigprocmask(SIG_SETMASK, &mask, NULL);
   printf("thread %d.%d lancé :->nom : %s\n", getpid(), pthread_self(), nom);
   
   //Attente
   while(nanosleep(&time, &remaining)!=0)
      time = remaining;

   //section fin de thread
   pthread_cleanup_pop(1);
   pthread_exit(NULL);

   return NULL;
}


//Gestionnaire de signaux
void HandlerSIGINT(int sig){
   printf("Thread %d.%d s'occupe de %s\n", getpid(), pthread_self(), (char*)pthread_getspecific(cle));
}

//Main
int main(){
   printf("Thread principale demarre : %d\n", pthread_self());

   //Initialisation du mutex
   if(pthread_mutex_init(&mutex_donnee, NULL))
      perror("Erreur d'initialisation mutex : ");
   
   if(pthread_mutex_init(&mutex_compteur, NULL))
      perror("Erreur d'initialisation mutex : ");

   if(pthread_cond_init(&condCompteur, NULL))
      perror("Erreur d'initialisation de cond : ");


   //Armement du signal SIGINT
   struct sigaction A;
   A.sa_handler = HandlerSIGINT;
   sigemptyset(&A.sa_mask);
   A.sa_flags = 0;

   if(sigaction(SIGINT, &A, NULL) == -1){
      perror("Erreur de sigaction");
      exit(EXIT_FAILURE);
   }

   //Masque par défaut pour tout le monde SIGINT
   sigset_t mask;
   sigemptyset(&mask);
   sigaddset(&mask, SIGINT);
   sigprocmask(SIG_SETMASK, &mask, NULL);

   //Création de la clé
   pthread_key_create(&cle, destructeur);

   //Création des threads
   for(int i =0; strcmp(data[i].nom, "")!=0;i++){
      pthread_mutex_lock(&mutex_donnee);
      memcpy(&Param, &data[i], sizeof(DONNEE));
      pthread_create(&tid[i], NULL, fctThread, &Param);
      pthread_mutex_lock(&mutex_compteur);
      compteur++;
      pthread_mutex_unlock(&mutex_compteur);
   }

   //Attente des threads
   pthread_mutex_lock(&mutex_compteur);
   while(compteur>0)
      pthread_cond_wait(&condCompteur, &mutex_compteur);
   pthread_mutex_unlock(&mutex_compteur);

   //fin prog
   pthread_mutex_destroy(&mutex_donnee);
   pthread_mutex_destroy(&mutex_compteur);
   pthread_cond_destroy(&condCompteur);

   printf("Fin du thread principal\n");
   exit(EXIT_SUCCESS);
}

