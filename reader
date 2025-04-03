#include <stdio.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <unistd.h>

#define SHM_SIZE 1024

int main() {
    key_t key = ftok("shmfile", 65); // Misma clave

    int shmid = shmget(key, SHM_SIZE, 0666); // Obtener segmento existente
    if (shmid == -1) {
        perror("shmget");
        return 1;
    }

    char *shm_ptr = (char *) shmat(shmid, NULL, 0); // Adjuntar
    if (shm_ptr == (char *) -1) {
        perror("shmat");
        return 1;
    }

    printf("Mensaje recibido: %s", shm_ptr); // Leer desde la memoria compartida

    shmdt(shm_ptr); // Desvincular memoria
    shmctl(shmid, IPC_RMID, NULL); // Eliminar segmento de memoria compartida

    return 0;
}
