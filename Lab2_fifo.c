// Lab2_fifo.c
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <signal.h>
#include <errno.h>

#define FIFO_PEDIDOS "fifo_pedidos"
#define FIFO_RESPUESTAS "fifo_respuestas"
#define MAX_PEDIDO 64
#define MAX_RESPUESTA 128

typedef struct {
    int cliente_id;
    char pedido[MAX_PEDIDO];
} Pedido;

typedef struct {
    int cliente_id;
    int estado; // 1: recibido, 2: preparado
    char respuesta[MAX_RESPUESTA];
} Respuesta;

// Crear los FIFOs si no existen
void inicializar_fifos() {
    struct stat st;

    if (stat(FIFO_PEDIDOS, &st) == -1) {
        if (mkfifo(FIFO_PEDIDOS, 0666) == -1) {
            perror("Error creando fifo_pedidos");
            exit(EXIT_FAILURE);
        }
    }

    if (stat(FIFO_RESPUESTAS, &st) == -1) {
        if (mkfifo(FIFO_RESPUESTAS, 0666) == -1) {
            perror("Error creando fifo_respuestas");
            exit(EXIT_FAILURE);
        }
    }
}

// Función para el cliente
void cliente(int id) {
    Pedido p;
    Respuesta r;
    int fd_pedidos, fd_respuestas;

    printf("Cliente %d: ¿Qué desea pedir? ", id);
    fgets(p.pedido, MAX_PEDIDO, stdin);
    p.pedido[strcspn(p.pedido, "\n")] = '\0'; // Quitar salto de línea

    p.cliente_id = id;

    fd_pedidos = open(FIFO_PEDIDOS, O_WRONLY);
    if (fd_pedidos == -1) {
        perror("Cliente: Error abriendo fifo_pedidos");
        exit(EXIT_FAILURE);
    }

    write(fd_pedidos, &p, sizeof(Pedido));
    close(fd_pedidos);

    printf("Cliente %d: Pedido enviado. Esperando confirmaciones...\n", id);

    // Esperar confirmación de recepción y preparación
    int recibido = 0, preparado = 0;
    while (!preparado) {
        fd_respuestas = open(FIFO_RESPUESTAS, O_RDONLY);
        if (fd_respuestas == -1) {
            perror("Cliente: Error abriendo fifo_respuestas");
            exit(EXIT_FAILURE);
        }

        while (read(fd_respuestas, &r, sizeof(Respuesta)) > 0) {
            if (r.cliente_id == id) {
                if (r.estado == 1 && !recibido) {
                    printf("Cliente %d: Confirmación de recepción: %s\n", id, r.respuesta);
                    recibido = 1;
                } else if (r.estado == 2) {
                    printf("Cliente %d: Pedido listo: %s\n", id, r.respuesta);
                    preparado = 1;
                }
            }
        }
        close(fd_respuestas);
        usleep(100000); // Esperar un poco antes de reintentar
    }
}

// Manejador de señal para eliminar los FIFOs al salir
void limpiar(int sig) {
    unlink(FIFO_PEDIDOS);
    unlink(FIFO_RESPUESTAS);
    printf("\nFIFOs eliminados. Saliendo...\n");
    exit(0);
}

// Función para la cocina
void cocina() {
    Pedido p;
    Respuesta r;
    int fd_pedidos, fd_respuestas;

    inicializar_fifos();
    signal(SIGINT, limpiar);

    printf("Cocina lista para recibir pedidos...\n");

    while (1) {
        fd_pedidos = open(FIFO_PEDIDOS, O_RDONLY);
        if (fd_pedidos == -1) {
            perror("Cocina: Error abriendo fifo_pedidos");
            continue;
        }

        if (read(fd_pedidos, &p, sizeof(Pedido)) > 0) {
            printf("Cocina: Pedido recibido de Cliente %d: %s\n", p.cliente_id, p.pedido);
            close(fd_pedidos);

            // Enviar confirmación de recibido
            r.cliente_id = p.cliente_id;
            r.estado = 1;
            snprintf(r.respuesta, MAX_RESPUESTA, "Pedido '%s' recibido.", p.pedido);

            fd_respuestas = open(FIFO_RESPUESTAS, O_WRONLY);
            if (fd_respuestas != -1) {
                write(fd_respuestas, &r, sizeof(Respuesta));
                close(fd_respuestas);
            }

            // Simular preparación del pedido
            printf("Cocina: Preparando pedido de Cliente %d...\n", p.cliente_id);
            sleep(3); // Tiempo de preparación

            // Enviar confirmación de preparado
            r.estado = 2;
            snprintf(r.respuesta, MAX_RESPUESTA, "Pedido '%s' listo para recoger.", p.pedido);

            fd_respuestas = open(FIFO_RESPUESTAS, O_WRONLY);
            if (fd_respuestas != -1) {
                write(fd_respuestas, &r, sizeof(Respuesta));
                close(fd_respuestas);
            }

            printf("Cocina: Pedido de Cliente %d finalizado.\n", p.cliente_id);
        } else {
            close(fd_pedidos);
        }
    }
}

int main(int argc, char *argv[]) {
    if (argc < 2) {
        printf("Uso: %s [cliente ID | cocina]\n", argv[0]);
        return 1;
    }

    if (strcmp(argv[1], "cocina") == 0) {
        cocina();
    } else {
        int cliente_id = atoi(argv[1]);
        if (cliente_id <= 0) {
            printf("ID de cliente inválido.\n");
            return 1;
        }
        cliente(cliente_id);
    }

    return 0;
}

