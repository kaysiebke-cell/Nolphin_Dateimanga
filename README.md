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

**Eigene, in diesem Fork neu implementierte Funktionen:**
- **Sicherheit**: Prüfsummen (MD5, SHA-1, SHA-256, SHA-512, BLAKE2), Datei-/Ordnerverschlüsselung über gpg, ACL-Verwaltung über getfacl/setfacl
- **Git-Integration**: Status, Hinzufügen, Commit, Pull, Push, Log, Diff, Remote hinzufügen — direkt im Kontextmenü, ohne Terminal
- **Papierkorb-Automatik**: Bereinigung nach konfigurierbarer Aufbewahrungsdauer, Warnung bei Größenlimit
- **Auswahl speichern/wiederherstellen**: benannte Dateiauswahlen pro Ordner merken und später wiederherstellen
- Eigene, abhängigkeitsfreie Symbole (fest in die Anwendung eingebettet, kein externes Icon-Theme nötig)

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
