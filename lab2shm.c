// lab2_shm.c
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/sem.h>

#define MAX_PEDIDOS 5
#define PEDIDO_LEN 64

typedef struct {
    int cliente_id;
    char pedido[PEDIDO_LEN];
    int estado; // 0: libre, 1: recibido, 2: preparado
} Pedido;

typedef struct {
    Pedido cola[MAX_PEDIDOS];
    int head; // próximo a atender
    int tail; // siguiente a insertar
} BufferCompartido;

#define SHM_KEY 0x1234
#define SEM_KEY 0x5678

int sem_id;

// Inicializar los semáforos
void init_semaforos() {
    sem_id = semget(SEM_KEY, 3, IPC_CREAT | 0666);
    semctl(sem_id, 0, SETVAL, 1);            // mutex
    semctl(sem_id, 1, SETVAL, MAX_PEDIDOS);  // espacios disponibles
    semctl(sem_id, 2, SETVAL, 0);            // pedidos disponibles
}

void sem_wait(int sem_num) {
    struct sembuf op = {sem_num, -1, 0};
    semop(sem_id, &op, 1);
}

void sem_signal(int sem_num) {
    struct sembuf op = {sem_num, +1, 0};
    semop(sem_id, &op, 1);
}

// CLIENTE
void cliente(int id) {
    int shm_id = shmget(SHM_KEY, sizeof(BufferCompartido), 0666);
    if (shm_id == -1) {
        perror("Cliente: Error al obtener memoria compartida");
        exit(EXIT_FAILURE);
    }

    BufferCompartido *buffer = (BufferCompartido *) shmat(shm_id, NULL, 0);

    char comida[PEDIDO_LEN];
    printf("Cliente %d - Ingrese su pedido: ", id);
    fgets(comida, PEDIDO_LEN, stdin);
    comida[strcspn(comida, "\n")] = '\0';

    // Esperar espacio disponible
    sem_wait(1); // espacio disponible
    sem_wait(0); // mutex

    // Escribir pedido
    Pedido *nuevo = &buffer->cola[buffer->tail];
    nuevo->cliente_id = id;
    strcpy(nuevo->pedido, comida);
    nuevo->estado = 1;

    printf("Cliente %d: Pedido enviado: %s\n", id, comida);

    buffer->tail = (buffer->tail + 1) % MAX_PEDIDOS;

    sem_signal(0); // mutex
    sem_signal(2); // pedidos disponibles

    shmdt(buffer);
}

// COCINA
void cocina() {
    int shm_id = shmget(SHM_KEY, sizeof(BufferCompartido), IPC_CREAT | 0666);
    if (shm_id == -1) {
        perror("Cocina: Error creando memoria compartida");
        exit(EXIT_FAILURE);
    }

    BufferCompartido *buffer = (BufferCompartido *) shmat(shm_id, NULL, 0);

    init_semaforos();

    // Inicializar la cola
    buffer->head = 0;
    buffer->tail = 0;
    for (int i = 0; i < MAX_PEDIDOS; i++) {
        buffer->cola[i].estado = 0;
    }

    printf("Cocina lista para atender pedidos...\n");

    while (1) {
        sem_wait(2); // esperar pedido disponible
        sem_wait(0); // mutex

        Pedido *p = &buffer->cola[buffer->head];
        printf("Cocina: Atendiendo pedido de Cliente %d: %s\n", p->cliente_id, p->pedido);
        p->estado = 2;

        buffer->head = (buffer->head + 1) % MAX_PEDIDOS;

        sem_signal(0); // mutex
        sem_signal(1); // espacio disponible

        sleep(3); // Simular preparación
        printf("Cocina: Pedido de Cliente %d listo.\n", p->cliente_id);
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

