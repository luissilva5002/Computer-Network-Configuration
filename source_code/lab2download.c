#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <stdarg.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <unistd.h>
#include <time.h>
#include <errno.h>

#define BAR_WIDTH 25
#define URL_MAX_LEN 2048
#define BUFFER_SIZE 2048
#define FTP_PORT 21

typedef struct {
    char user[BUFFER_SIZE + 1];
    char pass[BUFFER_SIZE + 1];
    char host[BUFFER_SIZE + 1];
    char filepath[BUFFER_SIZE + 1];
} UrlInfo;

typedef struct {
    int code;
    char text[BUFFER_SIZE + 1];
    bool is_final;
} FtpResponse;

// --- Funcoes auxiliares ---

void show_error(const char *func, const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);
    fprintf(stderr, "[ERROR] %s: ", func);
    vfprintf(stderr, fmt, args);
    fprintf(stderr, "\n");
    va_end(args);
}

// --- Funcoes Principais ---

int parse_url(char *input_url, UrlInfo *info) {
    if (strlen(input_url) > URL_MAX_LEN)
        return 1;

    char *scheme = strtok(input_url, ":");
    char *url_rest = strtok(NULL, "");

    if (!scheme || !url_rest || strncmp(url_rest, "//", 2) != 0)
        return 1;

    char *userpass = NULL;
    char *domain = NULL;
    char *url_start = url_rest + 2;

    if (strchr(url_start, '@')) {
        userpass = strtok(url_start, "@");
        domain = strtok(NULL, "/");
    } else {
        domain = strtok(url_start, "/");
    }

    char *path = strtok(NULL, "");
    if (!domain || !path)
        return 1;

    if (strcmp(scheme, "ftp") != 0)
        return 1;

    char *user, *pass;
    if (!userpass) {
        user = "anonymous";
        pass = "anonymous";
    } else if (!strchr(userpass, ':')) {
        user = userpass;
        pass = "anonymous";
    } else {
        char userpass_copy[BUFFER_SIZE+1];
        strncpy(userpass_copy, userpass, BUFFER_SIZE);
        userpass_copy[BUFFER_SIZE] = '\0';
        
        user = strtok(userpass_copy, ":");
        pass = strtok(NULL, "");
    }

    strncpy(info->user, user, BUFFER_SIZE);
    strncpy(info->pass, pass, BUFFER_SIZE);
    strncpy(info->host, domain, BUFFER_SIZE);
    strncpy(info->filepath, path, BUFFER_SIZE);
    info->user[BUFFER_SIZE] = '\0';
    info->pass[BUFFER_SIZE] = '\0';
    info->host[BUFFER_SIZE] = '\0';
    info->filepath[BUFFER_SIZE] = '\0';
    return 0;
}

int connect_to_address(const char *addr, uint16_t port) {
    int sockfd;
    struct sockaddr_in server_addr;

    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);

    if (inet_pton(AF_INET, addr, &server_addr.sin_addr) <= 0)
        return -1;

    if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
        return -1;

    if (connect(sockfd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        close(sockfd);
        return -1;
    }
    return sockfd;
}

int connect_to_host(const char *hostname, uint16_t port) {
    struct hostent *host = gethostbyname(hostname);
    if (!host)
        return -1;

    const char *ip = inet_ntoa(*((struct in_addr *) host->h_addr_list[0]));
    return connect_to_address(ip, port);
}

int read_response(int sock, FtpResponse *resp) {
    char buffer[BUFFER_SIZE + 1];
    resp->is_final = false;

    int expected_code = -1;
    resp->text[0] = '\0';

    while (1) {
        int i = 0;
        char c = 0;
        ssize_t read_bytes;

        // le a linha, para em \r ou quando o buffer estiver cheio
        while (i < BUFFER_SIZE && (read_bytes = read(sock, &c, 1)) > 0 && c != '\r')
            buffer[i++] = c;
        
        if (read_bytes <= 0 && i == 0) {
             return 1;
        }

        buffer[i] = '\0';
        
        // consome \n apos \r
        if (c == '\r') {
            read(sock, &c, 1); 
        }

        int code;
        char sep;
        int n = 0;

        if (sscanf(buffer, "%d%c%n", &code, &sep, &n) == 2) {
            
            if (expected_code == -1)
                expected_code = code;

            if (resp->text[0] != '\0') {
                 strncat(resp->text, " | ", BUFFER_SIZE - strlen(resp->text));
            }
            strncat(resp->text, buffer + n, BUFFER_SIZE - strlen(resp->text));
            resp->text[BUFFER_SIZE] = '\0';

            resp->code = code;

            if (sep == ' ') {
                resp->is_final = true;
                break;
            }
        } else if (expected_code != -1) {

        } else {
            return 1;
        }
    }
    return 0;
}


int send_cmd(int sock, const char *fmt, ...) {
    char cmd[BUFFER_SIZE + 1];

    va_list args;
    va_start(args, fmt);
    int len = vsnprintf(cmd, BUFFER_SIZE - 2, fmt, args);
    va_end(args);

    if (len >= BUFFER_SIZE - 2) return 1;

    strcpy(cmd + len, "\r\n");

    if (write(sock, cmd, len + 2) < 0)
        return 1;

    return 0;
}

void print_progress(size_t current, size_t total) {
    int width = total ? (current * BAR_WIDTH / total) : 0;
    if (width > BAR_WIDTH) width = BAR_WIDTH;

    double pct = total ? (current * 100.0 / total) : 0;

    printf("\x1B[2KDownloading... [%.*s%*s] %.1f%%\r",
           width, "=========================" , BAR_WIDTH - width, "", pct);
    fflush(stdout);
}

void reduce_unit(size_t size, const char **unit, size_t *modifier) {
    if (size >= 1024 * 1024) { *unit = "MiB"; *modifier = 1024*1024; }
    else if (size >= 1024) { *unit = "KiB"; *modifier = 1024; }
    else { *unit = "B"; *modifier = 1; }
}

void print_transfer_stats(size_t bytes, struct timespec s, struct timespec e) {
    double elapsed = (e.tv_sec - s.tv_sec) + (e.tv_nsec - s.tv_nsec)/1e9;
    double speed = bytes / (elapsed > 0 ? elapsed : 1e-6);

    const char *us, *uu;
    size_t ms, mu;

    reduce_unit(bytes, &us, &ms);
    reduce_unit(speed, &uu, &mu);

    printf("\n============ STATISTICS ============\n"); 
    printf("Total time: %.3f s\n", elapsed);          
    printf("Transferred: %.2f %s\n", bytes/(double)ms, us); 
    printf("Speed: %.2f %s/s\n", speed/mu, uu);         
    printf("====================================\n\n"); 
}

int enter_passive(int sock, char *ip_out, int *port_out) {
    FtpResponse resp;
    if (send_cmd(sock, "PASV")) return 1;
    if (read_response(sock, &resp)) return 1;

    if (resp.code != 227)
        return 1;

    uint8_t a,b,c,d,p1,p2;
    if (sscanf(resp.text, "Entering Passive Mode (%hhu,%hhu,%hhu,%hhu,%hhu,%hhu)",
               &a,&b,&c,&d,&p1,&p2) != 6)
        return 1;

    sprintf(ip_out, "%u.%u.%u.%u", a,b,c,d);
    *port_out = p1*256 + p2;
    return 0;
}

int download_file(int control_sock, int data_sock, const char *filename, size_t total_size) {
    FILE *fp = fopen(filename, "wb");
    if (!fp) {
        show_error("download_file", "Failed to open file '%s' for writing: %s", filename, strerror(errno));
        close(data_sock);
        return 1;
    }

    char buf[BUFFER_SIZE];
    ssize_t n;
    size_t total = 0;

    struct timespec start, end;
    clock_gettime(CLOCK_MONOTONIC, &start);

    while ((n = read(data_sock, buf, BUFFER_SIZE)) > 0) {
        fwrite(buf, 1, n, fp);
        total += n;
        print_progress(total, total_size);
    }

    close(data_sock);
    fclose(fp);

    clock_gettime(CLOCK_MONOTONIC, &end);
    print_transfer_stats(total, start, end);

    FtpResponse resp;
    if (read_response(control_sock, &resp)) return 1;
    
    if (resp.code != 226 && resp.code != 250)
        return 1;

    printf("\nFile saved as '%s'\n", filename); 
    return 0;
}

int main(int argc, char **argv) {
    if (argc != 2) {
        printf("Usage: %s <FTP URL>\n", argv[0]);
        return 1;
    }

    UrlInfo info;
    memset(&info,0,sizeof(info));

    char url_copy[URL_MAX_LEN+1];
    strncpy(url_copy, argv[1], URL_MAX_LEN);
    url_copy[URL_MAX_LEN] = '\0';

    if (parse_url(url_copy, &info)) {
        printf("URL parse error.\n");
        return 1;
    }

    int control_sock = connect_to_host(info.host, FTP_PORT);
    if (control_sock < 0) {
        show_error("main", "Control connection failed.");
        return 1;
    }

    FtpResponse resp;
    read_response(control_sock, &resp);
    if (resp.code != 220) {
        show_error("main", "Initial connection banner failed. Expected 220, got %d.", resp.code);
        close(control_sock);
        return 1;
    }

    if (send_cmd(control_sock, "USER %s", info.user)) { close(control_sock); return 1; }
    if (read_response(control_sock, &resp)) { close(control_sock); return 1; }
    if (resp.code != 331 && resp.code != 230) {
        show_error("main", "USER command failed. Expected 331 or 230, got %d.", resp.code);
        close(control_sock);
        return 1;
    }

    if (resp.code == 331) {
        if (send_cmd(control_sock, "PASS %s", info.pass)) { close(control_sock); return 1; }
        if (read_response(control_sock, &resp)) { close(control_sock); return 1; }
        if (resp.code != 230) {
            show_error("main", "PASS command failed. Expected 230, got %d.", resp.code);
            close(control_sock);
            return 1;
        }
    }

    if (send_cmd(control_sock, "TYPE I")) { close(control_sock); return 1; }
    if (read_response(control_sock, &resp)) { close(control_sock); return 1; }



    size_t filesize = 0;
    if (send_cmd(control_sock, "SIZE %s", info.filepath)) { close(control_sock); return 1; }
    if (read_response(control_sock, &resp)) { close(control_sock); return 1; }
    if (resp.code == 213) {
        sscanf(resp.text, "%zu", &filesize);
    }


    char pasv_ip[64];
    int pasv_port;
    if (enter_passive(control_sock, pasv_ip, &pasv_port)) {
        show_error("main", "Failed to enter passive mode.");
        close(control_sock);
        return 1;
    }
    int data_sock = connect_to_address(pasv_ip, pasv_port);
    if (data_sock < 0) {
        show_error("main", "Data connection failed.");
        close(control_sock);
        return 1;
    }


    if (send_cmd(control_sock, "RETR %s", info.filepath)) { close(control_sock); return 1; }
    if (read_response(control_sock, &resp)) { close(control_sock); return 1; }
    if (resp.code != 150 && resp.code != 125) {
        show_error("main", "RETR command failed. Expected 150 or 125, got %d.", resp.code);
        close(control_sock);
        return 1;
    }



    char *filename = strrchr(info.filepath, '/') ? strrchr(info.filepath, '/')+1 : info.filepath;


    if (download_file(control_sock, data_sock, filename, filesize))
        return 1;

    if (send_cmd(control_sock, "QUIT")) { close(control_sock); return 1; }
    read_response(control_sock, &resp); 

    close(control_sock);

    printf("Download completed successfully.\n"); 
    return 0;
}