/* Teilprüfung 6: Client-Server-basierte Sicherheitsimplementierung
=====================================================================================================
Projekt: Sicherheitsaspekte von Echtzeit-Embedded-Linux-Systemen (Server-Version)

Autor: Alexander Weber
Datum: Juli 2025

Dieses Programm implementiert einen TCP-Server, der nur Verbindungen von autorisierten
IP-Adressen akzeptiert und nach erfolgreicher Authentifizierung Echtzeit-Threads startet.

=====================================================================================================*/

#define _GNU_SOURCE           // Aktiviert GNU-spezifische Funktionen

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <sched.h>
#include <time.h>
#include <errno.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/mman.h>
#include <signal.h>

// Server-Konstanten
#define SERVER_PORT 8080
#define MAX_CLIENTS 5
#define BUFFER_SIZE 256

// Echtzeit-Konstanten
#define RT_PRIORITY 50
#define TASK_PERIOD_SEC 1
#define MAX_CYCLES 20

// Sicherheitskonstanten
#define MAX_USERNAME_LENGTH 50
#define AUTHORIZED_USER "admin"

// Netzwerksicherheitskonstanten
#define AUTHORIZED_IP "127.0.0.1"     // Autorisierte Client-IP
#define ALTERNATIVE_IP "192.168.1.100" // Alternative autorisierte Client-IP

// Globale Variablen für sauberes Shutdown
volatile int server_running = 1;
int server_socket = -1;

// ========================================
// CLIENT-DATENSTRUKTUR
// ========================================
// Enthält Informationen über verbundene Clients
// Wird in Threads verwendet, um Client-spezifische Operationen durchzuführen
// Diese Struktur ermöglicht es, mehrere Clients gleichzeitig zu bedienen
// und deren Verbindungen zu verwalten
// Enthält Socket, Adresse, IP und Authentifizierungsstatus
// Wird in handle_client() verwendet, um Threads für jeden Client zu starten
typedef struct {
    int client_socket;
    struct sockaddr_in client_addr;
    char client_ip[INET_ADDRSTRLEN];
    int authenticated;
    pthread_t thread_id;
} client_info_t;

// ========================================
// SIGNAL-HANDLER FÜR SAUBERES SHUTDOWN
// ========================================
// Beendet den Server bei SIGINT oder SIGTERM
// Wird aufgerufen, wenn der Server mit STRG+C oder kill beendet wird
void signal_handler(int sig) {
    printf("\n=== SERVER SHUTDOWN ===\n");
    printf("Signal %d empfangen, beende Server...\n", sig);
    server_running = 0;
    
    if (server_socket != -1) {
        close(server_socket);
    }
}

// ========================================
// IP-ADRESS-AUTORISIERUNG
// ========================================
// Überprüft ob die Client-IP autorisiert ist
int check_client_ip_authorization(const char* client_ip) {
    printf("Überprüfe Client-IP: %s\n", client_ip);
    
    // Autorisierte IPs prüfen
    if (strcmp(client_ip, AUTHORIZED_IP) == 0) {
        printf("✓ IP-Adresse %s ist autorisiert (Primary)\n", client_ip);
        return 1;
    } else if (strcmp(client_ip, ALTERNATIVE_IP) == 0) {
        printf("✓ IP-Adresse %s ist autorisiert (Alternative)\n", client_ip);
        return 1;
    } else {
        printf("✗ IP-Adresse %s ist NICHT autorisiert!\n", client_ip);
        printf("✗ Autorisierte IPs: %s, %s\n", AUTHORIZED_IP, ALTERNATIVE_IP);
        return 0;
    }
}

// ========================================
// CLIENT-AUTHENTIFIZIERUNG ÜBER NETZWERK
// ========================================
// Authentifiziert den Client über Netzwerkkommunikation
// Erwartet Benutzernamen und prüft ob er autorisiert ist
// Wird in handle_client() verwendet, um sicherzustellen, dass nur autorisierte Clients Echtzeit-Threads starten können
// Sendet Authentifizierungsaufforderung und empfängt Benutzernamen
// Vergleicht den Benutzernamen mit dem autorisierten Benutzernamen
int authenticate_network_client(int client_socket) {
    char buffer[BUFFER_SIZE];
    char username[MAX_USERNAME_LENGTH];
    ssize_t bytes_received;
    
    // Authentifizierungsaufforderung senden
    const char* auth_prompt = "=== REMOTE AUTHENTICATION ===\nUsername: ";
    if (send(client_socket, auth_prompt, strlen(auth_prompt), 0) < 0) {
        perror("send auth_prompt");
        return 0;
    }
    
    // Benutzername empfangen
    memset(buffer, 0, sizeof(buffer));
    bytes_received = recv(client_socket, buffer, sizeof(buffer) - 1, 0);
    if (bytes_received <= 0) {
        printf("Fehler beim Empfangen des Benutzernamens\n");
        return 0;
    }
    
    // Newline entfernen und in username kopieren
    buffer[bytes_received] = '\0';
    strncpy(username, buffer, sizeof(username) - 1);
    username[sizeof(username) - 1] = '\0';
    
    // Newline am Ende entfernen
    char* newline = strchr(username, '\n');
    if (newline) *newline = '\0';
    char* carriage = strchr(username, '\r');
    if (carriage) *carriage = '\0';
    
    printf("Empfangener Benutzername: '%s'\n", username);
    
    // Authentifizierung prüfen
    if (strcmp(username, AUTHORIZED_USER) == 0) {
        const char* success_msg = "✓ Authentication successful! RT access granted.\n";
        send(client_socket, success_msg, strlen(success_msg), 0);
        printf("✓ Client erfolgreich authentifiziert: %s\n", username);
        return 1;
    } else {
        const char* error_msg = "✗ Authentication failed! Access denied.\n";
        send(client_socket, error_msg, strlen(error_msg), 0);
        printf("✗ Authentifizierung fehlgeschlagen für: %s\n", username);
        return 0;
    }
}

// ========================================
// ECHTZEIT-TASK FÜR CLIENT
// ========================================
// Führt die Echtzeit-Operationen für einen verbundenen Client aus
// Wird in einem separaten Thread für jeden Client gestartet
// Simuliert eine Echtzeit-Task, die periodisch ausgeführt wird
// Nutzt clock_nanosleep() für präzise Zeitsteuerung
void* client_realtime_task(void* arg) {
    client_info_t* client = (client_info_t*)arg;
    struct timespec next_period, current_time;
    int cycle_count = 0;
    char message[BUFFER_SIZE];
    
    printf("Echtzeit-Thread gestartet für Client %s\n", client->client_ip);
    
    // Timing initialisieren
    if (clock_gettime(CLOCK_MONOTONIC, &next_period) != 0) {
        perror("clock_gettime");
        return NULL;
    }
    
    // Startmeldung an Client senden
    snprintf(message, sizeof(message), 
             "=== REALTIME THREAD STARTED ===\nPriority: %d, Cycles: %d\n", 
             RT_PRIORITY, MAX_CYCLES);
    send(client->client_socket, message, strlen(message), 0);
    
    // Echtzeit-Hauptschleife
    // Führt MAX_CYCLES Zyklen aus, jeder genau TASK_PERIOD_SEC Sekunden nach dem vorherigen
    while (cycle_count < MAX_CYCLES && server_running) {
        // Nächste Periode berechnen
        next_period.tv_sec += TASK_PERIOD_SEC;
        
        // Präzise Wartezeit
        if (clock_nanosleep(CLOCK_MONOTONIC, TIMER_ABSTIME, &next_period, NULL) != 0) {
            if (errno != EINTR) {
                perror("clock_nanosleep");
                break;
            }
        }
        
        // Aktuelle Zeit messen
        clock_gettime(CLOCK_MONOTONIC, &current_time);
        
        // RT-Task ausführen und an Client melden
        cycle_count++;
        snprintf(message, sizeof(message), 
                "[Cycle %02d] RT-Task executed at %ld.%03ld for %s\n", 
                cycle_count, 
                current_time.tv_sec, 
                current_time.tv_nsec / 1000000,
                client->client_ip);
        
        printf("%s", message);  // Lokale Ausgabe
        
        // An Client senden (mit Fehlerbehandlung)
        if (send(client->client_socket, message, strlen(message), MSG_NOSIGNAL) < 0) {
            printf("Client %s getrennt, beende RT-Thread\n", client->client_ip);
            break;
        }
        
        // Deterministische Arbeitslast simulieren
        for (volatile int i = 0; i < 100000; i++);
    }
    
    // Abschlussmeldung
    snprintf(message, sizeof(message), 
             "=== RT-THREAD COMPLETED ===\nExecuted %d cycles\n", cycle_count);
    send(client->client_socket, message, strlen(message), MSG_NOSIGNAL);
    
    printf("Echtzeit-Thread beendet für Client %s nach %d Zyklen\n", 
           client->client_ip, cycle_count);
    
    return NULL;
}

// ========================================
// CLIENT-HANDLER-THREAD
// ========================================
// Hinweis: Der Pointer 'client' MUSS mit malloc() alloziert werden!
// Niemals einen stack-allozierten oder wiederverwendeten Pointer übergeben!
void* handle_client(void* arg) {
    client_info_t* client = (client_info_t*)arg;
    pthread_t rt_thread;
    struct sched_param param;
    pthread_attr_t attr;
    int ret;
    
    printf("\n=== NEUER CLIENT ===\n");
    printf("Client verbunden von IP: %s\n", client->client_ip);
    
    // 1. IP-Autorisierung prüfen
    if (!check_client_ip_authorization(client->client_ip)) {
        const char* ip_error = "✗ IP address not authorized. Connection refused.\n";
        send(client->client_socket, ip_error, strlen(ip_error), 0);
        printf("Verbindung zu %s aus Sicherheitsgründen abgelehnt\n", client->client_ip);
        goto cleanup;
    }
    
    // 2. Client-Authentifizierung
    if (!authenticate_network_client(client->client_socket)) {
        printf("Authentifizierung für %s fehlgeschlagen\n", client->client_ip);
        goto cleanup;
    }
    
    client->authenticated = 1;
    printf("Client %s vollständig autorisiert\n", client->client_ip);
    
    // 3. Echtzeit-Thread-Attribute konfigurieren
    ret = pthread_attr_init(&attr);
    if (ret == 0) {
        ret = pthread_attr_setschedpolicy(&attr, SCHED_FIFO);
        if (ret == 0) {
            param.sched_priority = RT_PRIORITY;
            ret = pthread_attr_setschedparam(&attr, &param);
            if (ret == 0) {
                pthread_attr_setinheritsched(&attr, PTHREAD_EXPLICIT_SCHED);
            }
        }
    }
    
    // 4. Echtzeit-Thread für Client starten
    if (ret == 0) {
        ret = pthread_create(&rt_thread, &attr, client_realtime_task, client);
    }
    
    if (ret != 0) {
        printf("RT-Thread-Erstellung fehlgeschlagen: %s\n", strerror(ret));
        // Fallback: Normaler Thread
        if (pthread_create(&rt_thread, NULL, client_realtime_task, client) != 0) {
            perror("pthread_create fallback");
            const char* error_msg = "✗ Failed to start RT thread\n";
            send(client->client_socket, error_msg, strlen(error_msg), 0);
            goto cleanup;
        } else {
            printf("Normaler Thread erstellt für Client %s\n", client->client_ip);
        }
    } else {
        printf("Echtzeit-Thread erstellt für Client %s (Priorität: %d)\n", 
               client->client_ip, RT_PRIORITY);
    }
    
    // 5. Auf Thread-Beendigung warten
    pthread_join(rt_thread, NULL);
    pthread_attr_destroy(&attr);
    
cleanup:
    printf("Client %s getrennt\n", client->client_ip);
    close(client->client_socket);
    free(client);
    return NULL;
}

// ========================================
// MAIN SERVER FUNCTION
// ========================================
int main() {
    struct sockaddr_in server_addr, client_addr;
    socklen_t client_addr_len;
    int client_socket;
    pthread_t client_thread;
    int opt = 1;
    
    printf("=== SECURE REALTIME SERVER ===\n");
    printf("Port: %d\n", SERVER_PORT);
    printf("Autorisierte Client-IPs: %s, %s\n", AUTHORIZED_IP, ALTERNATIVE_IP);
    printf("Für STRG+C zum Beenden\n\n");
    
    // Signal-Handler registrieren
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);
    
    // Memory-Locking für RT-Performance
    if (mlockall(MCL_CURRENT | MCL_FUTURE) != 0) {
        printf("Warnung: Memory-Locking fehlgeschlagen: %s\n", strerror(errno));
    } else {
        printf("Memory-Locking erfolgreich aktiviert\n");
    }
    
    // 1. Socket erstellen
    server_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (server_socket < 0) {
        perror("socket");
        return EXIT_FAILURE;
    }
    
    // Socket-Optionen setzen (Adresse wiederverwenden)
    if (setsockopt(server_socket, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
        perror("setsockopt");
        close(server_socket);
        return EXIT_FAILURE;
    }
    
    // 2. Server-Adresse konfigurieren
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET; // IPv4   
    server_addr.sin_addr.s_addr = INADDR_ANY;  // Alle verfügbaren Interfaces
    server_addr.sin_port = htons(SERVER_PORT); // Port in Netzwerk-Byte-Reihenfolge
    
    // 3. Socket an Port binden
    if (bind(server_socket, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        perror("bind");
        close(server_socket);
        return EXIT_FAILURE;
    }
    printf("Server gebunden an Port %d\n", SERVER_PORT);
    
    // 4. Auf Verbindungen lauschen
    if (listen(server_socket, MAX_CLIENTS) < 0) {
        perror("listen");
        close(server_socket);
        return EXIT_FAILURE;
    }

    printf("Server lauscht auf Port %d...\n", SERVER_PORT);
    
    
    // 5. Client-Verbindungen akzeptieren
    // Hauptschleife: Akzeptiert Verbindungen solange der Server läuft (server_running == 1)
    printf("Warte auf Client-Verbindungen...\n");
    while (server_running) {
        client_addr_len = sizeof(client_addr);
        client_socket = accept(server_socket, (struct sockaddr*)&client_addr, &client_addr_len);
        
        
        if (client_socket < 0) {
            if (server_running) {  // Nur Fehler ausgeben wenn Server noch läuft
                perror("accept");
            }
            continue;
        }
        
        // Client-Info erstellen
        client_info_t* client = malloc(sizeof(client_info_t));
        if (client == NULL) {
            perror("malloc");
            close(client_socket);
            continue;
        }
        
        client->client_socket = client_socket;
        client->client_addr = client_addr;
        client->authenticated = 0;
        
        // Client-IP extrahieren
        inet_ntop(AF_INET, &(client_addr.sin_addr), client->client_ip, INET_ADDRSTRLEN);
        
        // Client-Handler-Thread starten
        if (pthread_create(&client_thread, NULL, handle_client, client) != 0) {
            perror("pthread_create client_thread");
            free(client);
            close(client_socket);
            continue;
        }
        
        // Thread detachen (automatische Ressourcen-Freigabe)
        pthread_detach(client_thread);
    }
    
    // Cleanup
    close(server_socket);
    munlockall(); // Speicher-Locking aufheben
    printf("Server beendet, alle Ressourcen freigegeben\n");
    
    return EXIT_SUCCESS;
}
