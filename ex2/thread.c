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


void *fctThreadSlaves(void *p){
   sigset_t mask;
   sigfillset(&mask);
   sigdelset(&mask, SIGUSR1);
   sigprocmask(SIG_SETMASK, &mask, NULL);
   pause();

   return NULL;
}

void fctFinMaster(void *p){
   printf("Fin du Master : %d\n", pthread_self());
}

void *fctThreadMaster(void *p){
   pthread_cleanup_push(fctFinMaster, NULL);
   pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL);

   sigset_t mask;
   sigemptyset(&mask);
   sigaddset(&mask, SIGUSR1);
   sigprocmask(SIG_SETMASK, &mask, NULL);
   printf("thread Master : %d\n", pthread_self());

   for(;;)
      pause();

   pthread_cleanup_pop(0);
   pthread_exit(NULL);

   return NULL;
}


pthread_t tid1, tid2, tid3, tid4;

void HandlerSIGINT(int sig){
   printf("thread %d a recu le signal SIGINT\n", pthread_self());
   kill(getpid(), SIGUSR1);
}

void HandlerSIGUSR1(int sig){
   printf("thread %d a recu le signal SIGUSR1\n", pthread_self());
   pthread_exit(NULL);
}


int main(){
   printf("Thread principale demarre : %d\n", pthread_self());
   struct sigaction A;
   A.sa_handler = HandlerSIGINT;
   sigemptyset(&A.sa_mask);
   A.sa_flags = 0;

   if(sigaction(SIGINT, &A, NULL) == -1){
      perror("Erreur de sigaction");
      exit(EXIT_FAILURE);
   }

   struct sigaction sigB;
   sigB.sa_handler = HandlerSIGUSR1;
   sigemptyset(&sigB.sa_mask);
   sigB.sa_flags = 0;

   if(sigaction(SIGUSR1, &sigB, NULL) == -1){
      perror("Erreur de sigaction");
      exit(EXIT_FAILURE);
   }
   
   //Création des threads
   pthread_create(&tid1, NULL, fctThreadMaster, NULL);
   puts("Threads 1 lance !");

   pthread_create(&tid2, NULL, fctThreadSlaves, NULL);    
   puts("Threads 2 lance !");
    
   pthread_create(&tid3, NULL, fctThreadSlaves, NULL);
   puts("Threads 3 lance !");

   pthread_create(&tid4, NULL, fctThreadSlaves, NULL);
   puts("Threads 4 lance !");

   sigset_t mask;
   sigfillset(&mask);
   sigdelset(&mask, SIGQUIT);
   sigprocmask(SIG_SETMASK, &mask, NULL);

   pthread_join(tid2, NULL);
   pthread_join(tid3, NULL);
   pthread_join(tid4, NULL);

   printf("cancel du master\n");
   pthread_cancel(tid1);
   pthread_join(tid1, NULL);


   //fin prog
   puts("Fin du thread principal");
   exit(EXIT_SUCCESS);
}

