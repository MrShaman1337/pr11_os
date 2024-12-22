#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <signal.h>

#define PORT 8080
#define BUFFER_SIZE 1024
#define IMAGE_PATH "herb.jpg"
#define VALID_MESSAGE "СтепаковВД_ККСО-06-23_2"

void handle_client(int client_sock, struct sockaddr_in client_addr);
void send_404(int client_sock);
void send_image(int client_sock);
void url_decode(char *src, char *dest);

int main() {
    int server_sock, client_sock;
    struct sockaddr_in server_addr, client_addr;
    socklen_t client_addr_len = sizeof(client_addr);

    if ((server_sock = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
        perror("Ошибка создания сокета");
        exit(EXIT_FAILURE);
    }

    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(PORT);

    if (bind(server_sock, (struct sockaddr *)&server_addr, sizeof(server_addr)) == -1) {
        perror("Ошибка привязки сокета");
        close(server_sock);
        exit(EXIT_FAILURE);
    }

    if (listen(server_sock, 5) == -1) {
        perror("Ошибка прослушивания");
        close(server_sock);
        exit(EXIT_FAILURE);
    }

    printf("Сервер запущен на порту %d\n", PORT);

    while (1) {
        if ((client_sock = accept(server_sock, (struct sockaddr *)&client_addr, &client_addr_len)) == -1) {
            perror("Ошибка принятия соединения");
            continue;
        }

        handle_client(client_sock, client_addr);
        close(client_sock);
    }

    close(server_sock);
    return 0;
}

void handle_client(int client_sock, struct sockaddr_in client_addr) {
    char buffer[BUFFER_SIZE];
    int bytes_received = recv(client_sock, buffer, BUFFER_SIZE - 1, 0);

    if (bytes_received < 0) {
        perror("Ошибка чтения данных от клиента");
        send_404(client_sock);
        return;
    }

    buffer[bytes_received] = '\0';
    printf("Получен запрос: %s\n", buffer);

    char *message_start = strstr(buffer, "GET /?message=");
    if (message_start) {
        message_start += strlen("GET /?message=");
        char *message_end = strchr(message_start, ' ');

        if (message_end) {
            *message_end = '\0';

            char decoded_message[BUFFER_SIZE];
            url_decode(message_start, decoded_message);
            printf("Декодированное сообщение от клиента: %s\n", decoded_message);

            if (strcmp(decoded_message, VALID_MESSAGE) == 0) {
                send_image(client_sock);
                return;
            }
        }
    }

    send_404(client_sock);
}

void send_404(int client_sock) {
    const char *response =
        "HTTP/1.1 404 Not Found\r\n"
        "Content-Type: text/html\r\n\r\n"
        "<html><body><h1>404 Not Found</h1></body></html>";

    send(client_sock, response, strlen(response), 0);
}

void send_image(int client_sock) {
    FILE *image = fopen(IMAGE_PATH, "rb");
    if (!image) {
        perror("Ошибка открытия файла изображения");
        send_404(client_sock);
        return;
    }

    const char *headers =
        "HTTP/1.1 200 OK\r\n"
        "Content-Type: image/jpeg\r\n\r\n";

    send(client_sock, headers, strlen(headers), 0);

    char buffer[BUFFER_SIZE];
    size_t bytes_read;
    while ((bytes_read = fread(buffer, 1, BUFFER_SIZE, image)) > 0) {
        send(client_sock, buffer, bytes_read, 0);
    }

    fclose(image);
}

void url_decode(char *src, char *dest) {
    char *p = src;
    char code[3] = {0};

    while (*p) {
        if (*p == '%') {
            strncpy(code, p + 1, 2);
            *dest++ = (char)strtol(code, NULL, 16);
            p += 3;
        } else if (*p == '+') {
            *dest++ = ' ';
            p++;
        } else {
            *dest++ = *p++;
        }
    }

    *dest = '\0';
}
