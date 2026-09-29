# Abhängigkeiten

Diese Datei ist laut Entwicklungsvertrag (`docs/NOLPHIN_SPEC.md`, §50.1) Pflicht und wird bei
jeder Änderung aktualisiert. Sie listet alle Bibliotheken und Werkzeuge, die Nolphin tatsächlich
verwendet - Paketnamen für Linux Mint/Ubuntu/Debian, getrennt nach Pflicht und Optional.

Stand: geprüft anhand von `meson.build`, `meson_options.txt` und aller `g_find_program_in_path()`-
Aufrufe im tatsächlichen Quellcode (nicht nur anhand der Spezifikation).

---

## Fertiger Befehl zum Installieren aller Pflicht- und Standard-Optional-Abhängigkeiten

```bash
sudo apt install \
  meson ninja-build \
  libgtk-3-dev libvte-2.91-dev libxapp-dev libcinnamon-desktop-dev \
  libgail-3-dev libx11-dev libjson-glib-dev libpango1.0-dev \
  libgirepository1.0-dev gobject-introspection gtk-doc-tools intltool itstool \
  libgsf-1-dev \
  libexif-dev libexempi-dev \
  dpkg git gnupg acl coreutils zip unzip tar p7zip-full file-roller
```

---

## 1. Pflicht (Build schlägt ohne diese fehl)

Aus `meson.build` (`dependency(...)` ohne `required: false`):

| Bibliothek | Debian/Mint-Paket | Mindestversion |
|---|---|---|
| GTK3 | `libgtk-3-dev` | ≥ 3.10.0 |
| GLib/GIO/GObject/GModule | `libglib2.0-dev` | ≥ 2.45.7 |
| GObject Introspection | `libgirepository1.0-dev`, `gobject-introspection` | ≥ 1.0 |
| JSON-GLib | `libjson-glib-dev` | ≥ 1.6 |
| cinnamon-desktop | `libcinnamon-desktop-dev` | ≥ 4.8.0 |
| GAIL | `libgail-3-dev` | - |
| X11 | `libx11-dev` | - |
| XApp | `libxapp-dev` | ≥ 2.0.0 |
| VTE (integriertes Terminal) | `libvte-2.91-dev` | ≥ 0.40 |
| Pango | `libpango1.0-dev` | ≥ 1.44.0 |
| libgsf | `libgsf-1-dev` | - |
| Build-Werkzeuge | `meson`, `ninja-build`, `gtk-doc-tools`, `intltool`, `itstool` | - |

**KONFLIKT mit §3 des Entwicklungsvertrags:** cinnamon-desktop und XApp (libxapp) sind laut
Vertrag ausdrücklich *optionale* Komponenten ("Nolphin muss ohne jede dieser Komponenten
kompilieren, starten und alle Kernfunktionen ausführen"). Im tatsächlichen `meson.build` sind
beide jedoch als **Pflichtabhängigkeiten** eingebunden (`dependency(...)` ohne
`required: false`), nicht als Meson-Feature-Option. Das ist ein bestehender Widerspruch aus
früheren Sitzungen, keine Änderung dieser Sitzung. Um ihn aufzulösen, müsste man beide Libs auf
`required: false` umstellen und den jeweils abhängigen Code bedingt kompilieren/ausblenden - das
ist ein eigener, größerer Umbau und wird hier nur gemeldet, nicht durchgeführt.

---

## 2. Optional zur Build-Zeit (Meson-Feature-Flags)

Aus `meson_options.txt`, jeweils per `-D<option>=false` abschaltbar:

| Bibliothek | Debian/Mint-Paket | Option | Standard | Funktion |
|---|---|---|---|---|
| libexif | `libexif-dev` | `exif` | an | EXIF-Daten von Bildern (§34) |
| Exempi (XMP) | `libexempi-dev` | `xmp` | an | XMP-Metadaten |
| Tracker | `libtracker-sparql-3.0-dev` (o. ä., versionsabhängig) | `tracker` | aus | Tracker-Suchindex |
| gtk-layer-shell + Wayland | `libgtk-layer-shell-dev` | `gtk_layer_shell` | aus | Desktop-Zeichnung unter Wayland |
| SELinux | `libselinux1-dev` | `selinux` | aus | SELinux-Unterstützung |

## 3. Optional zur Laufzeit (externe Werkzeuge über GSubprocess, §53.5)

Diese werden nicht gelinkt, sondern nur bei tatsächlicher Nutzung per
`g_find_program_in_path()` gesucht. Fehlt eines, wird die jeweilige Funktion laut Code
ausgegraut bzw. meldet einen Fehler - nichts wird vorgetäuscht.

| Werkzeug | Debian/Mint-Paket | Genutzt für |
|---|---|---|
| `git` | `git` | §40 Git-Integration |
| `gpg` | `gnupg` | §39 Verschlüsselung |
| `getfacl`, `setfacl` | `acl` | §39 ACL-Verwaltung |
| `b2sum` | `coreutils` (Standard bereits installiert) | §39 BLAKE2-Prüfsummen |
| `zip`, `unzip` | `zip`, `unzip` | §36 Archive (ZIP) |
| `tar` | `tar` (Standard bereits installiert) | §36 Archive (TAR/TAR.GZ/TAR.BZ2/TAR.XZ) |
| `7z` | `p7zip-full` | §36 Archive (7-Zip) |
| `dpkg-deb` | `dpkg` (Standard bereits installiert) | „Als .deb-Paket erstellen …" (neue Funktion, nicht im Vertrag) |
| `file-roller` | `file-roller` | Nur als Öffnen-Helfer bei Drag & Drop auf ein Archivsymbol und als MIME-Vorschlag - nicht die eigentliche Archiv-Engine |

Nicht in Meson gelistete, im Vertrag (§3, §34) aber vorgesehene Komponenten:

- **Poppler-GLib** (PDF-Vorschau, §34): im aktuellen Quellcode **nicht** eingebunden - FEHLT,
  nicht nur "optional deaktiviert".
- **GStreamer** (Video-/Audioinformationen, §34): im aktuellen Quellcode **nicht** eingebunden -
  FEHLT.
- **rsync**, **libreoffice --headless** (§3): im aktuellen Quellcode nicht als kontrollierter
  Subprozess eingebunden - FEHLT (Phase 2/erweiterte Vorschau, noch nicht umgesetzt).

Diese drei Punkte sind keine neuen Erkenntnisse dieser Änderung, sondern eine ehrliche
Bestandsaufnahme beim Erstellen dieser Datei (§4 der Vertragsregeln: keine Funktion als
implementiert behaupten, die nicht geprüft wurde).
