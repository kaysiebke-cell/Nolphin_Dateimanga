# Nolphin

**Ein nativer Linux-Dateimanager für Linux Mint Cinnamon – als eigenständiger Nemo-Fork mit erweiterten Datei-, Vorschau-, CAD-, Git- und Sicherheitsfunktionen.**

Nolphin ist ein eigenständiger Dateimanager für die Cinnamon-Desktopumgebung. Das Projekt basiert ursprünglich auf Nemo und wurde anschließend vollständig in **Nolphin** umbenannt und schrittweise um eigene Funktionen erweitert.

Der Schwerpunkt liegt auf einem leistungsfähigen Dateimanager, der klassische Dateiverwaltung mit **Dateivorschau, technischen Metadaten, CAD-Dateierkennung, Git-Unterstützung, Prüfsummen und weiteren Werkzeugen** verbindet.

> **Hinweis:** Nolphin ist ein unabhängiges Open-Source-Projekt und kein offizielles Projekt von Linux Mint.

---

## Warum Nolphin?

Klassische Dateimanager sind hervorragend für alltägliche Dateioperationen geeignet. Bei technischen Dateien, Entwicklungsprojekten und größeren Dateisammlungen benötigt man jedoch häufig zusätzliche Werkzeuge.

Nolphin versucht, einige dieser Funktionen direkt in den Dateimanager zu integrieren.

Statt beispielsweise für verschiedene Aufgaben zwischen Dateimanager, Terminal und zusätzlichen Programmen zu wechseln, sollen wichtige Informationen und Aktionen direkt in Nolphin verfügbar sein.

### Schwerpunkte

* Dateivorschau und technische Informationen
* 3D-/CAD-Dateierkennung
* Git-Integration
* Prüfsummen
* Verschlüsselungsfunktionen
* ACL-Verwaltung
* automatisierte Papierkorbbereinigung
* gespeicherte Dateiauswahlen
* Erstellen von .deb-Paketen aus der Dateiauswahl
* klassische Nemo-Dateiverwaltung

---

# Funktionen

## Dateiverwaltung

Die bewährten Funktionen aus Nemo bleiben erhalten.

Unter anderem:

* Tabs
* geteilte Ansicht / Split View
* integriertes Terminal über VTE
* Symbolansicht
* Listenansicht
* Kompaktansicht
* Galerieansicht mit großen Vorschaubildern
* Kopieren und Verschieben
* Fortschrittsanzeige
* Pause / Fortsetzen von Dateioperationen
* Abbrechen von Dateioperationen
* Rückgängig / Wiederholen
* Papierkorb
* Drag & Drop
* Eigenschaften-Dialog
* Dateiberechtigungen
* Netzwerkzugriff über GVfs

Unterstützte Netzwerkprotokolle umfassen unter anderem:

* SMB
* NFS
* HTTP
* HTTPS

---

# Vorschau- und Info-Panel

Eine der zentralen Erweiterungen von Nolphin ist das integrierte **Vorschau-/Info-Panel**.

Mit **F11** kann das Panel geöffnet werden.

Bei der Auswahl einer Datei können dort abhängig vom Dateityp Informationen wie:

* Vorschau
* Dateiinformationen
* technische Metadaten
* Textinhalt
* Dateiformat
* weitere formatabhängige Informationen

angezeigt werden.

Das Ziel ist, Informationen über eine Datei möglichst direkt im Dateimanager bereitzustellen.

---

# 3D- und CAD-Dateien

Nolphin enthält eine spezielle Erkennung für technische und CAD-Dateien.

## STL

STL-Dateien werden erkannt und analysiert.

Nolphin kann unter anderem unterscheiden zwischen:

* ASCII-STL
* Binär-STL

Bei Binär-STL kann beispielsweise die Anzahl der Dreiecke ausgelesen werden.

---

## STEP / STP

Bei STEP-Dateien werden Header-Informationen ausgewertet.

Unter anderem können folgende Informationen vorhanden sein:

* Beschreibung
* Dateiname
* Zeitstempel
* Autor
* Schema

---

## FreeCAD FCStd

FreeCAD-Dateien (`.FCStd`) werden ebenfalls erkannt.

Nolphin kann vorhandene Dokumentinformationen auswerten, beispielsweise:

* Kommentar
* Autor
* Firma
* Erstellungsdatum
* Änderungsdatum

Falls ein Vorschaubild innerhalb des FreeCAD-Dokuments vorhanden ist, kann dieses ebenfalls verwendet werden.

---

## Weitere technische Dateiformate

Weitere Formate werden erkannt, darunter:

* IGES
* OBJ
* 3MF
* DXF
* DWG

Wenn für ein Format noch kein echtes Vorschau-Backend vorhanden ist, zeigt Nolphin dies ausdrücklich an, anstatt eine nicht vorhandene Vorschau vorzutäuschen.

---

# Git-Integration

Nolphin integriert wichtige Git-Funktionen direkt in den Dateimanager.

Dadurch können Git-Aktionen über das Kontextmenü ausgeführt werden, ohne für jede Operation zuerst ein Terminal öffnen zu müssen.

Unterstützt werden unter anderem:

* Status
* Add
* Commit
* Pull
* Push
* Log
* Diff
* Remote hinzufügen

Damit eignet sich Nolphin insbesondere auch für die Arbeit mit lokalen Entwicklungsprojekten.

---

# Prüfsummen

Nolphin unterstützt verschiedene Prüfsummenverfahren.

Aktuell vorgesehen bzw. implementiert sind:

* MD5
* SHA-1
* SHA-256
* SHA-512
* BLAKE2

Damit können Dateien direkt aus dem Dateimanager heraus überprüft werden.

---

# Verschlüsselung

Für bestimmte Datei- und Ordneroperationen stehen Verschlüsselungsfunktionen über **GPG** zur Verfügung.

Das Ziel ist, Verschlüsselung möglichst direkt in die normale Dateiverwaltung einzubinden.

---

# ACL-Verwaltung

Nolphin unterstützt die Verwaltung von Access Control Lists (ACLs).

Dafür werden unter anderem die Linux-Werkzeuge

```text
getfacl
setfacl
```

verwendet.

Damit können detailliertere Zugriffsrechte direkt aus dem Dateimanager heraus verwaltet werden.

---

# .deb-Pakete erstellen

Über das Kontextmenü und Bearbeiten ▸ Archiv lässt sich eine Dateiauswahl direkt zu einem installierbaren Debian-Paket (`.deb`) packen.

Ein Einstellungsdialog fragt dafür die nötigen Angaben ab:

* Paketname, Version, Architektur
* Zielverzeichnis im Paket (z. B. `/opt/paketname`)
* Maintainer, Bereich (Section), Abhängigkeiten (Depends)
* Beschreibung
* Speicherort der `.deb`-Datei

Gebaut wird das Paket über das Systemwerkzeug `dpkg-deb --build --root-owner-group`, wodurch kein Root oder `fakeroot` nötig ist. Ohne installiertes `dpkg-deb` bleibt der Menüeintrag ausgegraut.

---

# Papierkorb-Automatik

Nolphin kann den Papierkorb automatisch verwalten.

Vorgesehen sind unter anderem:

* automatische Bereinigung nach einer konfigurierbaren Aufbewahrungsdauer
* Warnung bei Überschreitung eines Größenlimits

Damit kann verhindert werden, dass sich über längere Zeit große Mengen gelöschter Dateien ansammeln.

---

# Gespeicherte Dateiauswahl

Nolphin kann benannte Dateiauswahlen speichern.

Eine Auswahl kann später wiederhergestellt werden.

Das ist insbesondere bei Ordnern mit vielen Dateien interessant, wenn regelmäßig mit denselben Dateien gearbeitet wird.

---

# Eigene Symbole

Nolphin verwendet eigene, in die Anwendung eingebettete Symbole.

Dadurch ist für diese Symbole kein externes Icon-Theme erforderlich.

---

# Entwicklungsstand

Nolphin befindet sich in aktiver Entwicklung.

Ein Teil der geplanten Funktionen ist bereits implementiert, während weitere Funktionen noch entwickelt werden.

## Bereits vorhanden

* [x] Dateiverwaltung auf Nemo-Basis
* [x] Tabs
* [x] Split View
* [x] integriertes Terminal
* [x] verschiedene Ansichten
* [x] Vorschau-/Info-Panel
* [x] STL-Erkennung
* [x] STEP/STP-Erkennung
* [x] FreeCAD-FCStd-Unterstützung
* [x] Erkennung weiterer CAD-/3D-Formate
* [x] Git-Integration
* [x] Prüfsummen
* [x] GPG-Unterstützung
* [x] ACL-Verwaltung
* [x] Papierkorb-Automatik
* [x] gespeicherte Dateiauswahlen
* [x] eigene eingebettete Symbole
* [x] Auswahl als .deb-Paket erstellen

---

# Roadmap

Folgende Funktionen sind geplant bzw. befinden sich noch in der Entwicklung.

## Suche

* [ ] Erweiterte Suchoperatoren
* [ ] UND
* [ ] ODER
* [ ] NICHT
* [ ] kombinierbare Filter

Die grundlegende Filterung ist bereits vorhanden.

---

## Erweiterte Vorschau

* [ ] PDF-Vorschau
* [ ] Video-Informationen
* [ ] Audio-Informationen

Für die geplante PDF- und Medienunterstützung sollen unter anderem Poppler-GLib bzw. GStreamer verwendet werden.

---

## Metadaten und Tags

* [ ] Tags
* [ ] Bewertungen
* [ ] Kommentare
* [ ] eigenes Metadaten-Bedienfeld

---

## Arbeitsbereiche

* [ ] Tabs speichern
* [ ] Split-View-Zustand speichern
* [ ] Layout speichern
* [ ] Terminalzustand speichern
* [ ] Arbeitsbereiche benennen
* [ ] Arbeitsbereiche wiederherstellen

---

## Massenumbenennung

Geplant ist ein eigener Dialog für umfangreiche Umbenennungen.

Geplante Funktionen:

* [ ] Suchen und Ersetzen
* [ ] Nummerierung
* [ ] Vorschau der Änderungen
* [ ] mehrere Dateien gleichzeitig umbenennen

---

## Automatische Regeln

Geplant ist ein Regelsystem für automatische Dateioperationen.

Beispielsweise:

```text
Wenn Dateityp = PDF
und Größe > 100 MB
→ Aktion vorbereiten
```

Mögliche Kriterien:

* Dateityp
* Dateiname
* Dateigröße
* Datum

Vor der eigentlichen Ausführung soll eine Vorschau der Aktionen angezeigt werden.

---

## Synchronisation

Geplant ist eine Integration von `rsync`.

Vorgesehen sind:

* [ ] Ordner vergleichen
* [ ] einseitige Synchronisation
* [ ] Konflikterkennung
* [ ] Konfliktbehandlung
* [ ] Vorschau der Änderungen

---

## Versionierung

* [ ] eigene Dateiversionen speichern
* [ ] vorhandene Versionen anzeigen
* [ ] Version wiederherstellen

---

## Duplikaterkennung

* [ ] doppelte Dateien finden
* [ ] Dateien anhand von Prüfsummen vergleichen
* [ ] Duplikate übersichtlich anzeigen

---

## Diagnose

Geplant ist ein eigener Diagnosebereich.

* [ ] Diagnose-Dialog
* [ ] Fehlerbericht exportieren
* [ ] Systeminformationen anzeigen
* [ ] Konfiguration überprüfen

---

## Netzwerk

Weitere Netzwerkfunktionen sollen überprüft und gegebenenfalls erweitert werden:

* [ ] SFTP
* [ ] FTP
* [ ] WebDAV

SMB, NFS, HTTP und HTTPS sind bereits bestätigt.

---

# Technische Basis

Nolphin wird in **C11** entwickelt und verwendet unter anderem:

* GTK3
* GLib
* GIO
* GVfs
* VTE 2.91
* XApp
* cinnamon-desktop

Optionale Abhängigkeiten:

* libexif
* exempi
* dpkg-deb (für „Als .deb-Paket erstellen …“; ohne dpkg-deb ist der Menüeintrag ausgegraut)

Weitere technische Details und verbindliche Anforderungen befinden sich in:

`docs/NOLPHIN_SPEC.md`

---

# Voraussetzungen

Nolphin ist primär für:

* Linux
* Linux Mint
* Cinnamon

ausgelegt.

Für den Entwicklungsstand können je nach Distribution zusätzliche Entwicklungsbibliotheken erforderlich sein.

---

# Aus dem Quellcode bauen

## Repository klonen

```bash
git clone https://github.com/kaysiebke-cell/Nolphin_Dateimanga.git
cd Nolphin_Dateimanga
```

## Build-Verzeichnis erstellen

```bash
meson setup build
```

## Kompilieren

```bash
ninja -C build
```

## Tests ausführen

```bash
meson test -C build
```

---

# Nolphin ohne Installation starten

Für einen Testlauf kann Nolphin direkt aus dem Build-Verzeichnis gestartet werden:

```bash
env GSETTINGS_SCHEMA_DIR="$(pwd)/build/libnolphin-private" \
    LD_LIBRARY_PATH="$(pwd)/build/libnolphin-extension" \
    ./build/src/nolphin
```

Dadurch muss Nolphin nicht zunächst systemweit installiert werden.

---

# Installation

Eine systemweite Installation kann nach einem erfolgreichen Build mit dem Meson-Installationsmechanismus durchgeführt werden.

```bash
sudo ninja -C build install
```

Anschließend kann Nolphin über das System bzw. das Cinnamon-Anwendungsmenü gestartet werden.

> Die genaue Installationsmethode und Paketierung können sich während der Entwicklung noch ändern.

---

# Tests

Vor einer Installation empfiehlt es sich, die vorhandenen Tests auszuführen:

```bash
meson test -C build
```

Bei Fehlern bitte möglichst folgende Informationen zusammen mit einem Bugreport angeben:

* Linux-Mint-Version
* Cinnamon-Version
* Nolphin-Version bzw. Commit
* verwendete Hardware, falls relevant
* genaue Fehlermeldung
* Schritte zur Reproduktion

---

# Sprache

Die sichtbare Benutzeroberfläche von Nolphin ist derzeit **deutschsprachig**.

Menüs, Dialoge und Meldungen sind aktuell unabhängig von der Systemsprache auf Deutsch ausgelegt.

Weitere Übersetzungen sind für die Zukunft möglich.

---

# Herkunft

Nolphin begann als Quellstand von **Nemo 6.7.7**.

Seitdem wurde das Projekt schrittweise:

1. umbenannt,
2. strukturell angepasst,
3. dokumentiert,
4. um eigene Funktionen erweitert,
5. um zusätzliche Datei- und Metadatenfunktionen ergänzt.

Die Entwicklung ist über die Git-Historie nachvollziehbar.

Die verbindliche Funktionsspezifikation befindet sich in:

```text
docs/NOLPHIN_SPEC.md
```

---

# Nolphin und Nemo

Nolphin basiert auf Nemo, verfolgt aber ein eigenes Entwicklungsziel.

| Bereich                     | Nemo                           | Nolphin |
| --------------------------- | ------------------------------ | ------- |
| Klassische Dateiverwaltung  | ✓                              | ✓       |
| Tabs                        | ✓                              | ✓       |
| Split View                  | ✓                              | ✓       |
| Integriertes Terminal       | ✓                              | ✓       |
| Vorschau-/Info-Panel        | nicht standardmäßig integriert | ✓       |
| STL-Analyse                 | –                              | ✓       |
| STEP/STP-Metadaten          | –                              | ✓       |
| FreeCAD-FCStd-Informationen | –                              | ✓       |
| Git-Aktionen im Kontextmenü | –                              | ✓       |
| Prüfsummen                  | –                              | ✓       |
| GPG-Integration             | –                              | ✓       |
| ACL-Verwaltung              | –                              | ✓       |
| Papierkorb-Automatik        | –                              | ✓       |
| Gespeicherte Dateiauswahl   | –                              | ✓       |
| Auswahl als .deb-Paket      | –                              | ✓       |

Nolphin soll dabei **nicht einfach ein optisch veränderter Dateimanager** sein. Der Fokus liegt darauf, zusätzliche Funktionen direkt in die Dateiverwaltung zu integrieren.

---

# Für wen ist Nolphin gedacht?

Nolphin richtet sich insbesondere an Anwender, die:

* Linux Mint mit Cinnamon verwenden
* einen erweiterten Dateimanager ausprobieren möchten
* häufig mit technischen Dateien arbeiten
* CAD- oder 3D-Dateien verwalten
* Git-Repositories über den Dateimanager bearbeiten möchten
* Prüfsummen direkt aus dem Dateimanager verwenden möchten
* Datei- und Ordnerberechtigungen verwalten
* zusätzliche Dateiwerkzeuge möglichst zentral verfügbar haben möchten

---

# Mitmachen

Nolphin ist ein Open-Source-Projekt und Beiträge sind willkommen.

Mögliche Beiträge sind:

* Fehlerberichte
* Verbesserungsvorschläge
* Code
* Dokumentation
* Übersetzungen
* Tests
* Verbesserung der Benutzeroberfläche
* neue Vorschau-Backends
* Unterstützung zusätzlicher Dateiformate

Wenn du einen Fehler findest oder eine Funktion vermisst, erstelle bitte ein GitHub Issue.

---

# Fehler melden

Bitte überprüfe vor dem Erstellen eines Issues, ob das Problem bereits bekannt ist.

Ein hilfreicher Bugreport sollte möglichst enthalten:

```text
Nolphin-Version:
Linux-Mint-Version:
Cinnamon-Version:

Problem:
...

Schritte zur Reproduktion:
1.
2.
3.

Erwartetes Verhalten:
...

Tatsächliches Verhalten:
...

Fehlermeldungen:
...
```

Bei Problemen mit bestimmten Dateien können – sofern sie keine vertraulichen Informationen enthalten – auch Beispieldateien oder anonymisierte Informationen hilfreich sein.

---

# Entwicklungsdokumentation

Weitere technische Informationen befinden sich im Repository.

Wichtige Dateien:

```text
docs/
├── NOLPHIN_SPEC.md
```

Weitere Bereiche:

```text
src/                    Hauptquellcode
libnolphin-extension/   Erweiterungen
libnolphin-private/     interne Komponenten
test/                   Tests
debian/                 Debian-Paketierung
data/                   Daten und Ressourcen
po/                     Übersetzungsdateien
```

---

# Lizenz

Nolphin steht unter:

**GPL-2.0-or-later**

Siehe:

```text
COPYING
```

Weitere Lizenzinformationen befinden sich in den entsprechenden Lizenzdateien des Projekts.

---

# Projekt

**Nolphin – Linux-Dateimanager für Cinnamon**

GitHub:

https://github.com/kaysiebke-cell/Nolphin_Dateimanga

---

## Status

Nolphin befindet sich in Entwicklung.

Die aktuelle Version sollte daher als Entwicklungssoftware betrachtet werden. Funktionen können sich ändern, ergänzt oder neu strukturiert werden.

Feedback, Fehlerberichte und Verbesserungsvorschläge sind willkommen.

---

**Nolphin – Dateien verwalten. Informationen sehen. Mehr direkt erledigen.**
