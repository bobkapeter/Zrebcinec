/* 
 * File:        main.c
 * Author:      Peter Bobka
 * Project:     POS: Domaca uloha c.2 - Zrebcinec 
 */

#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <pthread.h>

#define ANSI_COLOR_GREEN   "\x1b[1;32m"
#define ANSI_COLOR_YELLOW  "\x1b[1;33m"
#define ANSI_COLOR_RED     "\x1b[1;31m"
#define ANSI_COLOR_RESET   "\x1b[0m"


static int exitSignal = 0;


/* Struktura reprezentujuca zdielane data */
typedef struct 
{
    int mnozstvoKrmiva;
    int maxKapacitaValova;
    
    pthread_mutex_t * mutex; 
    pthread_cond_t * naplneny; 
    pthread_cond_t * prazdny; 
    
} thread_data;


/* Struktura reprezentujuca konzumenta */
typedef struct 
{
    int id;
    thread_data * threadData;

} konz_data;


/* Vrat cislo nacitane od uzivatela */
int getNumber(char * message) 
{
    int myInt = 0;
    

    while (myInt <= 0)
    {
        printf(message);
        printf("%s", ANSI_COLOR_YELLOW);
        scanf("%d", &myInt);
        printf("%s", ANSI_COLOR_RESET);
        while (getchar() != '\n');
    }

    
    return myInt;
}


/* Funkcia reagujuca na SIGINT */
void signal_ctrl_c_handler(int sig) 
{
    exitSignal = 1;
}


/* Funkcia pre vlákno typu PRODUCENT */
void * f_producent(void * ptr)
{
    thread_data * threadData = (thread_data *)(ptr);
     
    while(exitSignal == 0)
    {
        pthread_mutex_lock(threadData->mutex);
        
        while (threadData->mnozstvoKrmiva > 0 && exitSignal == 0) 
        {
            pthread_cond_wait(threadData->prazdny, threadData->mutex);
        }
        
        if (exitSignal == 1) break;
        
        printf("\n%s[+]%s Naplnanie valova ...\n", ANSI_COLOR_GREEN, ANSI_COLOR_RESET);

        threadData->mnozstvoKrmiva = threadData->maxKapacitaValova;
        sleep(1);
        
        printf("%s[+]%s ... valov je naplneny.\n\n", ANSI_COLOR_GREEN, ANSI_COLOR_RESET);
        
        pthread_mutex_unlock(threadData->mutex);
        pthread_cond_broadcast(threadData->naplneny);
    }
    
    pthread_mutex_unlock(threadData->mutex);
    pthread_cond_broadcast(threadData->naplneny);
    
    return NULL;
}


/* Funkcia pre vlákno typu KONZUMENT */
void * f_konzument(void * ptr)
{
    konz_data * data = (konz_data *)(ptr);
    thread_data * threadData = data->threadData;

    int casPrechazania = (rand() % 10) + 1;
    
    while(exitSignal == 0)
    {
        pthread_mutex_lock(threadData->mutex);

        while (threadData->mnozstvoKrmiva <= 0 && exitSignal == 0) 
        {
            pthread_cond_wait(threadData->naplneny, threadData->mutex);
        }
        
        if (exitSignal == 1) break;
        
        printf("%s[-]%s Konzumuje kobyla %d\n", ANSI_COLOR_RED, ANSI_COLOR_RESET, data->id);
        threadData->mnozstvoKrmiva--;
        
        if (threadData->mnozstvoKrmiva <= 0) {
            pthread_cond_signal(threadData->prazdny);
        }
        
        pthread_mutex_unlock(threadData->mutex);
     
        /* Simulacia prechazania sa */
        int i;
        for (i = 0; i < casPrechazania; i++) {
            if (exitSignal == 1) break;
            sleep(1);
        }
    }
    
    pthread_cond_signal(threadData->prazdny);
    pthread_mutex_unlock(threadData->mutex);
    
    free(data);
    
    return NULL;
}


/*
 *  MAIN 
 */
int main(int argc, char** argv) 
{
    
    /* Nekonečné hranie hry */
    while (1) 
    {
        int i;
        int pocetKonzumentov = 0;
        int maxKapacitaValova = 0;
        exitSignal = 0;
        int choice = 0;
        
        signal(SIGINT, signal_ctrl_c_handler);
        
        /* Menu */
        while (choice == 0) 
        {
            system("clear");
            printf("1 >> Spustenie hry\n");
            printf("2 >> Ukoncenie\n\n");
            choice = getNumber(">>>> ");
            
            if ( choice == 1 ) 
            {
                /* __ Nova hra __ */
                pocetKonzumentov = getNumber("\nZadaj pocet kobyl: ");
                maxKapacitaValova = getNumber("Zadaj kapacitu valova: ");
            } 
            else if ( choice == 2 )
            {
                /* __ Koniec hry __ */
                return 0; 
            } 
            else 
            {
                /* __ Nespravna volba __ */
                choice = 0;
            }
        }
        
        /* Inicializacia: mutex */
        pthread_mutex_t mutex;
        pthread_mutex_init(&mutex, NULL);

        /* Inicializacia: podmienkove premenne */
        pthread_cond_t naplneny;
        pthread_cond_t prazdny;
        pthread_cond_init(&naplneny, NULL);
        pthread_cond_init(&prazdny, NULL);

        /* Inicializacia: vlakna */
        pthread_t producent;
        pthread_t * konzumenti = malloc(sizeof(pthread_t) * pocetKonzumentov);

        /* Inicializacia: Zdielana struktura */
        thread_data threadData = {
            0,
            maxKapacitaValova,
            &mutex,
            &naplneny,
            &prazdny
        }; 


        /* Vytvorenie: vlákno producenta */
        pthread_create(&producent, NULL, &f_producent, &threadData);

        /* Vytvorenie: vlákna konzumentov */
        for (i = 0; i < pocetKonzumentov; i++) 
        {
            konz_data * konzumentData = (konz_data *)malloc(sizeof(konz_data));

            konzumentData->id = i + 1;
            konzumentData->threadData = &threadData;

            pthread_create(&konzumenti[i], NULL, &f_konzument, konzumentData);
        }


        /* Spustenie: vlákno producenta */
        pthread_join(producent, NULL);

        /* Spustenie: vlákna konzumentov */
        for (i = 0; i < pocetKonzumentov; i++) 
        { 
            pthread_join(konzumenti[i], NULL); 
        }

        /* Uvolnenie prostriedkov */
        pthread_cond_destroy(&prazdny);
        pthread_cond_destroy(&naplneny);
        pthread_mutex_destroy(&mutex);
        free(konzumenti);
        
        printf("\n");
    }
    
    return (EXIT_SUCCESS);
}