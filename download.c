#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

#define URL_MAX_LEN 2048
#define BUFFER_SIZE 2048

// Estrutura com componentes do URL FTP
typedef struct {
    char user[BUFFER_SIZE + 1];
    char pass[BUFFER_SIZE + 1];
    char host[BUFFER_SIZE + 1];
    char filepath[BUFFER_SIZE + 1];
} UrlInfo;

// Estrutura com mensagem de resposta FTP
typedef struct {
    int code;
    char text[BUFFER_SIZE + 1];
    bool is_final;
} FtpResponse;

// Função de erro
void show_error(const char *func, const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);

    fprintf(stderr, "[ERROR] %s: ", func);
    vfprintf(stderr, fmt, args);
    fprintf(stderr, "\n");

    va_end(args);
}

// Parse do URL FTP
int parse_url(char *input_url, UrlInfo *info) {

    if (strlen(input_url) > URL_MAX_LEN) {
        show_error(__func__, "URL length exceeded");
        return 1;
    }

    char *scheme = strtok(input_url, ":");
    char *url_rest = strtok(NULL, "");

    if (scheme == NULL || url_rest == NULL || strncmp(url_rest, "//", 2) != 0) {
        show_error(__func__, "Invalid URL format");
        return 1;
    }

    char *userpass, *domain;
    if (strchr(url_rest + 2, '@') != NULL) {
        // Caso com user:pass
        userpass = strtok(url_rest + 2, "@");
        domain = strtok(NULL, "/");
    } else {
        // Sem user:pass → anonymous
        userpass = NULL;
        domain = strtok(url_rest + 2, "/");
    }

    char *path = strtok(NULL, "");

    if (domain == NULL || path == NULL) {
        show_error(__func__, "Invalid URL components");
        return 1;
    }

    if (strcmp(scheme, "ftp") != 0) {
        show_error(__func__, "Only ftp:// supported");
        return 1;
    }

    char *user, *pass;
    if (userpass == NULL) {
        user = "anonymous";
        pass = "anonymous";
    } else if (strchr(userpass, ':') == NULL) {
        user = userpass;
        pass = "anonymous";
    } else {
        user = strtok(userpass, ":");
        pass = strtok(NULL, "");
    }

    strncpy(info->user, user, BUFFER_SIZE);
    strncpy(info->pass, pass, BUFFER_SIZE);
    strncpy(info->host, domain, BUFFER_SIZE);
    strncpy(info->filepath, path, BUFFER_SIZE);

    return 0;
}


int main(int argc, char **argv) {

    // Verificar argumentos
    if (argc != 2) {
        printf("Usage: %s <FTP URL>\n", argv[0]);
        return 1;
    }

    // Parse do URL
    UrlInfo info;
    memset(&info, 0, sizeof(info));

    char url_copy[URL_MAX_LEN + 1];
    strncpy(url_copy, argv[1], URL_MAX_LEN);

    if (parse_url(url_copy, &info) != 0) {
        show_error(__func__, "Failed to parse URL");
        return 1;
    }

    // Resolver hostname
    struct hostent *host = gethostbyname(info.host);
    if (host == NULL) {
        show_error(__func__, "Could not resolve host");
        return 1;
    }

    // Obter socket de controlo
    int control_sock = connect_to_host(info.host, FTP_PORT);
    if (control_sock < 0) {
        show_error(__func__, "Control connection failed");
        return 1;
    }

    FtpResponse resp;

    // Ler mensagem de boas-vindas
    if (read_response(control_sock, &resp) != 0 || resp.code != 220) {
        show_error(__func__, "Invalid initial server response");
        return 1;
    }

    // Enviar USER
    send_cmd(control_sock, "USER %s\r\n", info.user);
    read_response(control_sock, &resp);
    if (resp.code != 331 && resp.code != 230) {
        show_error(__func__, "USER rejected");
        return 1;
    }

    // Enviar PASS
    if (resp.code == 331) {
        send_cmd(control_sock, "PASS %s\r\n", info.pass);
        read_response(control_sock, &resp);
        if (resp.code != 230) {
            show_error(__func__, "Authentication failed");
            return 1;
        }
    }

    // Modo Passivo
    char pasv_ip[64];
    int pasv_port;

    if (enter_passive(control_sock, pasv_ip, &pasv_port) != 0) {
        show_error(__func__, "PASV failed");
        return 1;
    }

    // Socket de dados
    int data_sock = connect_to_host(pasv_ip, pasv_port);
    if (data_sock < 0) {
        show_error(__func__, "Data connection failed");
        return 1;
    }

    // Pedir ficheiro
    send_cmd(control_sock, "RETR %s\r\n", info.filepath);
    read_response(control_sock, &resp);
    if (resp.code != 150 && resp.code != 125) {
        show_error(__func__, "RETR rejected");
        return 1;
    }

    // Transferir ficheiro
    char *filename = strrchr(info.filepath, '/') ?
                     strrchr(info.filepath, '/') + 1 :
                     info.filepath;

    if (download_file(control_sock, data_sock, filename) != 0) {
        show_error(__func__, "Download failed");
        return 1;
    }

    // QUIT
    send_cmd(control_sock, "QUIT\r\n");
    read_response(control_sock, &resp);

    // Fechar sockets
    close(control_sock);
    close(data_sock);

    printf("Download completed successfully.\n");

    return 0;
}