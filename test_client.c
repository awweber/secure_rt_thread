/* Test-Client für Secure Realtime Server
=====================================================================================================
Autor: Alexander Weber
Datum: Juli 2025

Einfacher Test-Client zum Testen der Server-Funktionalität
=====================================================================================================*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define SERVER_PORT 8080
#define BUFFER_SIZE 256

int main(int argc, char *argv[]) {
    int client_socket;
    struct sockaddr_in server_addr;
    char buffer[BUFFER_SIZE];
    char username[50];
    ssize_t bytes_received;
    const char* server_ip = "127.0.0.1";  // Standardmäßig localhost
    
    if (argc > 1) {
        server_ip = argv[1];
    }
    
    printf("=== SECURE RT CLIENT ===\n");
    printf("Verbinde zu Server: %s:%d\n", server_ip, SERVER_PORT);
    
    // 1. Socket erstellen
    client_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (client_socket < 0) {
        perror("socket");
        return EXIT_FAILURE;
    }
    
    // 2. Server-Adresse konfigurieren
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(SERVER_PORT);
    
    if (inet_pton(AF_INET, server_ip, &server_addr.sin_addr) <= 0) {
        printf("Ungültige Server-IP: %s\n", server_ip);
        close(client_socket);
        return EXIT_FAILURE;
    }
    
    // 3. Zum Server verbinden
    if (connect(client_socket, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        perror("connect");
        close(client_socket);
        return EXIT_FAILURE;
    }
    
    printf("✓ Verbindung zum Server hergestellt\n");
    
    // 4. Authentifizierungsaufforderung empfangen
    memset(buffer, 0, sizeof(buffer));
    bytes_received = recv(client_socket, buffer, sizeof(buffer) - 1, 0);
    if (bytes_received > 0) {
        buffer[bytes_received] = '\0';
        printf("%s", buffer);  // Auth-Prompt ausgeben
    }
    
    // 5. Benutzername eingeben und senden
    if (fgets(username, sizeof(username), stdin) != NULL) {
        send(client_socket, username, strlen(username), 0);
    }
    
    // 6. Authentifizierungsantwort empfangen
    memset(buffer, 0, sizeof(buffer));
    bytes_received = recv(client_socket, buffer, sizeof(buffer) - 1, 0);
    if (bytes_received > 0) {
        buffer[bytes_received] = '\0';
        printf("%s", buffer);
        
        // Wenn Authentifizierung fehlgeschlagen, beenden
        if (strstr(buffer, "failed") != NULL) {
            close(client_socket);
            return EXIT_FAILURE;
        }
    }
    
    printf("\n=== ECHTZEIT-DATEN EMPFANGEN ===\n");
    
    // 7. RT-Thread-Daten kontinuierlich empfangen
    while (1) {
        memset(buffer, 0, sizeof(buffer));
        bytes_received = recv(client_socket, buffer, sizeof(buffer) - 1, 0);
        
        if (bytes_received <= 0) {
            printf("Verbindung zum Server beendet\n");
            break;
        }
        
        buffer[bytes_received] = '\0';
        printf("%s", buffer);
        
        // Wenn "COMPLETED" empfangen, ist RT-Thread beendet
        if (strstr(buffer, "COMPLETED") != NULL) {
            printf("Echtzeit-Thread abgeschlossen\n");
            break;
        }
    }
    
    close(client_socket);
    printf("Client beendet\n");
    return EXIT_SUCCESS;
}
