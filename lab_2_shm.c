// lab2_shm.c
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/sem.h>
#include <time.h>

#define MAX_PEDIDOS 5
#define PEDIDO_LEN 64

// Estructura del pedido
typedef struct {
    int cliente_id;
    char pedido[PEDIDO_LEN];
    int estado; // 0: libre, 1: recibido, 2: preparado
} Pedido;

// Memoria compartida completa
typedef struct {
    Pedido cola[MAX_PEDIDOS];
    int head;  // próximo a atender
    int tail;  // próximo espacio libre
} BufferCompartido;

#define SHM_KEY 0x1234
#define SEM_KEY 0x5678

// Semáforos: 0 = mutex, 1 = espacio disponible, 2 = pedidos disponibles
int sem_id;

void init_semaforos() {
    sem_id = semget(SEM_KEY, 3, IPC_CREAT | 0666);
    semctl(sem_id, 0, SETVAL, 1);             // mutex
    semctl(sem_id, 1, SETVAL, MAX_PEDIDOS);   // espacios disponibles
    semctl(sem_id, 2, SETVAL, 0);             // pedidos disponibles
}

void sem_wait(int sem_num) {
    struct sembuf op = {sem_num, -1, 0};
    semop(sem_id, &op, 1);
}

void sem_signal(int sem_num) {
    struct sembuf op = {sem_num, +1, 0};
    semop(sem_id, &op, 1);
}

// Cliente
void cliente(int id) {
    int shm_id = shmget(SHM_KEY, sizeof(BufferCompartido), 0666);
    if (shm_id == -1) {
        perror("Error conectando a memoria compartida");
        exit(1);
    }

    BufferCompartido *buffer = (BufferCompartido *) shmat(shm_id, NULL, 0);
    sem_id = semget(SEM_KEY, 3, 0666);

    char comida[PEDIDO_LEN];
    printf("Cliente %d - Ingrese su pedido: ", id);
    fgets(comida, PEDIDO_LEN, stdin);
    comida[strcspn(comida, "\n")] = 0;

    // Esperar espacio disponible
    sem_wait(1);      // Espacio disponible
    sem_wait(0);      // Mutex

    // Escribir el pedido
    Pedido *slot = &buffer->cola[buffer->tail];
    slot->cliente_id = id;
    strcpy(slot->pedido, comida);
    slot->estado = 1; // recibido

    printf("Cliente %d - Pedido enviado: %s\n", id, slot->pedido);

    buffer->tail = (buffer->tail + 1) % MAX_PEDIDOS;

    sem_signal(0);    // Mutex
    sem_signal(2);    // Pedido disponible

    // Esperar a que sea preparado
    int recibido = 0;
    while (!recibido) {
        sem_wait(0);  // Mutex
        for (int i = 0; i < MAX_PEDIDOS; ++i) {
            if (buffer->cola[i].cliente_id == id && buffer->cola[i].estado == 2) {
                printf("Cliente %d - Pedido preparado: %s\n", id, buffer->cola[i].pedido);
                buffer->cola[i].estado = 0; // Liberar slot
                recibido = 1;
                break;
            }
        }
        sem_signal(0); // Mutex
        if (!recibido) sleep(1); // Evitar CPU al 100%
    }

    shmdt(buffer);
}

// Cocina
void cocina() {
    int shm_id = shmget(SHM_KEY, sizeof(BufferCompartido), IPC_CREAT | 0666);
    if (shm_id == -1) {
        perror("Error creando memoria compartida");
        exit(1);
    }

    BufferCompartido *buffer = (BufferCompartido *) shmat(shm_id, NULL, 0);
    init_semaforos();

    buffer->head = 0;
    buffer->tail = 0;

    printf("Cocina - Lista para atender pedidos...\n");

    while (1) {
        sem_wait(2); // Esperar que haya pedidos
        sem_wait(0); // Mutex

        Pedido *slot = &buffer->cola[buffer->head];
        if (slot->estado == 1) {
            printf("Cocina - Atendiendo pedido de Cliente %d: %s\n", slot->cliente_id, slot->pedido);
            buffer->head = (buffer->head + 1) % MAX_PEDIDOS;
        }

        sem_signal(0); // Mutex

        sleep(3); // Simula tiempo de preparación

        sem_wait(0); // Mutex
        slot->estado = 2; // preparado
        sem_signal(0); // Mutex

        sem_signal(1); // Aumenta espacio disponible
    }

    shmdt(buffer);
}

int main(int argc, char *argv[]) {
    if (argc < 2) {
        printf("Uso: %s [cliente ID | cocina]\n", argv[0]);
        return 1;
    }

    if (strcmp(argv[1], "cocina") == 0) {
        cocina();
    } else {
        int id = atoi(argv[1]);
        cliente(id);
    }

    return 0;
}
