# Nolphin

Nolphin ist ein eigenständiger, nativer Linux-Dateimanager für Linux Mint (Cinnamon), entwickelt in C11 mit GTK3, GLib, GIO und GVfs. Das Projekt ist ein Fork von [Nemo](https://github.com/linuxmint/nemo), dem Standard-Dateimanager von Linux Mint, wurde vollständig umbenannt und um eigene Funktionen erweitert.

Die verbindliche Funktionsspezifikation und der Entwicklungsvertrag stehen in [`docs/NOLPHIN_SPEC.md`](docs/NOLPHIN_SPEC.md).

## Herkunft

Nolphin startete als unveränderter Nemo-6.7.7-Quellstand und wurde seitdem schrittweise umbenannt, dokumentiert und um neue Funktionen ergänzt. Jede Änderung ist einzeln committet und nach dem Schema REQUIREMENT/DATEI/ÄNDERUNG/WARUM/TEST dokumentiert — nachvollziehbar in der Commit-Historie.

## Funktionsumfang (Auszug)

**Von Nemo geerbt und beibehalten:**
- Tabs, geteilte Ansicht (Split View), integriertes Terminal (VTE)
- Symbol-, Listen-, Kompakt- und **Galerieansicht** (große Vorschaubilder)
- Kopieren/Verschieben mit Fortschrittsanzeige, Pause/Fortsetzen, Abbrechen
- Rückgängig/Wiederholen, Papierkorb, Drag & Drop
- Eigenschaften-Dialog, Berechtigungen, Netzwerkzugriff über GVfs (SMB, NFS, HTTP/HTTPS)

**Eigene, in diesem Fork neu implementierte Funktionen — das hat das originale Nemo nicht:**

- **Vorschau-/Info-Panel rechts (F11)**: zeigt zur ausgewählten Datei direkt Bild-Vorschau, Text-Inhalt und technische Metadaten an. Im originalen Nemo gibt es das nicht eingebaut — dort ist Vorschau höchstens über eine separate, optionale Cinnamon-Spice-Erweiterung nachrüstbar.
- **3D-/CAD-Dateierkennung** im selben Vorschau-Panel:
  - **STL**: erkennt ASCII- oder Binärformat, liest die Dreieckszahl aus
  - **STEP/STP**: liest Header-Metadaten (Beschreibung, Dateiname, Zeitstempel, Autor, Schema)
  - **FCStd** (FreeCAD): liest Dokumenteigenschaften (Kommentar, Autor, Firma, Erstellungs-/Änderungsdatum) und zeigt das im Archiv eingebettete Vorschaubild, falls vorhanden
  - Weitere Formate (IGES, OBJ, 3MF, DXF, DWG) werden zuverlässig erkannt und ehrlich als "kein Vorschau-Backend verfügbar" gemeldet, statt eine Vorschau vorzutäuschen
- **Sicherheit**: Prüfsummen (MD5, SHA-1, SHA-256, SHA-512, BLAKE2), Datei-/Ordnerverschlüsselung über gpg, ACL-Verwaltung über getfacl/setfacl
- **Git-Integration**: Status, Hinzufügen, Commit, Pull, Push, Log, Diff, Remote hinzufügen — direkt im Kontextmenü, ohne Terminal
- **Papierkorb-Automatik**: Bereinigung nach konfigurierbarer Aufbewahrungsdauer, Warnung bei Größenlimit
- **Auswahl speichern/wiederherstellen**: benannte Dateiauswahlen pro Ordner merken und später wiederherstellen
- Eigene, abhängigkeitsfreie Symbole (fest in die Anwendung eingebettet, kein externes Icon-Theme nötig)

## Geplant, aber noch nicht gebaut

Laut Entwicklungsvertrag ([`docs/NOLPHIN_SPEC.md`](docs/NOLPHIN_SPEC.md)) noch offen:

- **Erweiterte Suche**: explizite Operatoren UND/ODER/NICHT (Filterung selbst funktioniert bereits)
- **Erweiterte Vorschau**: PDF-Vorschau (Poppler-GLib) und Video-/Audio-Informationen (GStreamer) — beide Abhängigkeiten sind aktuell noch gar nicht im Build eingebunden
- **Metadaten und Tags**: eigenes Bedienfeld für Tags, Bewertung, Kommentare (GVfs-Metadaten-Zugriff besteht bereits als Grundlage)
- **Arbeitsbereiche**: Tabs, Bereiche, Layout und Terminalzustand unter einem Namen speichern und wiederherstellen
- **Massenumbenennung**: eigener Dialog mit Suchen/Ersetzen, Nummerierung und Vorschau (aktuell nur über ein extern konfigurierbares Werkzeug)
- **Regeln**: automatische Aktionen nach Dateityp/Name/Größe/Datum mit Vorschau vor Ausführung
- **Synchronisation** über rsync (Ordner vergleichen, einseitig abgleichen, Konfliktbehandlung)
- **Versionierung**: eigene Dateiversionen speichern, anzeigen, wiederherstellen
- **Duplikaterkennung**
- **Verwaltung und Diagnose**: Diagnose-Dialog, Fehlerbericht-Export, Systeminformationen
- Prüfen, ob SFTP/FTP/WebDAV tatsächlich funktionieren (SMB/NFS/HTTP/HTTPS sind bereits bestätigt)

## Bauen

```bash
meson setup build
ninja -C build
meson test -C build
```

Kernabhängigkeiten: GTK3, GLib/GIO, GVfs, VTE 2.91, XApp, cinnamon-desktop. Optional: libexif, exempi (XMP). Details siehe [`docs/NOLPHIN_SPEC.md`](docs/NOLPHIN_SPEC.md), Abschnitt 3.

Uninstalliert starten (ohne `ninja install`):

```bash
env GSETTINGS_SCHEMA_DIR="$(pwd)/build/libnolphin-private" \
    LD_LIBRARY_PATH="$(pwd)/build/libnolphin-extension" \
    ./build/src/nolphin
```

## Sprache

Nolphin ist eine deutschsprachige Anwendung — alle sichtbaren Texte (Menüs, Dialoge, Meldungen) sind fest auf Deutsch, unabhängig von der Systemsprache.

## Lizenz

GPL-2.0-or-later (geerbt von Nemo / GNOME Files / Nautilus), siehe [`COPYING`](COPYING).
