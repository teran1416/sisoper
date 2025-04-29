// lab2_fifo.c
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <errno.h>

#define FIFO_PEDIDOS "fifo_pedidos"
#define FIFO_RESPUESTAS "fifo_respuestas"
#define MAX_PEDIDO 128

typedef struct {
    int cliente_id;
    char mensaje[MAX_PEDIDO];
} Pedido;

// Crear los FIFOs si no existen
void inicializar_fifos() {
    if (access(FIFO_PEDIDOS, F_OK) == -1) {
        if (mkfifo(FIFO_PEDIDOS, 0666) == -1 && errno != EEXIST) {
            perror("Error creando fifo_pedidos");
            exit(1);
        }
    }

    if (access(FIFO_RESPUESTAS, F_OK) == -1) {
        if (mkfifo(FIFO_RESPUESTAS, 0666) == -1 && errno != EEXIST) {
            perror("Error creando fifo_respuestas");
            exit(1);
        }
    }
}

// Función del cliente
void cliente(int id) {
    inicializar_fifos(); // Asegura que existan

    Pedido p;
    p.cliente_id = id;

    // Leer pedido del usuario
    printf("Cliente %d - Ingrese su pedido: ", id);
    fgets(p.mensaje, MAX_PEDIDO, stdin);
    p.mensaje[strcspn(p.mensaje, "\n")] = 0; // Eliminar salto de línea

    // Abrir FIFO para escribir el pedido
    int fd_pedidos = open(FIFO_PEDIDOS, O_WRONLY);
    if (fd_pedidos == -1) {
        perror("Error abriendo fifo_pedidos");
        exit(1);
    }
    write(fd_pedidos, &p, sizeof(Pedido));
    close(fd_pedidos);
    printf("Cliente %d - Pedido enviado: %s\n", id, p.mensaje);

    // Abrir FIFO para leer la respuesta
    int fd_respuestas = open(FIFO_RESPUESTAS, O_RDONLY);
    if (fd_respuestas == -1) {
        perror("Error abriendo fifo_respuestas");
        exit(1);
    }
    Pedido respuesta;
    while (1) {
        int bytes = read(fd_respuestas, &respuesta, sizeof(Pedido));
        if (bytes > 0 && respuesta.cliente_id == id) {
            printf("Cliente %d - Respuesta recibida: %s\n", id, respuesta.mensaje);
            break;
        }
    }
    close(fd_respuestas);
}

// Función de la cocina
void cocina() {
    inicializar_fifos(); // Crear FIFOs si no existen

    Pedido p;
    printf("Cocina iniciada. Esperando pedidos...\n");

    while (1) {
        int fd_pedidos = open(FIFO_PEDIDOS, O_RDONLY);
        if (fd_pedidos == -1) {
            perror("Error abriendo fifo_pedidos");
            exit(1);
        }

        int bytes = read(fd_pedidos, &p, sizeof(Pedido));
        close(fd_pedidos);

        if (bytes > 0) {
            printf("Cocina - Pedido recibido de Cliente %d: %s\n", p.cliente_id, p.mensaje);
            printf("Cocina - Preparando...\n");
            sleep(3); // Simula tiempo de preparación

            // Enviar confirmación
            int fd_respuestas = open(FIFO_RESPUESTAS, O_WRONLY);
            if (fd_respuestas == -1) {
                perror("Error abriendo fifo_respuestas");
                exit(1);
            }

            snprintf(p.mensaje, MAX_PEDIDO, "Tu pedido ha sido preparado.");
            write(fd_respuestas, &p, sizeof(Pedido));
            close(fd_respuestas);

            printf("Cocina - Pedido entregado al Cliente %d\n", p.cliente_id);
        }
    }
}

// Main
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
