/* Teilprüfung 55: Einführung in die Sicherheitsaspekte von Echtzeit-Embedded-Linux-Systemen
=====================================================================================================
Projekt: Sicherheitsaspekte von Echtzeit-Embedded-Linux-Systemen

Autor: Alexander Weber
Datum: Juli 2025

Dieses Programm erstellt einen einfachen Thread, der kontinuierlich eine Nachricht ausgibt.

=====================================================================================================*/

#define _GNU_SOURCE           // Aktiviert GNU-spezifische Funktionen

#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <sched.h>        // Für Echtzeit-Scheduling (SCHED_FIFO, etc.)
#include <time.h>         // Für präzise Zeitmessung (clock_nanosleep, etc.)
#include <errno.h>        // Für Fehlerbehandlung
#include <string.h>       // Für strerror()
#include <sys/mman.h>     // Für Memory-Locking (mlockall)
#include <sys/socket.h>   // Für Socket-Funktionen
#include <netinet/in.h>   // Für IP-Adressen Strukturen
#include <arpa/inet.h>    // Für inet_addr(), inet_ntoa()
#include <ifaddrs.h>      // Für getifaddrs() - Interface-Adressen

// Echtzeit-Konstanten
#define RT_PRIORITY 50        // Echtzeit-Priorität (1-99 für SCHED_FIFO)
                              // 50 = Mittlerer Bereich, nicht zu aggressiv
#define TASK_PERIOD_SEC 1     // Periode in Sekunden zwischen Task-Ausführungen
#define MAX_CYCLES 10         // Maximale Anzahl Zyklen für Demo (begrenzt Laufzeit)

// Sicherheitskonstanten
#define MAX_USERNAME_LENGTH 50
#define AUTHORIZED_USER "admin"    // Autorisierter Benutzername

// Netzwerksicherheitskonstanten
#define AUTHORIZED_IP "127.0.0.1"     // Autorisierte IP-Adresse (localhost)
#define ALTERNATIVE_IP "192.168.178.49" // Alternative autorisierte IP

// ========================================
// AUTHENTIFIZIERUNGSFUNKTION
// ========================================
int authenticate_user() {
    char username[MAX_USERNAME_LENGTH];
    
    printf("=== SICHERHEITSAUTHENTIFIZIERUNG ===\n");
    printf("Bitte geben Sie Ihren Benutzernamen ein: ");
    fflush(stdout);  // Stelle sicher, dass die Ausgabe sofort erscheint
    
    // Sichere Eingabe des Benutzernamens
    if (fgets(username, sizeof(username), stdin) == NULL) {
        printf("Fehler beim Lesen der Eingabe\n");
        return 0;  // Authentifizierung fehlgeschlagen
    }
    
    // Entferne newline-Zeichen am Ende, falls vorhanden
    size_t len = strlen(username);
    if (len > 0 && username[len-1] == '\n') {
        username[len-1] = '\0';
    }
    
    // Überprüfe Benutzername
    if (strcmp(username, AUTHORIZED_USER) == 0) {
        printf("✓ Authentifizierung erfolgreich für Benutzer: %s\n", username);
        printf("✓ Zugriff auf Echtzeit-Funktionen gewährt\n\n");
        return 1;  // Authentifizierung erfolgreich
    } else {
        printf("✗ Authentifizierung fehlgeschlagen!\n");
        printf("✗ Unautorisierten Zugriff verweigert für: %s\n", username);
        printf("✗ Nur autorisierte Benutzer dürfen Echtzeit-Threads starten\n");
        return 0;  // Authentifizierung fehlgeschlagen
    }
}

// ========================================
// NETZWERKSICHERHEITSFUNKTION
// ========================================
int check_network_security() {
    struct ifaddrs *ifaddr, *ifa;
    int authorized = 0;
    char ip_str[INET_ADDRSTRLEN];
    
    printf("=== NETZWERKSICHERHEITSÜBERPRÜFUNG ===\n");
    
    // Alle Netzwerk-Interfaces abrufen
    if (getifaddrs(&ifaddr) == -1) {
        perror("getifaddrs");
        return 0;
    }
    
    printf("Überprüfe aktive Netzwerk-Interfaces:\n");
    
    // Durch alle Interfaces iterieren
    for (ifa = ifaddr; ifa != NULL; ifa = ifa->ifa_next) {
        if (ifa->ifa_addr == NULL) continue;
        
        // Nur IPv4-Adressen betrachten
        if (ifa->ifa_addr->sa_family == AF_INET) {
            struct sockaddr_in* addr_in = (struct sockaddr_in*)ifa->ifa_addr;
            
            // IP-Adresse in String umwandeln
            if (inet_ntop(AF_INET, &(addr_in->sin_addr), ip_str, INET_ADDRSTRLEN) != NULL) {
                printf("  Interface %s: IP %s", ifa->ifa_name, ip_str);
                
                // Überprüfe ob IP autorisiert ist
                if (strcmp(ip_str, AUTHORIZED_IP) == 0 || 
                    strcmp(ip_str, ALTERNATIVE_IP) == 0) {
                    printf(" ✓ [AUTORISIERT]\n");
                    authorized = 1;
                } else {
                    printf(" ⚠ [NICHT AUTORISIERT]\n");
                }
            }
        }
    }
    
    freeifaddrs(ifaddr);
    
    if (authorized) {
        printf("✓ Netzwerksicherheit: Autorisierte IP-Adresse gefunden\n");
        printf("✓ Netzwerkzugriff für Echtzeit-Operationen genehmigt\n\n");
        return 1;
    } else {
        printf("✗ Netzwerksicherheit: Keine autorisierte IP-Adresse gefunden!\n");
        printf("✗ Autorisierte IPs: %s, %s\n", AUTHORIZED_IP, ALTERNATIVE_IP);
        printf("✗ Netzwerkzugriff verweigert aus Sicherheitsgründen\n");
        return 0;
    }
}

// Echtzeit-Thread-Funktion (ersetzt simple_task)
void *realtime_task(void *arg) {
    (void)arg; // Parameter nicht verwendet, Compiler-Warning vermeiden
    struct timespec next_period, current_time;
    int cycle_count = 0;
    
    printf("Echtzeit-Thread gestartet mit Priorität %d\n", RT_PRIORITY);
    
    // === TIMING-INITIALISIERUNG ===
    // Aktuelle Zeit als Startpunkt setzen - CLOCK_MONOTONIC ist wichtig:
    // - Wird nicht von Systemzeit-Änderungen beeinflusst
    // - Perfekt für Echtzeit-Anwendungen mit relativen Zeitintervallen
    if (clock_gettime(CLOCK_MONOTONIC, &next_period) != 0) {
        perror("clock_gettime");
        return NULL;
    }
    
    // === HAUPTSCHLEIFE ===
    // Führt MAX_CYCLES Zyklen aus, jeder genau TASK_PERIOD_SEC Sekunden nach dem vorherigen
    while (cycle_count < MAX_CYCLES) {
        // === PERIODE BERECHNEN ===
        // Nächste Periode berechnen (1 Sekunde später)
        // Wichtig: Wir addieren zur ABSOLUTEN Zeit, nicht zur aktuellen Zeit
        // Dies verhindert "Timing-Drift" - Akkumulation kleiner Verzögerungen
        next_period.tv_sec += TASK_PERIOD_SEC;
        
        // === PRÄZISE WARTEZEIT ===
        // clock_nanosleep() mit TIMER_ABSTIME wartet bis zu einem absoluten Zeitpunkt
        // Vorteile gegenüber sleep(1):
        // - Nanosekunden-Genauigkeit statt Sekunden
        // - Absoluter Zeitpunkt verhindert Drift
        // - Unterbrechungen durch Signale können behandelt werden
        if (clock_nanosleep(CLOCK_MONOTONIC, TIMER_ABSTIME, &next_period, NULL) != 0) {
            if (errno != EINTR) {  // EINTR = Unterbrochen durch Signal (OK)
                perror("clock_nanosleep");
                break;
            }
        }
        
        // === ZEITMESSUNG FÜR ANALYSE ===
        // Aktuelle Zeit messen um die tatsächliche Ausführungszeit zu dokumentieren
        clock_gettime(CLOCK_MONOTONIC, &current_time);
        
        // === ECHTZEIT-TASK AUSFÜHREN ===
        // In einer echten Anwendung würde hier die kritische Echtzeit-Arbeit stattfinden
        // z.B.: Sensordaten lesen, Aktoren steuern, Kommunikation, etc.
        printf("[Zyklus %02d] Echtzeit-Task ausgeführt um %ld.%03ld\n", 
               ++cycle_count, 
               current_time.tv_sec, 
               current_time.tv_nsec / 1000000);  // Nanosekunden zu Millisekunden
        
        // === DETERMINISTISCHE ARBEITSLAST SIMULIEREN ===
        // volatile verhindert Compiler-Optimierung (Loop würde sonst wegoptimiert)
        // In Echtzeit-Systemen ist wichtig, dass Arbeitslasten vorhersagbare Dauer haben
        for (volatile int i = 0; i < 100000; i++);
    }
    
    printf("Echtzeit-Thread beendet nach %d Zyklen\n", cycle_count);
    return NULL;
}

int main() {
    pthread_t thread;
    pthread_attr_t attr;
    struct sched_param param;
    int ret;
    
    printf("=== Echtzeit-Thread Demo ===\n");
    printf("PREEMPT-RT Kernel empfohlen für beste Performance\n\n");
    
    // ========================================
    // SCHRITT 0: BENUTZERAUTHENTIFIZIERUNG
    // ========================================
    // Sicherheitscheck: Nur autorisierte Benutzer dürfen RT-Threads starten
    // In produktiven Systemen kritisch für Systemsicherheit und -stabilität
    if (!authenticate_user()) {
        printf("\n=== SICHERHEITSVERLETZUNG ===\n");
        printf("Programmausführung aus Sicherheitsgründen beendet.\n");
        printf("Kontaktieren Sie den Systemadministrator für Zugriff.\n");
        return EXIT_FAILURE;
    }
    
    // ========================================
    // SCHRITT 0.5: NETZWERKSICHERHEITSÜBERPRÜFUNG
    // ========================================
    // Überprüfe ob System über autorisierte Netzwerk-Interfaces verfügt
    // Wichtig für networked embedded systems und remote RT-Operationen
    if (!check_network_security()) {
        printf("\n=== NETZWERKSICHERHEITSVERLETZUNG ===\n");
        printf("System läuft nicht auf autorisierten Netzwerk-Interfaces.\n");
        printf("Echtzeit-Operationen aus Sicherheitsgründen verweigert.\n");
        printf("Kontaktieren Sie den Netzwerkadministrator.\n");
        return EXIT_FAILURE;
    }
    
    printf("=== ECHTZEIT-INITIALISIERUNG ===\n");
    
    // ========================================
    // SCHRITT 1: SPEICHER-LOCKING
    // ========================================
    // mlockall() verhindert, dass unser Speicher auf Festplatte ausgelagert wird (Paging)
    // MCL_CURRENT: Bereits allokierter Speicher wird gesperrt
    // MCL_FUTURE: Zukünftig allokierter Speicher wird ebenfalls gesperrt
    // Kritisch für Echtzeit: Paging kann zu unvorhersagbaren Verzögerungen führen
    if (mlockall(MCL_CURRENT | MCL_FUTURE) != 0) {
        printf("Warnung: Speicher-Locking fehlgeschlagen: %s\n", strerror(errno));
        printf("Läuft weiter, aber ohne optimale RT-Performance\n");
        // Hinweis: Benötigt oft root-Rechte oder spezielle Limits-Konfiguration
    } else {
        printf("Speicher erfolgreich gesperrt\n");
    }
    
    // ========================================
    // SCHRITT 2: THREAD-ATTRIBUTE INITIALISIEREN
    // ========================================
    // pthread_attr_t Struktur enthält alle Thread-Konfigurationsparameter
    ret = pthread_attr_init(&attr);
    if (ret != 0) {
        printf("Fehler bei pthread_attr_init: %s\n", strerror(ret));
        return EXIT_FAILURE;
    }
    
    // ========================================
    // SCHRITT 3: ECHTZEIT-SCHEDULING KONFIGURIEREN
    // ========================================
    // Versuche SCHED_FIFO zu setzen - das Herzstück der Echtzeit-Funktionalität
    // SCHED_FIFO = First-In-First-Out Echtzeit-Scheduling:
    // - Thread läuft bis er freiwillig aufgibt oder von höherer Priorität unterbrochen wird
    // - Keine Zeitscheiben wie bei normalem Scheduling (CFS)
    // - Deterministische, vorhersagbare Ausführung
    ret = pthread_attr_setschedpolicy(&attr, SCHED_FIFO);
    if (ret != 0) {
        printf("Warnung: SCHED_FIFO nicht verfügbar: %s\n", strerror(ret));
        printf("Verwende Standard-Scheduling (für RT-Rechte: sudo ausführen)\n");
        // Fallback: Programm läuft weiter, aber ohne echte RT-Garantien
    } else {
        // === PRIORITÄT SETZEN ===
        // SCHED_FIFO Prioritäten: 1 (niedrigste) bis 99 (höchste)
        // Wichtig: Nicht zu hoch wählen, um System nicht zu blockieren
        // 50 ist ein guter Mittelwert für Anwendungen
        param.sched_priority = RT_PRIORITY;
        ret = pthread_attr_setschedparam(&attr, &param);
        if (ret != 0) {
            printf("Fehler bei Priorität setzen: %s\n", strerror(ret));
            pthread_attr_destroy(&attr);
            return EXIT_FAILURE;
        }
        
        // === EXPLIZITE SCHEDULING-PARAMETER ===
        // PTHREAD_EXPLICIT_SCHED: Verwende unsere eigenen Scheduling-Parameter
        // (Alternative: PTHREAD_INHERIT_SCHED würde Parent-Parameter übernehmen)
        ret = pthread_attr_setinheritsched(&attr, PTHREAD_EXPLICIT_SCHED);
        if (ret != 0) {
            printf("Fehler bei setinheritsched: %s\n", strerror(ret));
            pthread_attr_destroy(&attr);
            return EXIT_FAILURE;
        }
        
        printf("Echtzeit-Scheduling (SCHED_FIFO) aktiviert, Priorität: %d\n", RT_PRIORITY);
    }
    
    // ========================================
    // SCHRITT 4: THREAD ERSTELLEN
    // ========================================
    // Versuche zuerst Echtzeit-Thread mit konfigurierten Attributen zu erstellen
    ret = pthread_create(&thread, &attr, realtime_task, NULL);
    if (ret != 0) {
        printf("RT-Thread-Erstellung fehlgeschlagen: %s\n", strerror(ret));
        
        // === FALLBACK-MECHANISMUS ===
        // Wenn RT-Thread fehlschlägt (meist wegen fehlender Rechte), 
        // erstelle normalen Thread mit denselben Timing-Funktionen
        printf("Versuche normalen Thread als Fallback...\n");
        pthread_attr_destroy(&attr);  // RT-Attribute nicht mehr benötigt
        
        // Normaler Thread ohne spezielle Attribute
        if (pthread_create(&thread, NULL, realtime_task, NULL) != 0) {
            perror("pthread_create (fallback)");
            return EXIT_FAILURE;
        }
        printf("Normaler Thread erstellt (keine RT-Priorität)\n");
        // Hinweis: Timing-Funktionen funktionieren trotzdem, aber ohne RT-Garantien
    } else {
        printf("Echtzeit-Thread erfolgreich erstellt\n");
    }
    
    printf("Thread erstellt, warte auf Beendigung...\n\n");
    
    // ========================================
    // SCHRITT 5: THREAD-SYNCHRONISATION
    // ========================================
    // pthread_join() wartet bis Thread beendet ist
    // Wichtig für saubere Programmbeendigung
    pthread_join(thread, NULL);
    
    // ========================================
    // SCHRITT 6: RESSOURCEN FREIGEBEN
    // ========================================
    pthread_attr_destroy(&attr);  // Thread-Attribute freigeben
    munlockall();                  // Speicher-Locking aufheben
    
    printf("\nProgramm erfolgreich beendet\n");
    return EXIT_SUCCESS;
}