# Aufgabe: Einführung in die Sicherheitsaspekte von Echtzeit-Embedded-Linux-Systemen

## Ziel der Aufgabe:

Erweitere den vorhandenen Code, um grundlegende Sicherheitsmechanismen wie Authentifizierung und einfache Netzwerksicherheit zu implementieren.

## Aufgabe:
1. Authentifizierung: Füge eine einfache Authentifizierungsfunktion hinzu, die überprüft, ob der Benutzername korrekt ist, bevor der Echtzeit-Thread gestartet wird.
2. Einfache Netzwerksicherheit: Füge eine Überprüfung der IP-Adresse hinzu, um sicherzustellen, dass nur Verbindungen von einer bestimmten IP-Adresse akzeptiert werden.

```c
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <sched.h>
#include <unistd.h>

void* realtime_task(void* arg) {
    struct sched_param param;
    param.sched_priority = sched_get_priority_max(SCHED_FIFO);
    if (pthread_setschedparam(pthread_self(), SCHED_FIFO, &param) != 0) {
        perror("pthread_setschedparam");
        return NULL;
    }

    while (1) {
        // Echtzeit-Operationen
        printf("Echtzeit-Task wird
ausgeführt\n");
        usleep(1000);  // 1
Millisekunde warten
    }

    return NULL;
}

int main() {
    pthread_t
thread;
    if (pthread_create(&thread, NULL, realtime_task, NULL) != 0) {
        perror("pthread_create");
        return EXIT_FAILURE;
    }

    pthread_join(thread, NULL);
    return EXIT_SUCCESS;
}
```