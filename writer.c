#include <stdio.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <string.h>
#include <unistd.h>

#define SHM_SIZE 1024  // Tamaño de la memoria compartida

int main() {
    key_t key = ftok("shmfile", 65); // Clave única
    int shmid = shmget(key, SHM_SIZE, 0666 | IPC_CREAT); // Crear segmento

    if (shmid == -1) {
        perror("shmget");
        return 1;
    }

    char *shm_ptr = (char *) shmat(shmid, NULL, 0); // Adjuntar segmento
    if (shm_ptr == (char *) -1) {
        perror("shmat");
        return 1;
    }

    printf("Escriba un mensaje: ");
    fgets(shm_ptr, SHM_SIZE, stdin); // Escribir en la memoria compartida

    printf("Mensaje escrito en la memoria compartida.\n");

    shmdt(shm_ptr); // Desvincular memoria

    return 0;
}
