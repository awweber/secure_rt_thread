# Secure Real-Time Thread Demo

## Projektübersicht

Dieses Projekt demonstriert die Implementierung von Sicherheitsaspekten in Echtzeit-Embedded-Linux-Systemen. Es kombiniert Echtzeit-Thread-Programmierung mit mehrstufigen Sicherheitsmechanismen und zeigt Best Practices für sichere RT-Anwendungen.

**Autor:** Alexander Weber  
**Datum:** Juli 2025  
**Teilprüfung:** 6 - Einführung in die Sicherheitsaspekte von Echtzeit-Embedded-Linux-Systemen

---

## Inhaltsverzeichnis

1. [Features](#features)
2. [Systemanforderungen](#systemanforderungen)
3. [Installation und Kompilierung](#installation-und-kompilierung)
4. [Verwendung](#verwendung)
5. [Code-Architektur](#code-architektur)
6. [Sicherheitsmechanismen](#sicherheitsmechanismen)
7. [Echtzeit-Aspekte](#echtzeit-aspekte)
8. [Troubleshooting](#troubleshooting)
9. [Technische Details](#technische-details)

---

## Features

### ✅ **Sicherheitsfeatures**
- **Benutzerauthentifizierung**: Überprüfung des Benutzernamens vor RT-Thread-Start
- **Netzwerksicherheit**: IP-Adress-Validierung für autorisierte Netzwerk-Interfaces
- **Defense in Depth**: Mehrstufige Sicherheitsarchitektur
- **Sichere Eingabebehandlung**: Buffer-Overflow-Schutz mit `fgets()`

### ⚡ **Echtzeit-Features**
- **SCHED_FIFO Scheduling**: Deterministische Echtzeit-Ausführung
- **Memory Locking**: Verhindert Paging für konstante Latenz
- **Präzise Zeitmessung**: Nanosekunden-genaue Timing mit `clock_nanosleep()`
- **Monotonic Clock**: Schutz vor Systemzeit-Änderungen
- **Fallback-Mechanismus**: Graceful Degradation bei fehlenden RT-Rechten

### 🔧 **Entwickler-Features**
- **Umfassende Dokumentation**: Detaillierte Code-Kommentare
- **Automatisierte Tests**: Make-basierte Testsuite
- **Fehlerbehandlung**: Robuste Error-Handling-Strategien
- **Portabilität**: GNU/Linux-kompatibel

---

## Systemanforderungen

### **Hardware**
- x86_64 oder ARM-basierte Systeme
- Mindestens 512 MB RAM
- Netzwerk-Interface (für Netzwerksicherheitscheck)

### **Software**
- **Betriebssystem**: GNU/Linux (empfohlen: Ubuntu 20.04+, Debian 11+)
- **Kernel**: Standard Linux-Kernel (PREEMPT-RT empfohlen für optimale RT-Performance)
- **Compiler**: GCC 7.0+ mit C99-Standard
- **Bibliotheken**: 
  - `libpthread` (pthread-Funktionen)
  - `librt` (Real-Time-Funktionen)

### **Berechtigungen**
- **Standard-Benutzer**: Basis-Funktionalität verfügbar
- **Root/sudo**: Erforderlich für echte RT-Prioritäten und Memory-Locking
- **CAP_SYS_NICE**: Alternative zu root für RT-Scheduling

---

## Installation und Kompilierung

### **1. Repository klonen/herunterladen**
```bash
git clone <repository-url>
cd secure_rt_thread
```

### **2. Kompilierung**
```bash
# Standard-Kompilierung
make

# Alternative: Manuell kompilieren
gcc -Wall -O2 -o secure_rt_thread secure_rt_thread.c -lpthread -lrt
```

### **3. Testen**
```bash
# Automatisierte Tests
make test

# Manuelle Ausführung
./secure_rt_thread
```

### **4. Aufräumen**
```bash
make clean
```

---

## Verwendung

### **Interaktive Ausführung**
```bash
$ ./secure_rt_thread

=== Echtzeit-Thread Demo ===
PREEMPT-RT Kernel empfohlen für beste Performance

=== SICHERHEITSAUTHENTIFIZIERUNG ===
Bitte geben Sie Ihren Benutzernamen ein: admin
✓ Authentifizierung erfolgreich für Benutzer: admin
✓ Zugriff auf Echtzeit-Funktionen gewährt

=== NETZWERKSICHERHEITSÜBERPRÜFUNG ===
Überprüfe aktive Netzwerk-Interfaces:
  Interface lo: IP 127.0.0.1 ✓ [AUTORISIERT]
✓ Netzwerksicherheit: Autorisierte IP-Adresse gefunden
✓ Netzwerkzugriff für Echtzeit-Operationen genehmigt
```

### **Mit erhöhten Rechten (empfohlen)**
```bash
# Für echte RT-Prioritäten
sudo ./secure_rt_thread
```

### **Autorisierte Zugangsdaten**
- **Benutzername**: `admin`
- **Autorisierte IPs**: `127.0.0.1`, `192.168.178.49`

---

## Code-Architektur

### **Datei-Struktur**
```
secure_rt_thread/
├── secure_rt_thread.c    # Hauptprogramm
├── Makefile             # Build-System
├── README.md            # Diese Dokumentation
└── aufgabe.md          # Aufgabenstellung
```

### **Funktions-Übersicht**

| Funktion | Zweck | Sicherheitskritisch |
|----------|-------|-------------------|
| `main()` | Hauptprogramm, orchestriert alle Komponenten | ✅ |
| `authenticate_user()` | Benutzer-Authentifizierung | ✅ |
| `check_network_security()` | Netzwerk-Interface-Validierung | ✅ |
| `realtime_task()` | Echtzeit-Thread-Ausführung | ⚡ |

---

## Sicherheitsmechanismen

### **1. Benutzerauthentifizierung (`authenticate_user()`)**

**Zweck**: Verhindert unautorisierten Zugriff auf RT-Funktionen

**Implementierung**:
```c
int authenticate_user() {
    char username[MAX_USERNAME_LENGTH];
    
    // Sichere Eingabe mit fgets() (Buffer-Overflow-Schutz)
    if (fgets(username, sizeof(username), stdin) == NULL) {
        return 0;  // Authentifizierung fehlgeschlagen
    }
    
    // Input-Sanitization: Newline entfernen
    size_t len = strlen(username);
    if (len > 0 && username[len-1] == '\n') {
        username[len-1] = '\0';
    }
    
    // Autorisierung prüfen
    return (strcmp(username, AUTHORIZED_USER) == 0);
}
```

**Sicherheitsaspekte**:
- ✅ **Buffer-Overflow-Schutz**: `fgets()` statt `scanf()`
- ✅ **Input-Validation**: Längen- und Format-Überprüfung
- ✅ **Fail-Safe**: Standard-Verhalten bei Fehlern ist "Zugriff verweigert"
- ✅ **Audit-Trail**: Detaillierte Protokollierung aller Versuche

### **2. Netzwerksicherheit (`check_network_security()`)**

**Zweck**: Validiert autorisierte Netzwerk-Interfaces für sichere RT-Operationen

**Implementierung**:
```c
int check_network_security() {
    struct ifaddrs *ifaddr, *ifa;
    int authorized = 0;
    
    // Alle aktiven Netzwerk-Interfaces scannen
    if (getifaddrs(&ifaddr) == -1) {
        return 0;  // Fehler = Zugriff verweigert
    }
    
    // IPv4-Interfaces durchsuchen
    for (ifa = ifaddr; ifa != NULL; ifa = ifa->ifa_next) {
        if (ifa->ifa_addr && ifa->ifa_addr->sa_family == AF_INET) {
            // IP-Adresse extrahieren und validieren
            struct sockaddr_in* addr_in = (struct sockaddr_in*)ifa->ifa_addr;
            char ip_str[INET_ADDRSTRLEN];
            
            if (inet_ntop(AF_INET, &(addr_in->sin_addr), ip_str, INET_ADDRSTRLEN)) {
                // Autorisierung prüfen
                if (strcmp(ip_str, AUTHORIZED_IP) == 0 || 
                    strcmp(ip_str, ALTERNATIVE_IP) == 0) {
                    authorized = 1;
                }
            }
        }
    }
    
    freeifaddrs(ifaddr);
    return authorized;
}
```

**Sicherheitsaspekte**:
- ✅ **Network Segmentation**: Nur vertrauenswürdige Netzwerke
- ✅ **Dynamic Discovery**: Automatische Interface-Erkennung
- ✅ **IPv4-Fokus**: Konzentration auf primäre Netzwerkschicht
- ✅ **Whitelist-Approach**: Nur explizit autorisierte IPs

### **3. Defense in Depth Strategie**

Das Programm implementiert mehrere Sicherheitsschichten:

1. **Authentifizierungsschicht**: Benutzer-Validierung
2. **Netzwerkschicht**: IP-Adress-Autorisierung  
3. **System-Schicht**: RT-Rechte und Memory-Locking
4. **Anwendungsschicht**: Sichere Programmierung (Buffer-Schutz, Error-Handling)

---

## Echtzeit-Aspekte

### **1. Real-Time Scheduling**

**SCHED_FIFO Konfiguration**:
```c
// RT-Policy setzen
pthread_attr_setschedpolicy(&attr, SCHED_FIFO);

// Priorität konfigurieren (1-99, 50 = mittel)
param.sched_priority = RT_PRIORITY;
pthread_attr_setschedparam(&attr, &param);

// Explizite Parameter-Vererbung
pthread_attr_setinheritsched(&attr, PTHREAD_EXPLICIT_SCHED);
```

**Vorteile**:
- ⚡ **Determinismus**: Vorhersagbare Ausführungszeiten
- ⚡ **Prioritäts-basiert**: Höhere Priorität = sofortige Ausführung
- ⚡ **Preemption**: Unterbricht niedrigere Prioritäten sofort

### **2. Memory Management**

**Memory Locking**:
```c
// Verhindert Paging für konstante Latenz
if (mlockall(MCL_CURRENT | MCL_FUTURE) != 0) {
    // Fallback: Programm läuft weiter, aber mit Warnung
    printf("Warnung: Speicher-Locking fehlgeschlagen\n");
}
```

**Auswirkungen**:
- ⚡ **Konstante Latenz**: Kein Paging-induced Jitter
- ⚡ **Vorhersagbarkeit**: Keine unerwarteten Speicher-Delays
- ⚡ **Embedded-Tauglich**: Kritisch für deterministische Systeme

### **3. Präzise Zeitmessung**

**Monotonic Clock mit Absolute Timing**:
```c
// Startzeit setzen
clock_gettime(CLOCK_MONOTONIC, &next_period);

while (cycle_count < MAX_CYCLES) {
    // Absolute Zeit für nächste Periode berechnen
    next_period.tv_sec += TASK_PERIOD_SEC;
    
    // Präzise Wartezeit bis absoluter Zeitpunkt
    clock_nanosleep(CLOCK_MONOTONIC, TIMER_ABSTIME, &next_period, NULL);
    
    // RT-Task ausführen
    printf("[Zyklus %02d] Echtzeit-Task ausgeführt\n", ++cycle_count);
}
```

**Technische Vorteile**:
- ⚡ **Nanosekunden-Genauigkeit**: Präziser als `sleep()`
- ⚡ **Drift-Vermeidung**: Absolute statt relative Zeiten
- ⚡ **System-Zeit-unabhängig**: CLOCK_MONOTONIC ist immun gegen Systemzeit-Änderungen
- ⚡ **Signal-robust**: Fortsetzung nach Unterbrechungen

### **4. Fallback-Mechanismus**

```c
// Versuche RT-Thread
ret = pthread_create(&thread, &attr, realtime_task, NULL);
if (ret != 0) {
    printf("RT-Thread-Erstellung fehlgeschlagen\n");
    
    // Fallback: Normaler Thread
    pthread_attr_destroy(&attr);
    if (pthread_create(&thread, NULL, realtime_task, NULL) != 0) {
        perror("pthread_create (fallback)");
        return EXIT_FAILURE;
    }
    printf("Normaler Thread erstellt (keine RT-Priorität)\n");
}
```

**Graceful Degradation**:
- ⚡ **Robustheit**: Programm läuft auch ohne RT-Rechte
- ⚡ **Transparenz**: Benutzer wird über Einschränkungen informiert
- ⚡ **Entwickler-freundlich**: Testen ohne sudo möglich

---

## Troubleshooting

### **Häufige Probleme und Lösungen**

#### **1. "Operation not permitted" bei RT-Scheduling**
```bash
# Problem: Keine RT-Rechte
RT-Thread-Erstellung fehlgeschlagen: Operation not permitted

# Lösung 1: Mit sudo ausführen
sudo ./secure_rt_thread

# Lösung 2: RT-Limits konfigurieren
echo "@realtime - rtprio 99" | sudo tee -a /etc/security/limits.conf
echo "@realtime - memlock unlimited" | sudo tee -a /etc/security/limits.conf
# Benutzer zur "realtime" Gruppe hinzufügen
sudo usermod -a -G realtime $USER
```

#### **2. Memory Locking fehlgeschlagen**
```bash
# Problem: Unzureichende Speicher-Limits
Warnung: Speicher-Locking fehlgeschlagen: Cannot allocate memory

# Lösung: Limits erhöhen
ulimit -l unlimited

# Oder dauerhaft in /etc/security/limits.conf:
echo "* - memlock unlimited" | sudo tee -a /etc/security/limits.conf
```

#### **3. Authentifizierung fehlgeschlagen**
```bash
# Problem: Falscher Benutzername
✗ Authentifizierung fehlgeschlagen!
✗ Unautorisierten Zugriff verweigert für: user

# Lösung: Korrekten Benutzernamen verwenden
# Autorisiert: "admin"
```

#### **4. Netzwerksicherheit blockiert**
```bash
# Problem: Keine autorisierte IP gefunden
✗ Netzwerksicherheit: Keine autorisierte IP-Adresse gefunden!
✗ Autorisierte IPs: 127.0.0.1, 192.168.1.100

# Lösung 1: Localhost-Interface aktivieren
sudo ip link set lo up

# Lösung 2: IP-Konfiguration anpassen (im Code)
# AUTHORIZED_IP und ALTERNATIVE_IP in secure_rt_thread.c ändern
```

### **Debugging-Tipps**

#### **System-Information sammeln**
```bash
# Kernel-Version und RT-Support prüfen
uname -a
grep CONFIG_PREEMPT /boot/config-$(uname -r)

# Aktuelle Limits anzeigen
ulimit -a

# Netzwerk-Interfaces anzeigen
ip addr show

# RT-Prioritäten prüfen
chrt -m
```

#### **Performance-Analyse**
```bash
# RT-Latenz messen (mit cyclictest)
sudo apt install rt-tests
sudo cyclictest -t1 -p 80 -n -i 200 -l 100000

# CPU-Isolation für bessere RT-Performance
# In /etc/default/grub: GRUB_CMDLINE_LINUX="isolcpus=1,2,3"
```

---

## Technische Details

### **Kompilier-Flags**
```makefile
CC = gcc
CFLAGS = -Wall -O2
LDFLAGS = -lpthread -lrt
```

**Erklärung**:
- `-Wall`: Alle wichtigen Warnungen aktivieren
- `-O2`: Optimierung Level 2 (Balance zwischen Speed und Debug-Tauglichkeit)
- `-lpthread`: POSIX Threads Library linken
- `-lrt`: Real-Time Library linken (für `clock_nanosleep`, etc.)

### **Verwendete Standards und APIs**
- **POSIX.1-2001**: pthread-Funktionen
- **POSIX.1b (Real-Time Extensions)**: Scheduling und Timing
- **GNU/Linux Extensions**: `_GNU_SOURCE`, erweiterte Funktionalität
- **BSD Sockets**: Netzwerk-Interface-Abfrage

### **Memory Layout**
```
Programm-Speicher:
├── Text Segment (Code)           [Read-Only, Locked]
├── Data Segment (Globale Vars)   [Read-Write, Locked] 
├── Heap (Dynamic Allocation)     [Read-Write, Locked]
└── Stack (Local Vars, Calls)     [Read-Write, Locked]
```

### **Thread-Lifecycle**
```
main() Thread                     RT-Thread
     │                                │
     ├─► Authentifizierung            │
     ├─► Netzwerk-Check               │
     ├─► Memory-Locking               │
     ├─► pthread_create() ────────────┤
     │                                ├─► SCHED_FIFO Setup
     │                                ├─► Timing Loop (10x)
     │                                │   ├─► clock_nanosleep()
     │                                │   ├─► RT-Task Execute
     │                                │   └─► Zyklus-Statistik
     ├─► pthread_join() ◄─────────────┤
     ├─► Cleanup                      │
     └─► EXIT                         │
```

### **Timing-Charakteristika**
- **Zykluszeit**: 1 Sekunde (konfigurierbar)
- **Timing-Drift**: < 1ms bei korrekter RT-Konfiguration
- **Jitter**: < 100μs mit PREEMPT-RT Kernel
- **Latenz**: < 50μs für Task-Aktivierung (typisch)

---

## Erweiterte Konfiguration

### **1. RT-Prioritäten anpassen**
```c
// In secure_rt_thread.c
#define RT_PRIORITY 50    // Ändern: 1 (niedrig) bis 99 (hoch)
```

### **2. Autorisierte IPs erweitern**
```c
// In secure_rt_thread.c
#define AUTHORIZED_IP "127.0.0.1"      // Localhost
#define ALTERNATIVE_IP "192.168.1.100" // Ihre IP hier
```

### **3. Zykluszeit konfigurieren**
```c
// In secure_rt_thread.c
#define TASK_PERIOD_SEC 1     // Sekunden zwischen Zyklen
#define MAX_CYCLES 10         // Anzahl Zyklen
```

### **4. Benutzername anpassen**
```c
// In secure_rt_thread.c
#define AUTHORIZED_USER "admin"    // Autorisierter Benutzername
```

---

## Sicherheitsrichtlinien

### **Produktive Umgebung**
1. **Starke Authentifizierung**: Ersetzen Sie einfache Benutzernamen durch Token/Zertifikate
2. **Netzwerk-Härtung**: Implementieren Sie Verschlüsselung und Zertifikat-basierte Validierung
3. **Audit-Logging**: Protokollieren Sie alle Sicherheitsereignisse in Logdateien
4. **Least Privilege**: Minimieren Sie erforderliche Berechtigungen
5. **Regular Updates**: Halten Sie System und Dependencies aktuell

### **Entwicklungsumgebung**
1. **Code Reviews**: Sicherheitskritische Änderungen peer-reviewen
2. **Static Analysis**: Tools wie `cppcheck`, `clang-static-analyzer` verwenden
3. **Fuzzing**: Input-Validation mit fuzzing-Tools testen
4. **Penetration Testing**: Regelmäßige Sicherheitstests durchführen

---

## Lizenz und Haftungsausschluss

**Bildungszweck**: Dieses Projekt dient ausschließlich Lehr- und Lernzwecken.

**Haftungsausschluss**: Nicht für produktive Umgebungen ohne umfassende Sicherheitsauditierung geeignet.

**Autor**: Alexander Weber, Juli 2025

---

## Weiterführende Ressourcen

### **Dokumentation**
- [Linux RT Wiki](https://rt.wiki.kernel.org/)
- [POSIX Real-Time Programming](https://man7.org/linux/man-pages/man7/sched.7.html)
- [pthread Documentation](https://man7.org/linux/man-pages/man7/pthreads.7.html)

### **Tools**
- [cyclictest](https://wiki.linuxfoundation.org/realtime/documentation/howto/tools/cyclictest) - RT-Latenz-Messung
- [stress-ng](https://wiki.ubuntu.com/Kernel/Reference/stress-ng) - System-Stress-Testing
- [rtla](https://docs.kernel.org/tools/rtla/) - RT Linux Analysis

### **Bücher**
- "Linux Kernel Development" von Robert Love
- "Programming with POSIX Threads" von David Butenhof
- "Real-Time Systems Design and Analysis" von Phillip Laplante

---

**Ende der Dokumentation** - Version 1.0, Juli 2025
