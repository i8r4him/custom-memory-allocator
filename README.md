# Custom Memory Allocator

Ein kompakter Heap-Speicher-Allocator in C11. Er verwaltet eine feste Arena mit **First-Fit**, **Block-Splitting**, **Wiederverwendung** und **Coalescing** benachbarter freier Blöcke. Eine kleine Demo zeigt die Speichernutzung vor und nach Allokationen.

Die Arena wird einmal mit dem System-`malloc` reserviert. Alle anschließenden `allocator_alloc`-/`allocator_free`-Aufrufe verwalten diese Arena selbst; `allocator_destroy` gibt sie abschließend an das Betriebssystem zurück.

## Funktionen

- Ausrichtung aller Rückgabepointer auf `_Alignof(max_align_t)`.
- First-Fit-Suche durch eine physisch geordnete Blockliste.
- Aufteilen ausreichend großer freier Blöcke.
- Zusammenführen benachbarter freier Blöcke beim Freigeben.
- Erkennung unbekannter Pointer, Innenpointer und bereits freier Blöcke.
- Definiertes Verhalten für Nullgröße, Integer-Überlauf und Speichermangel.
- Statistiken zu freien Bytes, größtem freien Block und Blockanzahl.
- Deterministischer Stresstest mit 10.000 Operationen und Prüfung von Datenintegrität und Überschneidungen.
- AddressSanitizer- und UndefinedBehaviorSanitizer-Testlauf.

## Starten

Voraussetzungen: C11-Compiler (Clang oder GCC) und Make. Keine externen Bibliotheken.

```bash
make demo
make test
make sanitize
```

`make sanitize` benötigt einen Compiler mit AddressSanitizer und UndefinedBehaviorSanitizer. Alternativ explizit Clang auswählen:

```bash
make CC=clang sanitize
```

Beispielausgabe auf macOS/ARM64; Metadatengrößen sind plattformabhängig:

```text
Initial arena          free=4072  largest=4072  free blocks=1  allocated=0
Hello from the custom allocator!
numbers[7] = 49
After two allocations  free=3832  largest=3832  free blocks=1  allocated=2
After free + coalesce  free=4072  largest=4072  free blocks=1  allocated=0
```

## Verwendung

```c
#include "allocator.h"

int main(void) {
    Allocator arena = {0};
    if (!allocator_init(&arena, 4096)) {
        return 1;
    }

    int *values = allocator_alloc(&arena, 16 * sizeof(*values));
    if (values == NULL) {
        allocator_destroy(&arena);
        return 1;
    }

    values[0] = 42;
    bool released = allocator_free(&arena, values);
    allocator_destroy(&arena);
    return released ? 0 : 1;
}
```

## Speicherlayout

```text
[Blockheader][ausgerichteter Nutzspeicher][Blockheader][Nutzspeicher] ...
     |                                      |
     +---------------- next ----------------+
```

Ein Header speichert die Nutzgröße, den Frei-/Belegt-Zustand und den nächsten Block. Headergrößen und Anforderungen werden auf die nötige Ausrichtung gerundet. Ein Block wird nur geteilt, wenn der Rest mindestens einen neuen Header und einen ausgerichteten Nutzbereich aufnehmen kann.

Beim Freigeben wird der Pointer mit bekannten Nutzbereichsadressen verglichen. Anschließend werden direkt benachbarte freie Blöcke zusammengeführt, sodass größere Allokationen wieder möglich werden.

## Tests und Grenzen

`test_allocator.c` prüft Ausrichtung, Zero-/Overflow-Fälle, ungültiges Freigeben, Wiederverwendung, Erschöpfung, Zusammenführung sowie 10.000 deterministische Operationen mit Byte-Mustern. GitHub Actions testet unter Linux und macOS mit Sanitizern.

Dieses Lernprojekt ist nicht threadsicher, bietet weder `realloc` noch `calloc` und ersetzt nicht global `malloc`. Speicher wird nicht automatisch initialisiert. Ein veralteter Pointer, dessen Adresse inzwischen neu vergeben wurde, kann wie bei üblichen manuellen Allocator-APIs nicht als veraltet erkannt werden. Nach `allocator_destroy` sind sämtliche Pointer der Arena ungültig. Die Suche nach einem freien Block und die Zusammenführung sind linear in der Anzahl der Blöcke; es werden keine Leistungsbehauptungen gegenüber dem System-Allocator gemacht.

## Dateien

| Datei | Aufgabe |
| --- | --- |
| `allocator.h` | Öffentliche API und Statistiken |
| `allocator.c` | First-Fit, Splitting und Coalescing |
| `demo.c` | Ausführbares Anwendungsbeispiel |
| `test_allocator.c` | Funktions-, Fehlerfall- und Stresstests |
| `Makefile` | Build, Demo, Tests und Sanitizer |

Lizenz: MIT.
