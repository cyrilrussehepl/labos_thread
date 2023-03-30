#include <unistd.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <time.h>
#include <SDL/SDL.h>
#include "./presentation/presentation.h"

#define VIDE 0
#define DKJR 1
#define CROCO 2
#define CORBEAU 3
#define CLE 4

#define AUCUN_EVENEMENT 0

#define LIBRE_BAS 1
#define LIANE_BAS 2
#define DOUBLE_LIANE_BAS 3
#define LIBRE_HAUT 4
#define LIANE_HAUT 5

void *FctThreadEvenements(void *);
void *FctThreadCle(void *);
void *FctThreadDK(void *);
void *FctThreadDKJr(void *);
void *FctThreadScore(void *);
void *FctThreadEnnemis(void *);
void *FctThreadCorbeau(void *);
void *FctThreadCroco(void *);

void initGrilleJeu();
void setGrilleJeu(int l, int c, int type = VIDE, pthread_t tid = 0);
void afficherGrilleJeu();

void HandlerSIGUSR1(int);
void HandlerSIGUSR2(int);
void HandlerSIGALRM(int);
void HandlerSIGINT(int);
void HandlerSIGQUIT(int);
void HandlerSIGCHLD(int);
void HandlerSIGHUP(int);

void DestructeurVS(void *p);

pthread_t threadCle;
pthread_t threadDK;
pthread_t threadDKJr;
pthread_t threadEvenements;
pthread_t threadScore;
pthread_t threadEnnemis;

pthread_cond_t condDK;
pthread_cond_t condScore;

pthread_mutex_t mutexGrilleJeu;
pthread_mutex_t mutexDK;
pthread_mutex_t mutexEvenement;
pthread_mutex_t mutexScore;

pthread_key_t keySpec;

bool MAJDK = false;
int score = 0;
bool MAJScore = true;
int delaiEnnemis = 4000;
int positionDKJr = 1;
int evenement = AUCUN_EVENEMENT;
int etatDKJr;

typedef struct
{
	int type;
	pthread_t tid;
} S_CASE;

S_CASE grilleJeu[4][8];

typedef struct
{
	bool haut;
	int position;
} S_CROCO;

// ------------------------------------------------------------------------

int main(int argc, char *argv[])
{
	int evt, nbViePerdues = 0;

	//Set masque par défaut pour tout le monde
	sigset_t mask;
	sigfillset(&mask);
	sigprocmask(SIG_SETMASK, &mask, NULL);

	//Armement des signaux
	struct sigaction A;
	A.sa_handler = HandlerSIGQUIT;
	sigemptyset(&A.sa_mask);
	A.sa_flags = 0;
	if (sigaction(SIGQUIT, &A, NULL) == -1)
	{
		perror("Erreur de sigaction");
		exit(EXIT_FAILURE);
	}

	struct sigaction B;
	B.sa_handler = HandlerSIGALRM;
	sigemptyset(&B.sa_mask);
	B.sa_flags = 0;
	if (sigaction(SIGALRM, &B, NULL) == -1)
	{
		perror("Erreur de sigaction");
		exit(EXIT_FAILURE);
	}

	struct sigaction C;
	C.sa_handler = HandlerSIGINT;
	sigemptyset(&C.sa_mask);
	C.sa_flags = 0;
	if (sigaction(SIGINT, &C, NULL) == -1)
	{
		perror("Erreur de sigaction");
		exit(EXIT_FAILURE);
	}

	struct sigaction D;
	D.sa_handler = HandlerSIGUSR1;
	sigemptyset(&D.sa_mask);
	D.sa_flags = 0;
	if (sigaction(SIGUSR1, &D, NULL) == -1)
	{
		perror("Erreur de sigaction");
		exit(EXIT_FAILURE);
	}

	ouvrirFenetreGraphique();

	// Initialisation des mutex et des variables de condition
	if (pthread_mutex_init(&mutexGrilleJeu, NULL))
		perror("Erreur d'initialisation mutex grille jeu");

	if (pthread_mutex_init(&mutexEvenement, NULL))
		perror("Erreur d'initialisation mutex evenement");

	if (pthread_mutex_init(&mutexDK, NULL))
		perror("Erreur d'initialisation mutex DK");

	if (pthread_mutex_init(&mutexScore, NULL))
		perror("Erreur d'initilisation mutex score");

	if (pthread_cond_init(&condDK, NULL))
		perror("Erreur d'initialisation de la variable de condition condDK");

	if (pthread_cond_init(&condScore, NULL))
		perror("Erreur d'initialisation de la variable de condition condScore");

	if (pthread_key_create(&keySpec, DestructeurVS))
		perror("Erreur lors de la création de la clé spécifique");

	// Initialisation de la grille
	initGrilleJeu();

	// Création des threads
	if (pthread_create(&threadCle, NULL, FctThreadCle, NULL))
		perror("Erreur de création du thread threadCle");

	if (pthread_create(&threadEvenements, NULL, FctThreadEvenements, NULL))
		perror("Erreur de création du thread threadEvenements");

	if (pthread_create(&threadDK, NULL, FctThreadDK, NULL))
		perror("Erreur de création du thread threadDK");

	if (pthread_create(&threadScore, NULL, FctThreadScore, NULL))
		perror("Erreur de création du thread threadScore");

	if (pthread_create(&threadEnnemis, NULL, FctThreadEnnemis, NULL))
		perror("Erreur de création du thread threadEnnemis");

	while (nbViePerdues < 3)
	{
		if (pthread_create(&threadDKJr, NULL, FctThreadDKJr, NULL))
			perror("Erreur de création du thread threadDKJr");

		pthread_join(threadDKJr, NULL);
		nbViePerdues++;
		afficherEchec(nbViePerdues);
	}
	pause();
}

// -------------------------------------

void initGrilleJeu()
{
	int i, j;

	pthread_mutex_lock(&mutexGrilleJeu);

	for (i = 0; i < 4; i++)
		for (j = 0; j < 7; j++)
			setGrilleJeu(i, j);

	pthread_mutex_unlock(&mutexGrilleJeu);
}

// -------------------------------------

void setGrilleJeu(int l, int c, int type, pthread_t tid)
{
	grilleJeu[l][c].type = type;
	grilleJeu[l][c].tid = tid;
}

// -------------------------------------

void afficherGrilleJeu()
{
	for (int j = 0; j < 4; j++)
	{
		for (int k = 0; k < 8; k++)
			printf("%d  ", grilleJeu[j][k].type);
		printf("\n");
	}

	printf("\n");
}

// -------------------------------------
void *FctThreadEvenements(void *)
{
	int evt;
	struct timespec time;
	time.tv_nsec = 100000000;
	time.tv_sec = 0;

	while (1)
	{
		evt = lireEvenement();

		if (evt == SDL_QUIT)
			exit(0);
		else if (evt == SDLK_UP || evt == SDLK_DOWN || evt == SDLK_RIGHT || evt == SDLK_LEFT)
		{
			pthread_mutex_lock(&mutexEvenement);
			evenement = evt;
			pthread_mutex_unlock(&mutexEvenement);
			kill(getpid(), SIGQUIT);
		}

		nanosleep(&time, NULL);

		pthread_mutex_lock(&mutexEvenement);
		evenement = AUCUN_EVENEMENT;
		pthread_mutex_unlock(&mutexEvenement);
	}

	pthread_exit(NULL);

	return NULL;
}

void *FctThreadCle(void *)
{
	int pos = 1;
	int direction = 1;
	struct timespec time;
	time.tv_nsec = 700000000;
	time.tv_sec = 0;

	for (;;)
	{
		switch (pos)
		{
		case 1:
			effacerCarres(3, 12, 2, 1);
			break;
		case 2:
			effacerCarres(3, 13, 2, 1);
			break;
		case 3:
			effacerCarres(3, 13, 2, 2);
			break;
		case 4:
			effacerCarres(3, 13, 2, 2);
			break;
		}

		pos += direction;
		if (pos == 4)
			direction = -1;
		else if (pos == 1)
		{
			direction = 1;
			pthread_mutex_lock(&mutexGrilleJeu);
			setGrilleJeu(0, 1, CLE, pthread_self());
			pthread_mutex_unlock(&mutexGrilleJeu);
		}
		else if (pos == 2 && direction == 1)
		{
			pthread_mutex_lock(&mutexGrilleJeu);
			setGrilleJeu(0, 1);
			pthread_mutex_unlock(&mutexGrilleJeu);
		}
		afficherCle(pos);
		nanosleep(&time, NULL);
	}

	pthread_exit(NULL);

	return NULL;
}

void *FctThreadDK(void *)
{
	int nbrRestant = 0;
	struct timespec time;
	time.tv_nsec = 700000000;
	time.tv_sec = 0;

	pthread_mutex_lock(&mutexDK);

	while (true)
	{
		if (nbrRestant == 0)
		{
			for (int i = 1; i < 5; i++)
				afficherCage(i);
			nbrRestant = 4;
		}
		MAJDK = false;
		pthread_cond_wait(&condDK, &mutexDK);
		if(MAJDK){
			switch (nbrRestant)
			{
			case 4:
				effacerCarres(2, 9, 2, 2);
				break;
			case 3:
				effacerCarres(2, 7, 2, 2);
				break;
			case 2:
				effacerCarres(4, 9, 2, 3);
				break;
			case 1:
				// efface dernière cage, affiche rire pendant 0.7 sec puis l'efface
				effacerCarres(4, 7, 2, 2);
				afficherRireDK();
				nanosleep(&time, NULL);
				effacerCarres(3, 8, 2, 2);
				pthread_mutex_lock(&mutexScore);
				score += 10;
				pthread_mutex_unlock(&mutexScore);
				pthread_cond_signal(&condScore);
				break;
			}
			nbrRestant--;
		}
	}

	pthread_mutex_unlock(&mutexDK);

	pthread_exit(NULL);
	return NULL;
}

void *FctThreadDKJr(void *)
{
	bool on = true;
	struct timespec time;
	time.tv_nsec = 400000000;
	time.tv_sec = 1;

	struct timespec time_end_jmp;
	time_end_jmp.tv_nsec = 500000000;
	time_end_jmp.tv_sec = 0;

	//Masque tous les signaux sauf SIGQUIT et SIGINT
	sigset_t mask;
	sigfillset(&mask);
	sigdelset(&mask, SIGQUIT);
	sigdelset(&mask, SIGINT);
	sigprocmask(SIG_SETMASK, &mask, NULL);

	pthread_mutex_lock(&mutexGrilleJeu);
	setGrilleJeu(3, 1, DKJR, getpid());
	pthread_mutex_unlock(&mutexGrilleJeu);
	afficherDKJr(11, 9, 1);
	etatDKJr = LIBRE_BAS;
	positionDKJr = 1;

	while (on)
	{
		pause();
		pthread_mutex_lock(&mutexEvenement);
		pthread_mutex_lock(&mutexGrilleJeu);
		switch (etatDKJr)
		{
		case LIBRE_BAS:
			switch (evenement)
			{
			case SDLK_LEFT:
				if (positionDKJr > 1)
				{
					setGrilleJeu(3, positionDKJr);
					effacerCarres(11, (positionDKJr * 2) + 7, 2, 2);
					positionDKJr--;
					setGrilleJeu(3, positionDKJr, DKJR, getpid());
					afficherDKJr(11, (positionDKJr * 2) + 7, ((positionDKJr - 1) % 4) + 1);
				}
				break;
			case SDLK_RIGHT:
				if (positionDKJr < 7)
				{
					setGrilleJeu(3, positionDKJr);
					effacerCarres(11, (positionDKJr * 2) + 7, 2, 2);
					positionDKJr++;
					setGrilleJeu(3, positionDKJr, DKJR, getpid());
					afficherDKJr(11, (positionDKJr * 2) + 7, ((positionDKJr - 1) % 4) + 1);
				}
				break;
			case SDLK_UP:
				setGrilleJeu(3, positionDKJr);
				effacerCarres(11, (positionDKJr * 2) + 7, 2, 2);

				//Check collision corbeau
				if(grilleJeu[2][positionDKJr].type == CORBEAU){
					pthread_mutex_unlock(&mutexGrilleJeu);
					kill(getpid(), SIGUSR1);
					pthread_mutex_unlock(&mutexEvenement);
					pthread_exit(NULL);
					return NULL;
				}

				setGrilleJeu(2, positionDKJr, DKJR, getpid());
				if (positionDKJr == 7)
				{
					etatDKJr = DOUBLE_LIANE_BAS;
					afficherDKJr(10, (positionDKJr * 2) + 7, 5);
				}
				else if (positionDKJr == 1 || positionDKJr == 5)
				{
					etatDKJr = LIANE_BAS;
					afficherDKJr(10, (positionDKJr * 2) + 7, 7);
				}
				else
				{
					afficherDKJr(10, (positionDKJr * 2) + 7, 8);

					pthread_mutex_unlock(&mutexGrilleJeu);
					nanosleep(&time, NULL);
					pthread_mutex_lock(&mutexGrilleJeu);

					setGrilleJeu(2, positionDKJr);
					effacerCarres(10, (positionDKJr * 2) + 7, 2, 2);
					setGrilleJeu(3, positionDKJr, DKJR, getpid());
					afficherDKJr(11, (positionDKJr * 2) + 7, ((positionDKJr - 1) % 4) + 1);
				}
				break;
			}
			break;
		case LIANE_BAS:
			if (evenement == SDLK_DOWN)
			{
				etatDKJr = LIBRE_BAS;
				setGrilleJeu(2, positionDKJr);
				effacerCarres(10, (positionDKJr * 2) + 7, 2, 2);
				setGrilleJeu(3, positionDKJr, DKJR, getpid());
				afficherDKJr(11, (positionDKJr * 2) + 7, ((positionDKJr - 1) % 4) + 1);
			}
			break;
		case DOUBLE_LIANE_BAS:
			switch (evenement)
			{
			case SDLK_UP:
				etatDKJr = LIBRE_HAUT;
				setGrilleJeu(2, positionDKJr);
				effacerCarres(10, (positionDKJr * 2) + 7, 2, 2);
				setGrilleJeu(1, positionDKJr, DKJR, getpid());
				afficherDKJr(7, (positionDKJr * 2) + 7, 6);
				break;
			case SDLK_DOWN:
				etatDKJr = LIBRE_BAS;
				setGrilleJeu(2, positionDKJr);
				effacerCarres(10, (positionDKJr * 2) + 7, 2, 2);
				setGrilleJeu(3, positionDKJr, DKJR, getpid());
				afficherDKJr(11, (positionDKJr * 2) + 7, 3);
				break;
			}
			break;
		case LIBRE_HAUT:
			switch (evenement)
			{
			case SDLK_UP:
				if (positionDKJr == 6)
				{
					etatDKJr = LIANE_HAUT;
					setGrilleJeu(1, positionDKJr);
					effacerCarres(7, (positionDKJr * 2) + 7, 2, 2);
					setGrilleJeu(0, positionDKJr, DKJR, getpid());
					afficherDKJr(6, (positionDKJr * 2) + 7, 7);
				}
				else if (positionDKJr == 3 || positionDKJr == 4)
				{
					setGrilleJeu(1, positionDKJr);
					effacerCarres(7, (positionDKJr * 2) + 7, 2, 2);
					setGrilleJeu(0, positionDKJr, DKJR, getpid());
					afficherDKJr(6, (positionDKJr * 2) + 7, 8);

					pthread_mutex_unlock(&mutexGrilleJeu);
					nanosleep(&time, NULL);
					pthread_mutex_lock(&mutexGrilleJeu);

					setGrilleJeu(0, positionDKJr);
					effacerCarres(6, (positionDKJr * 2) + 7, 2, 2);
					setGrilleJeu(1, positionDKJr, DKJR, getpid());
					afficherDKJr(7, (positionDKJr * 2) + 7, 7 - positionDKJr);
				}
				break;
			case SDLK_RIGHT:
				if (positionDKJr < 7)
				{
					setGrilleJeu(1, positionDKJr);
					effacerCarres(7, (positionDKJr * 2) + 7, 2, 2);
					positionDKJr++;
					setGrilleJeu(1, positionDKJr, DKJR, getpid());
					if (positionDKJr == 7)
						afficherDKJr(7, (positionDKJr * 2) + 7, 6);
					else
						afficherDKJr(7, (positionDKJr * 2) + 7, 7 - positionDKJr);
				}
				break;
			case SDLK_LEFT:
				if (positionDKJr > 3)
				{
					setGrilleJeu(1, positionDKJr);
					effacerCarres(7, (positionDKJr * 2) + 7, 2, 2);
					positionDKJr--;
					setGrilleJeu(1, positionDKJr, DKJR, getpid());
					afficherDKJr(7, (positionDKJr * 2) + 7, 7 - positionDKJr);
				}
				else if (positionDKJr == 3)
				{
					// saute pour clé
					setGrilleJeu(1, positionDKJr);
					effacerCarres(7, (positionDKJr * 2) + 7, 2, 2);

					positionDKJr--;
					afficherDKJr(5, 11, 9);
					nanosleep(&time_end_jmp, NULL);

					if (grilleJeu[0][1].type == CLE)
					{
						//Réveil threadDK
						pthread_mutex_lock(&mutexDK);
						MAJDK = true;
						pthread_mutex_unlock(&mutexDK);
						pthread_cond_signal(&condDK);
						//Réveil threadScore
						pthread_mutex_lock(&mutexScore);
						MAJScore = true;
						score += 10;
						pthread_mutex_unlock(&mutexScore);
						pthread_cond_signal(&condScore);


						effacerCarres(5, 12, 3, 2);

						//afficher grab cle

						//Réinitialisation
						setGrilleJeu(3, 1, DKJR, getpid());
						afficherDKJr(11, 9, 1);
						etatDKJr = LIBRE_BAS;
						positionDKJr = 1;
					}
					else
					{
						effacerCarres(5, 12, 3, 2);
						afficherDKJr(6, 11, 12);
						nanosleep(&time_end_jmp, NULL);

						effacerCarres(6, 11, 2, 2);
						afficherDKJr(11, 7, 13);
						nanosleep(&time_end_jmp, NULL);

						effacerCarres(11, 7, 2, 2);
						pthread_mutex_unlock(&mutexEvenement);
						pthread_mutex_unlock(&mutexGrilleJeu);
						pthread_exit(NULL);
					}
					break;
				case SDLK_DOWN:
					if (positionDKJr == 7)
					{
						etatDKJr = DOUBLE_LIANE_BAS;
						setGrilleJeu(1, positionDKJr);
						effacerCarres(7, (positionDKJr * 2) + 7, 2, 2);
						setGrilleJeu(2, positionDKJr, DKJR, getpid());
						afficherDKJr(10, (positionDKJr * 2) + 7, 5);
					}
				}
				break;
			case LIANE_HAUT:
				if (evenement == SDLK_DOWN)
				{
					etatDKJr = LIBRE_HAUT;
					setGrilleJeu(0, positionDKJr);
					effacerCarres(6, (positionDKJr * 2) + 7, 2, 2);
					setGrilleJeu(1, positionDKJr, DKJR, getpid());
					afficherDKJr(7, (positionDKJr * 2) + 7, 7 - positionDKJr);
				}
				break;
			}
		}
		pthread_mutex_unlock(&mutexGrilleJeu);
		pthread_mutex_unlock(&mutexEvenement);
	}

	pthread_exit(0);
	return NULL;
}

void *FctThreadScore(void *){
	int chiffre = -1, tempScore;
	
	pthread_mutex_lock(&mutexScore);
	while (true)
	{
		if(MAJScore){
			MAJScore = false;
			tempScore = score;
			effacerCarres(3, 26, 1, 4);
			for(int i = 0; i<4;i++){
				chiffre = tempScore%10;
				afficherChiffre(3, 29-i, chiffre);
				tempScore /= 10;
			}
		}
		pthread_cond_wait(&condScore, &mutexScore);
	}	
	pthread_mutex_unlock(&mutexScore);

	pthread_exit(NULL);
	return NULL;
}

void *FctThreadEnnemis(void *){
	sigset_t mask;
	sigfillset(&mask);
	sigdelset(&mask, SIGALRM);
	sigprocmask(SIG_SETMASK, &mask, NULL);

	srand(time(NULL));

	pthread_t threadCorbeaux[3];
	int index_corbeau = 0;
	int random_ennemy = 1;

	alarm(15);
	while(true){
		usleep(delaiEnnemis*1000);
		// random_ennemy = rand()%2;
		if(random_ennemy){
			if(pthread_create(&threadCorbeaux[index_corbeau], NULL, FctThreadCorbeau, NULL))
				perror("Erreur lors de l'initialisation du thread corbeau");
		
			index_corbeau = (++index_corbeau)%4;
		}
		// else{
		// }
	}

	pthread_exit(NULL);
	return NULL;
}

void *FctThreadCorbeau(void *){
	int *position = (int*)malloc(sizeof(int));
	pthread_setspecific(keySpec, (void*)position);
	int img = 1;
	*position = 0;

	sigset_t mask;
	sigfillset(&mask);
	sigdelset(&mask, SIGUSR1);
	sigprocmask(SIG_SETMASK, &mask, NULL);
	
	while(*position<=7){
		pthread_mutex_lock(&mutexGrilleJeu);
		//Collision corbeau
		if(grilleJeu[2][*position].type == DKJR){
			pthread_mutex_unlock(&mutexGrilleJeu);
			kill(getpid(), SIGINT);
			pthread_exit(NULL);
			return NULL;
		}
		setGrilleJeu(2, (*position), CORBEAU, pthread_self());
		afficherCorbeau((*position)*2 + 8, img+1);
		pthread_mutex_unlock(&mutexGrilleJeu);
		
		usleep(700000);

		pthread_mutex_lock(&mutexGrilleJeu);
		setGrilleJeu(2, (*position));
		effacerCarres(10-img, (*position)*2 + 8, 1+img, 1);
		img = (++img)%2;
		(*position)++;
		pthread_mutex_unlock(&mutexGrilleJeu);
	}

	pthread_exit(NULL);
	return NULL;
}

void *FctThreadCroco(void *);

// Handlers---------------------------------------------
void HandlerSIGUSR1(int){
	int positionCorbeau = *(int*)pthread_getspecific(keySpec);
	//Pas le bon corbeau
	if(positionCorbeau != positionDKJr){
		kill(getpid(), SIGUSR1);
		return;
	}
	pthread_mutex_lock(&mutexGrilleJeu);
	setGrilleJeu(2, positionCorbeau);
	pthread_mutex_unlock(&mutexGrilleJeu);

	effacerCarres(9, positionCorbeau*2+8, 2, 1);
	pthread_exit(NULL);
}

void HandlerSIGUSR2(int);

void HandlerSIGALRM(int){
	delaiEnnemis -= 250;
	if(delaiEnnemis > 2500)
		alarm(15);
}

void HandlerSIGINT(int){
	effacerCarres(10, (positionDKJr * 2) + 7, 2, 2);
	setGrilleJeu(2, positionDKJr);

	if(etatDKJr == LIBRE_BAS)
		pthread_mutex_unlock(&mutexEvenement);

	pthread_exit(NULL);
}

void HandlerSIGQUIT(int)
{
	return;
}

void HandlerSIGCHLD(int);

void HandlerSIGHUP(int);


//Destructeur variable spécifique
void DestructeurVS(void *p){
   free(p);
}
