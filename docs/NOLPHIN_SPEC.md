ANWEISUNG FÜR DIESE SITZUNG

Ignoriere alle bisherigen Startanweisungen und frühere Versionen des Vertrags. Ab jetzt gilt ausschließlich der folgende Entwicklungsvertrag.

1. Speichere den Vertrag unten unverändert als docs/NOLPHIN_SPEC.md im Repository (vorhandene Version ersetzen).
2. Erstelle oder aktualisiere im Hauptordner die CLAUDE.md mit dem Hinweis: „docs/NOLPHIN_SPEC.md ist der verbindliche Entwicklungsvertrag für Nolphin und muss vor jeder Aufgabe gelesen werden."
3. Committe beide Dateien.
4. Führe danach ausschließlich die Erstanalyse aus Abschnitt 6 durch. Ändere keinen Code. Warte auf meine Freigabe.

==================================================

# NOLPHIN – MASTER DEVELOPMENT CONTRACT (Version 2)

Verbindlicher Entwicklungs-, Funktions- und Arbeitsvertrag

---

## 0. VERBINDLICHER ARBEITSMODUS

Du arbeitest an Nolphin, einem eigenständigen nativen Linux-Dateimanager für Linux Mint. Dieser Text ist der verbindliche Entwicklungsvertrag und die vollständige Funktionsspezifikation.

Absolute Regeln:

1. Du setzt die hier beschriebenen Anforderungen technisch um.
2. Du entfernst, vereinfachst oder ignorierst keine Anforderung eigenständig.
3. Du erfindest keine bereits implementierten Funktionen.
4. Du behauptest nie, dass eine Funktion fertig ist, wenn sie nicht tatsächlich implementiert und getestet wurde.
5. Du triffst keine eigenständigen Produktentscheidungen, wenn eine Anforderung eindeutig vorgegeben ist.
6. Bei echten technischen oder fachlichen Konflikten meldest du den Konflikt (Format siehe Abschnitt 58), statt selbst eine Anforderung zu streichen.
7. Bestehenden Code untersuchst du vor jeder Änderung.
8. Du überschreibst nie blind komplette Dateien, ohne deren Struktur und Funktion zu analysieren.
9. Nach einer Analyse wartest du auf meine Freigabe, bevor du größere Änderungen implementierst.
10. Jede Änderung muss einer konkreten Anforderung zugeordnet werden können.
11. Build, Installation und Funktionstests gehören zur Implementierung und dürfen nicht übersprungen werden.
12. Keine Pseudofunktionen, Fake-APIs, Platzhalter oder vorgetäuschten Implementierungen.
13. Ist eine Funktion wegen einer fehlenden Systemkomponente nicht verfügbar, wird dies technisch korrekt erkannt und dem Benutzer angezeigt.
14. Der Funktionsumfang darf nicht wegen einer bevorzugten Architektur künstlich reduziert werden.
15. Du arbeitest die Phasen aus Abschnitt 7 der Reihe nach ab. Eine spätere Phase beginnt erst nach meiner Freigabe.

---

## 1. ROLLE

Du arbeitest gleichzeitig als Lead Software Architect, Senior C Developer, Senior GTK3 Developer, GLib/GObject/GIO Developer, Linux-Systemintegrationsentwickler, Dateisystem- und Dateimanager-Entwickler, UI/UX-Entwickler, Build- und Release-Engineer, Test- und Qualitätssicherungsingenieur sowie als technischer Recherche-Assistent für Dateien, Bilder, 3D- und CAD-Formate.

Du entwickelst produktionsfähigen Code, keine Demonstration.

---

## 2. PRODUKTZIEL

Nolphin wird ein vollständiger, nativer Linux-Dateimanager:

- installierbar, stabil, performant, GTK3-nativ
- für Linux Mint (Cinnamon) optimiert, auf anderen GTK-basierten Linux-Desktops lauffähig
- als Standard-Dateimanager verwendbar
- umfangreiche Datei-, Verzeichnis-, Geräte-, Netzwerk-, Such-, Vorschau-, Archiv- und Systemfunktionen
- vollständig über die grafische Oberfläche bedienbar
- Tastatur- und Mausbedienung, Drag & Drop, Kontextmenüs
- Tabs, Split-Ansichten, Vorschau, integriertes Terminal
- konfigurierbare Einstellungen, erweiterbare Aktionen

Nolphin läuft ausschließlich unter Linux.

Die Benutzeroberfläche orientiert sich an der von mir bereitgestellten Referenz und an einer klassischen, übersichtlichen Linux-Dateimanager-Bedienung. Du darfst die Oberfläche nicht eigenständig in ein völlig anderes Produktdesign umwandeln. Ist keine Referenz im Projekt auffindbar, meldest du das in der Analyse.

---

## 3. TECHNISCHE GRUNDLAGE

Nolphin wird ausschließlich mit den Mitteln gebaut, die auch der Linux-Mint-Dateimanager Nemo verwendet bzw. die unter Linux Mint standardmäßig verfügbar sind. Nemo dient als technische Orientierung dafür, welche Bibliotheken, Systemdienste und Standards verwendet werden. Es wird kein Code aus Nemo oder anderen Projekten kopiert, und die Oberfläche richtet sich nach meiner Referenz, nicht nach Nemo.

Sprache und Build:

- C11
- Meson, Ninja

Kernbibliotheken:

- GTK3, GLib, GObject, GIO, GVFS, GdkPixbuf
- VTE 2.91 (integriertes Terminal)

Nolphin ist eine vollständig eigenständige Anwendung. Es besteht keine Abhängigkeit zu Nemo, zu Nemo-Paketen, zu Nemo-Erweiterungsbibliotheken oder zu Nemo-Konfigurationsdateien. Nolphin verändert keine Dateien oder Einstellungen anderer Dateimanager.

Optionale Mint-Komponenten (Einsatz jeweils in der Analyse begründen). Nolphin muss ohne jede dieser Komponenten kompilieren, starten und alle Kernfunktionen ausführen; fehlt eine, wird nur die jeweilige Zusatzfunktion deaktiviert und der Grund angezeigt:

- libxapp (XApp-Favoriten, XApp-Hilfsfunktionen)
- cinnamon-desktop (Thumbnail-Erzeugung nach Freedesktop-Standard); ohne diese Komponente liest Nolphin vorhandene Vorschaubilder aus ~/.cache/thumbnails und ruft installierte Freedesktop-Thumbnailer selbst auf
- libexif (EXIF-Daten von Bildern)
- GStreamer (Video- und Audioinformationen)
- Poppler-GLib (PDF-Vorschau)

Standards:

- Freedesktop/XDG: Benutzerverzeichnisse, MIME, Desktop-Dateien, Papierkorb, Thumbnails
- GTK-Lesezeichendatei für Orte
- GVFS-Metadaten (metadata::-Attribute) für Tags, Bewertungen, Kommentare, Emblems

Systemwerkzeuge, die unter Linux Mint vorhanden sind oder dort üblich nachinstalliert werden (nur über kontrollierte Subprozesse, Abschnitt 53.5):

- file-roller, tar, gzip, bzip2, xz, zip/unzip, 7z (Archive)
- rsync (Synchronisation)
- gpg (Verschlüsselung)
- git (Git-Aktionen)
- getfacl/setfacl (ACL)
- b2sum (BLAKE2-Prüfsummen)
- libreoffice --headless (Office-Vorschau, nur wenn installiert)
- dconf (Einstellungen exportieren/importieren)

Nicht verwenden: Qt, QML oder KDE-Bibliotheken jeglicher Art.

Jede weitere Bibliothek oder jedes weitere Werkzeug nur nach technischer Begründung und meiner Freigabe.

Grundsatz: Ist eine Funktion mit diesen Mitteln nicht umsetzbar, wird sie nicht nachgebaut oder vorgetäuscht, sondern als Konflikt gemeldet.

---

## 4. TECHNISCHE ZUORDNUNG

| Funktion | Technische Umsetzung |
|---|---|
| Lokale und entfernte Dateien, Netzwerk | GIO/GVFS |
| Papierkorb | GIO (trash://) |
| Zuletzt verwendet | GVFS (recent://) bzw. GtkRecentManager |
| Orte/Lesezeichen | GTK-Lesezeichendatei (~/.config/gtk-3.0/bookmarks) |
| Favoriten (Dateien und Ordner) | XApp-Favoriten (libxapp) |
| Dateizuordnungen, Öffnen mit | XDG-MIME, GAppInfo |
| Tags, Bewertung, Kommentare, Emblems | GVFS-Metadaten |
| Vorschaubilder | Freedesktop-Thumbnailer über cinnamon-desktop |
| Bilder | GdkPixbuf, libexif |
| PDF | Poppler-GLib |
| Video/Audio-Informationen | GStreamer |
| Geräte, Einbinden, LUKS-Entsperren | GIO GVolumeMonitor/GMount, GVFS/udisks |
| Zugangsdaten | GtkMountOperation mit Systemschlüsselbund |
| Benachrichtigungen | GNotification |
| Zwischenablage | GTK/GDK |
| Terminal | VTE 2.91 |
| Dateiüberwachung | GFileMonitor |
| Prüfsummen | GChecksum, BLAKE2 über b2sum |
| Archive | file-roller bzw. Systemwerkzeuge |
| Synchronisation | rsync |
| Verschlüsselung | gpg |
| Einstellungen | GSettings/dconf |
| Plugins | GModule |

---

## 5. ENTWICKLUNGSZYKLUS UND ARBEITSWEISE

Für jede größere Änderung gilt:

Gesamtziel erfassen → Dateien/Projekt prüfen → Bilder/Referenzen prüfen → relevante Anforderungen identifizieren → technische Analyse → Architektur → Freigabe → Implementierung → Build → Test → Nachweis

Nicht: Anforderung → sofort Code schreiben.

Gesamtbild vor Detailarbeit:

- Unterscheide zwischen Gesamtziel, aktueller Phase, aktueller Aufgabe, aktuellem Arbeitsschritt, relevanten Anforderungen und späteren Anforderungen.
- Der gesamte Vertrag bleibt verbindlich, aber nicht jede Anforderung ist für jeden Arbeitsschritt relevant. Eine einfache, konkrete Aufgabe darf nicht durch eine unnötige Vollanalyse aller Anforderungen verzögert werden.
- Bei jeder Aufgabe zuerst klären: Was soll am Ende tatsächlich funktionieren? Danach: Welche Dateien, Referenzen, Komponenten und Detailanforderungen werden dafür gebraucht? Erst dann beginnt die Detailarbeit.

Gesamtbild behalten + relevante Details korrekt bearbeiten + keine Anforderung eigenmächtig entfernen. Alle drei gelten gleichzeitig.

---

## 6. ERSTANALYSE (STARTBEFEHL)

Beim ersten Zugriff auf das Projekt veränderst du keinen Code. Du führst ausschließlich folgende Schritte aus:

1. Projektstruktur und vorhandene Dateien untersuchen
2. vorhandenen C-Code, GTK- und GObject-Struktur untersuchen
3. Architektur und Module erfassen
4. Build-System (Meson), Desktop-Datei, Icons, GSettings-Schema, Übersetzungen, Installationsstruktur untersuchen
5. vorhandene UI erfassen: Navigation, Dateiansicht, Tabs, Split View, Seitenleisten, Vorschau, Terminal, Menüs, Kontextmenüs, Einstellungen
6. vorhandene Tests erfassen
7. vorhandene Bilder, Referenzen, technische Dateien und 3D-/CAD-Dateien erfassen
8. technische Abhängigkeiten prüfen, insbesondere welche Komponenten aus Abschnitt 3 im Build-System bereits vorhanden sind
9. STL-/STEP-/STP-/FCStd-Unterstützung und vorhandene bzw. fehlende 3D-/CAD-Backends feststellen
10. Bild- und Dateirecherchebedarf bestimmen
11. Gap-Analyse erstellen (Abschnitt 6.1), gegliedert nach den Phasen aus Abschnitt 7
12. Architekturprobleme nennen
13. Konflikte nennen
14. Implementierungsreihenfolge innerhalb von Phase 1 vorschlagen

Danach: STOPP. Warte auf meine ausdrückliche Freigabe. Erst danach darf Code geändert werden.

### 6.1 Gap-Analyse

Für jede Funktion ist ausschließlich einer dieser Status erlaubt: IMPLEMENTIERT, TEILWEISE, FEHLT, UNBEKANNT.

| Anforderung | Phase | Status | Datei/Modul | Bemerkung |
|---|---|---|---|---|

Keine Funktion wird als IMPLEMENTIERT bezeichnet, solange dies nicht am tatsächlichen Projekt geprüft und getestet wurde.

---

## 7. PHASEN

Die Anforderungen werden in drei Phasen umgesetzt. Alle Phasen sind verbindlich; die Reihenfolge legt nur fest, was zuerst gebaut wird.

- **Phase 1 – Kern:** Abschnitte 11 bis 32 (Oberfläche, Tastatur, Navigation, Orte, Tabs, Split View, Terminal, Ansicht, Auswahl, Datei- und Bearbeiten-Funktionen, Kopieren/Verschieben, Löschen/Papierkorb, Öffnen, Kontextmenü, Drag & Drop, Zwischenablage, Eigenschaften, Berechtigungen, Filter/Suche Grundfunktionen, Vorschau Grundfunktionen, Einstellungen, Systemintegration)
- **Phase 2 – Erweitert:** Abschnitte 33 bis 41 (erweiterte Suche, erweiterte Vorschau, Metadaten und Tags, Archive, Netzwerk, Geräte und Dateisystem, Prüfsummen, Verschlüsselung, Git, Arbeitsbereiche)
- **Phase 3 – Zusatzfunktionen:** Abschnitte 42 bis 48 (3D/CAD, Aktionen und Automatisierung, Synchronisation, Versionierung, Duplikaterkennung, Plugins, Verwaltung und Diagnose)

Die Architektur von Phase 1 muss so angelegt sein, dass Phase 2 und 3 ohne Umbau des Hauptprogramms ergänzt werden können.

---

## 8. ÄNDERUNGSPROTOKOLL

Jede Änderung wird dokumentiert:

- REQUIREMENT: Welche Anforderung wird umgesetzt?
- DATEI: Welche Datei(en) werden geändert?
- ÄNDERUNG: Was wurde konkret geändert?
- WARUM: Warum ist die Änderung erforderlich?
- TEST: Wie wurde die Funktion getestet?

---

## 9. PROJEKTSTRUKTUR

Mindestens: `src/`, `data/`, `data/icons/`, `tests/`, `docs/`

Mögliche Module (jeweils .c/.h):

nolphin-main, nolphin-window, nolphin-file-view, nolphin-navigation, nolphin-tabs, nolphin-split-view, nolphin-sidebar, nolphin-preview, nolphin-preview-3d, nolphin-terminal, nolphin-search, nolphin-properties, nolphin-file-operations, nolphin-operation-queue, nolphin-device-manager, nolphin-network, nolphin-archive, nolphin-cad, nolphin-settings, nolphin-actions, nolphin-plugins, nolphin-metadata, nolphin-workspace, nolphin-sync, nolphin-versions

Die Modulaufteilung darf verbessert werden, wenn es technisch sinnvoller ist. Funktionalität darf dabei nicht entfernt werden.

---

## 10. KERNARCHITEKTUR

Nolphin besteht aus getrennten Engines:

- **Dateisystem-Engine:** lokale Dateien, entfernte Dateien, virtuelle Ressourcen (alles über GIO/GVFS)
- **Operations-Engine:** Kopieren, Verschieben, Löschen, Umbenennen, Wiederherstellen, mit Warteschlange und Rückgängig-Verlauf
- **Such-Engine:** Dateisuche, Inhaltssuche, Filter (ohne eigenen Hintergrund-Index)
- **Vorschau-Engine:** Vorschau-Erzeugung, Thumbnail-System, Metadaten-Auslesen
- **Plugin-Engine:** Laden und Verwalten von Plugins und Aktionen

---

# PHASE 1 – KERN

## 11. FENSTER UND BENUTZEROBERFLÄCHE

- Hauptfenster: Menüleiste, Werkzeugleiste, Adressleiste, Hauptansicht, Seitenleiste, Statusleiste
- Panels: Orte, Ordnerbaum, Informationen/Vorschau, Terminal
- mehrere Fenster gleichzeitig möglich

Werkzeugleiste: Zurück, Vorwärts, Übergeordnet, Startseite, Aktualisieren, Suchen, Neuer Tab, Split, Ansichtsmodus, Einstellungen. Buttons spiegeln den tatsächlichen Zustand wider.

UI-Regeln: logisch, konsistent, übersichtlich, schnell verständlich, tastatur- und mausbedienbar, skalierbar. Keine redundanten oder widersprüchlichen Bedienkonzepte. Verändert eine UI-Änderung die Referenz wesentlich: dokumentieren und meine Freigabe einholen.

### 11.1 Menüleiste

Alle Funktionen von Nolphin werden in der eigenen Menüleiste von Nolphin untergebracht. Die Menüleiste von Nolphin hat genau diese sechs Hauptmenüs, in dieser Reihenfolge:

**Datei – Bearbeiten – Ansicht – Gehe zu – Lesezeichen – Hilfe**

Es werden keine weiteren Hauptmenüs angelegt. Jede Funktion dieses Vertrags, die über die Oberfläche bedienbar ist, muss in der Menüleiste von Nolphin erreichbar sein – auch wenn sie zusätzlich über Tastenkürzel, Werkzeugleiste oder Kontextmenü erreichbar ist. Zusätzliche Funktionen kommen in Untermenüs (▸).

Regeln:

- Jeder Menüeintrag ist mit derselben GAction verbunden wie das zugehörige Tastenkürzel, der Werkzeugleisten-Button und der Kontextmenü-Eintrag. Eine Funktion wird nur einmal implementiert.
- Tastenkürzel werden im Menü neben dem Eintrag angezeigt.
- Ein Menüeintrag wird erst hinzugefügt, wenn die Funktion tatsächlich implementiert ist. Keine leeren oder funktionslosen Einträge für spätere Phasen.
- Einträge, die im aktuellen Zustand nicht möglich sind (z. B. Einfügen bei leerer Zwischenablage), werden ausgegraut.
- Einträge, deren Systemkomponente fehlt (z. B. git, gpg, rsync), werden ausgegraut und zeigen beim Überfahren mit der Maus den Grund an.
- Die Menüleiste ist ein-/ausblendbar; bei ausgeblendeter Menüleiste blendet Alt sie vorübergehend ein.

**Datei**

- Neues Fenster
- Neuer Tab (Strg+T)
- Neuer Ordner (Strg+Shift+N)
- Neue Datei ▸ Leere Datei, Vorlagen aus ~/Vorlagen
- Verknüpfung erstellen ▸ Verknüpfung, Symbolische Verknüpfung, Hardlink
- Öffnen (Enter)
- Öffnen mit ▸ empfohlene Anwendungen, Andere Anwendung …
- In neuem Tab öffnen
- In neuem Bereich öffnen
- Mit Server verbinden …
- Arbeitsbereiche ▸ Speichern …, Laden, Duplizieren, Verwalten …
- Eigenschaften (Alt+Enter)
- Tab schließen (Strg+W)
- Fenster schließen
- Alle Fenster schließen / Beenden

**Bearbeiten**

- Rückgängig (Strg+Z), Wiederholen (Strg+Shift+Z)
- Ausschneiden (Strg+X), Kopieren (Strg+C), Einfügen (Strg+V)
- Zwischenablage als Datei einfügen
- Kopieren als ▸ Pfad, Dateiname
- Zwischenablage ▸ Inhalt anzeigen, Leeren
- Duplizieren
- Umbenennen (F2)
- Massenumbenennung …
- In den Papierkorb verschieben (Entf)
- Endgültig löschen (Shift+Entf)
- Alles auswählen (Strg+A), Auswahl aufheben, Auswahl umkehren
- Nach Kriterien auswählen …
- Auswahl ▸ Auswahl speichern …, Gespeicherte Auswahl wiederherstellen
- Archiv ▸ Komprimieren …, Hier entpacken, Entpacken nach …, Archiv testen
- Sicherheit ▸ Prüfsumme berechnen …, Verschlüsseln …, Entschlüsseln, Berechtigungen und ACL …
- Metadaten ▸ Tags bearbeiten …, Bewertung, Kommentar …, Emblem
- Werkzeuge ▸ Ordner vergleichen / Synchronisieren …, Duplikate finden …, Regeln anwenden …, Stapelverarbeitung …, Versionen ▸ (Version speichern, Versionen anzeigen …)
- Git ▸ Status, Hinzufügen, Commit …, Pull, Push, Log, Diff
- Aktionen ▸ eigene Aktionen
- Skripte ▸ Skripte aus dem Skriptordner, Skriptordner öffnen
- Plugins …
- Einstellungen

**Ansicht**

- Stopp
- Neu laden (F5)
- Seitenleiste ▸ Orte, Ordnerbaum, Ausblenden (F9)
- Informationsbereich (F11)
- Terminal (F4)
- Werkzeugleiste, Statusleiste, Menüleiste (jeweils ein/aus)
- Geteilte Ansicht ▸ Vertikal teilen (F3), Horizontal teilen (Shift+F3), Bereich duplizieren, Bereich maximieren, Bereich schließen
- Filterleiste (Strg+I)
- Versteckte Dateien anzeigen (Strg+H)
- Ansichtsmodus ▸ Symbole, Liste, Kompakt, Galerie
- Sortieren nach ▸ Name, Größe, Typ, Datum, Besitzer, Erweiterung, Umgekehrte Reihenfolge
- Gruppieren nach ▸ Keine, Name, Typ, Datum, Größe, Erweiterung
- Sichtbare Spalten …
- Vergrößern (Strg++), Verkleinern (Strg+-), Normale Größe (Strg+0)

**Gehe zu**

- Übergeordneter Ordner (Alt+↑)
- Zurück (Alt+←), Vorwärts (Alt+→)
- Verlauf ▸ zuletzt besuchte Ordner, Verlauf löschen
- Persönlicher Ordner (Alt+Pos1)
- Computer
- Dateisystem (/)
- Vorlagen
- Zuletzt verwendet
- Häufig verwendet
- Favoriten
- Netzwerk
- Papierkorb
- Ort eingeben … (Strg+L)
- Suchen … (Strg+F)
- Tabs ▸ Nächster Tab (Strg+Tab), Vorheriger Tab (Strg+Shift+Tab), Geschlossenen Tab wiederherstellen (Strg+Shift+T), Tab sperren, Tab umbenennen, Alle Tabs schließen

**Lesezeichen**

- Lesezeichen hinzufügen (Strg+D)
- Lesezeichen bearbeiten …
- Zu Favoriten hinzufügen
- Favoriten verwalten …
- danach: Liste aller Lesezeichen

**Hilfe**

- Tastenkürzel
- Diagnose ▸ Protokolle anzeigen, Fehlerbericht erstellen, Systeminformationen, Plugin-Status
- Über Nolphin

Einstellungen exportieren, importieren und zurücksetzen befinden sich im Einstellungsdialog (Bearbeiten ▸ Einstellungen).

Weicht diese Menüstruktur an einer Stelle von der Referenz ab oder ist eine Einordnung technisch unsinnig, meldest du das als Konflikt, statt die Struktur eigenständig zu ändern.

---

## 12. TASTATURBEDIENUNG

Alle Kürzel müssen tatsächlich funktionieren.

| Taste | Funktion |
|---|---|
| Pfeiltasten | Navigation in der Dateiansicht |
| Enter | Öffnen |
| Escape | Umbenennen, Filter oder Dialog abbrechen |
| Backspace / Alt+↑ | Übergeordneter Ordner |
| Alt+← / Alt+→ | Zurück / Vorwärts |
| Alt+Pos1 | Persönlicher Ordner |
| Strg+D | Lesezeichen hinzufügen |
| Strg++ / Strg+- / Strg+0 | Vergrößern / Verkleinern / Normale Größe |
| Leertaste | Vorschau der ausgewählten Datei ein-/ausblenden |
| Strg+C / Strg+X / Strg+V | Kopieren / Ausschneiden / Einfügen |
| Strg+Z / Strg+Shift+Z | Rückgängig / Wiederholen |
| Strg+A | Alles auswählen |
| Strg+Shift+N | Neuer Ordner |
| F2 | Umbenennen |
| Entf | In den Papierkorb verschieben |
| Shift+Entf | Endgültig löschen (mit Bestätigung) |
| Alt+Enter | Eigenschaften |
| F5 | Aktualisieren |
| Strg+H | Versteckte Dateien ein-/ausblenden |
| Strg+L | Pfad eingeben |
| Strg+I | Filterleiste für das aktuelle Verzeichnis |
| Strg+F | Vollständige Suche |
| Strg+T | Neuer Tab |
| Strg+W | Tab schließen |
| Strg+Shift+T | Geschlossenen Tab wiederherstellen |
| Strg+Tab / Strg+Shift+Tab | Nächster / vorheriger Tab |
| F3 | Vertikale Teilung |
| Shift+F3 | Horizontale Teilung |
| F4 | Integriertes Terminal ein-/ausblenden |
| F9 | Seitenleiste ein-/ausblenden |
| F11 | Informations-/Vorschaubereich ein-/ausblenden |

---

## 13. NAVIGATION

- Verlauf: Zurück, Vorwärts, Verlauf anzeigen, Verlauf löschen
- Verzeichnisse: Übergeordnet, Startseite, Persönlicher Ordner, Computer, Root-Dateisystem, Netzwerk, Papierkorb
- Pfadnavigation: Breadcrumbs, editierbare Adressleiste (Pfade und URIs), Pfad kopieren, Pfad einfügen, Pfad als Ort speichern
- Schnellnavigation: Orte, Favoriten, Zuletzt verwendet, Häufig verwendet (von Nolphin selbst gezählt), Geräte, Netzwerke
- zwischen Split-Bereichen wechseln

---

## 14. ORTE UND FAVORITEN

- Standardorte (XDG-Benutzerverzeichnisse): Persönlicher Ordner, Desktop, Dokumente, Downloads, Bilder, Musik, Videos, Papierkorb
- Eigene Orte (GTK-Lesezeichen): hinzufügen, entfernen, bearbeiten, umbenennen, verschieben
- Favoriten (XApp-Favoriten): Ordner und Dateien favorisieren, Favorit öffnen, Favoriten verwalten
- Geräte: Festplatten, USB-Geräte, externe Laufwerke, Netzwerkspeicher

Die Seitenleiste arbeitet konsistent mit der Hauptnavigation.

---

## 15. TABS

Jeder Tab hat eigenen Pfad, eigene Navigation, eigene Auswahl, eigene Dateiansicht und eigenen Zustand. Tabs verändern nie versehentlich den Zustand anderer Tabs.

- Verwaltung: neuer Tab, Tab schließen, alle Tabs schließen, Tab duplizieren, Tab verschieben, Tab umbenennen, Tab sperren (ein gesperrter Tab kann nicht versehentlich geschlossen werden)
- Navigation: nächster Tab, vorheriger Tab, Tab auswählen
- Wiederherstellung: geschlossenen Tab wiederherstellen, Tabs beim Start wiederherstellen
- Ordner in neuem Tab öffnen

---

## 16. SPLIT VIEW

- F3: vertikale Teilung, Shift+F3: horizontale Teilung
- ein bis vier Bereiche, rekursiv über GtkPaned aufgebaut
- jeder Bereich ist eine eigenständige Dateiansicht mit eigenem Pfad, Navigationszustand, eigener Auswahl, eigenen Tabs und eigenem Kontext
- Bereichsaktionen: aktivieren, wechseln, schließen, duplizieren, maximieren
- Dateiübertragung: zwischen Bereichen kopieren und verschieben, Drag & Drop zwischen Bereichen; der andere Bereich wird als Standardziel vorgeschlagen
- Ordner in neuem Bereich öffnen

---

## 17. INTEGRIERTES TERMINAL (F4)

- VTE 2.91, im unteren Bereich über GtkPaned, Höhe veränderbar
- mehrere Terminals als Tabs im Terminalbereich
- Terminalpfad folgt dem aktuellen Verzeichnis; Navigation im Dateimanager aktualisiert den Terminalpfad
- „Terminal hier öffnen" im Kontextmenü öffnet das integrierte Terminal im gewählten Ordner
- Befehle und Skripte ausführen; Befehl als Aktion speichern (Abschnitt 43)

Keine externe Terminalanwendung als Ersatz.

---

## 18. ANSICHT

- Ansichtsmodi: Symbolansicht, Kompakte Ansicht, Listenansicht mit Spalten (Detailansicht), aufklappbare Ordner in der Listenansicht (Baumansicht), Galerieansicht (große Vorschaubilder)
- Darstellung: Symbolgröße, Miniaturansichten, Dateiname, Größe, Typ, Änderungsdatum, Berechtigungen, Besitzer, Speicherort
- Sortierung: Name, Größe, Typ, Datum, Besitzer, Erweiterung – jeweils auf- und absteigend
- Gruppierung: Name, Typ, Datum, Größe, Erweiterung
- Anzeige: versteckte Dateien, Informations-/Vorschaubereich, Statusleiste (mit freiem Speicherplatz), Seitenleiste
- Detailspalten konfigurierbar (ein-/ausblenden, Reihenfolge, Breite), Schriftgröße

---

### 18.1 Symbole (Icons)

- Datei- und Ordnersymbole kommen aus dem im System eingestellten Symbolthema (GtkIconTheme), passend zum MIME-Typ (`standard::icon`, `g_content_type_get_icon()`); keine eigenen Ersatzsymbole
- beim Abfragen von Dateiinformationen werden alle benötigten Attribute ausdrücklich angefordert, mindestens `standard::*`, `access::*`, `unix::mode`, `thumbnail::*`
- ein Schloss- bzw. „nicht lesbar"-Emblem erscheint nur, wenn `access::can-read` bzw. `access::can-write` tatsächlich vorhanden und FALSE ist; ein fehlendes Attribut gilt nie als „kein Zugriff"
- fehlt ein Symbol im Thema, wird eine sinnvolle Rückfallstufe verwendet (z. B. `text-x-generic`, `folder`, `application-x-executable`)
- das Programmsymbol wird nach `<präfix>/share/icons/hicolor/scalable/apps/` installiert und der Symbol-Cache aktualisiert

Test: Im persönlichen Ordner zeigen normale Dateien und Ordner ihre üblichen Symbole ohne Schloss; nur eine Datei ohne Leserechte (z. B. nach `chmod 000`) zeigt das Schloss.

---

## 19. AUSWAHL

- Einzelne Datei, mehrere Dateien, Bereich (Shift), Strg-Auswahl, Tastaturfokus
- Alles auswählen, Auswahl aufheben, Auswahl umkehren
- Auswahl nach Kriterien: Erweiterung, Dateityp, Name (mit Platzhaltern), Größe, Datum
- Auswahl speichern: aktuelle Auswahl unter einem Namen merken und später im selben Ordner wiederherstellen

---

## 20. DATEI-FUNKTIONEN

- Neu: Ordner, leere Datei, Dokument aus Vorlage (~/Vorlagen), Verknüpfung (.desktop), symbolische Verknüpfung, Hardlink (nur innerhalb desselben Dateisystems und nur für Dateien; sonst verständliche Meldung)
- Öffnen: Datei, Ordner, mit Standardanwendung, Öffnen mit …, in neuem Tab, in neuem Bereich
- Operationen: Kopieren, Ausschneiden, Einfügen, Verschieben, Umbenennen, Duplizieren, In Papierkorb verschieben, Löschen
- Verknüpfungen: Ziel anzeigen, zum Ziel springen
- Beenden: Fenster schließen, alle Fenster schließen; laufende Vorgänge werden vor dem Beenden angezeigt und müssen bestätigt werden

---

## 21. BEARBEITEN-FUNKTIONEN

- Rückgängig/Wiederholen funktioniert tatsächlich für mindestens: Umbenennen, Verschieben, Kopieren, In Papierkorb verschieben, Ordner erstellen
- Zwischenablage: aktuellen Inhalt anzeigen, leeren, Pfad kopieren, Dateiname kopieren, Text oder Bild aus der Zwischenablage als Datei einfügen

---

## 22. KOPIEREN UND VERSCHIEBEN

- einzelne Dateien, mehrere Dateien, Ordner, rekursiv, große Dateien
- Fortschritt: Fortschrittsbalken, Geschwindigkeit, verbleibende Zeit, übertragene und verbleibende Daten
- Steuerung: Pause, Fortsetzen, Abbrechen, Warteschlange, Reihenfolge wartender Vorgänge ändern
- Namenskonflikte: Überschreiben, Überspringen, Umbenennen, Ordner zusammenführen, Für alle anwenden
- Fehler werden verständlich angezeigt

---

## 23. LÖSCHEN UND PAPIERKORB

- In Papierkorb verschieben, endgültig löschen (mit Bestätigung), klar unterscheidbar
- Papierkorb öffnen, leeren, Dateien und Ordner wiederherstellen, einzelne Einträge endgültig löschen
- Papierkorbverwaltung: automatische Bereinigung nach Aufbewahrungsdauer (anhand DeletionDate der Freedesktop-Papierkorbinfo), Größenlimit mit Warnung

---

## 24. DATEIEN ÖFFNEN

Standardanwendung, Öffnen mit …, Standardanwendung ändern, passende Anwendungen vorschlagen (GAppInfo-Empfehlungen), ausführbare Dateien starten und Skripte ausführen (jeweils mit Rückfrage).

---

## 25. KONTEXTMENÜ

Passt sich an das Objekt an (Datei, Ordner, Archiv, mehrere Objekte, leerer Bereich):

- Datei: Öffnen, Öffnen mit …, Kopieren, Ausschneiden, Umbenennen, Duplizieren, Löschen, Eigenschaften
- Ordner: zusätzlich In neuem Tab öffnen, In neuem Bereich öffnen, Terminal hier öffnen
- Erweitert: Komprimieren, Entpacken, Senden an (E-Mail-Programm bzw. andere Anwendung), Prüfsumme berechnen, Verschlüsseln, Entschlüsseln
- Benutzeraktionen: eigene Aktionen, Skripte, Plugin-Einträge

---

## 26. DRAG & DROP

- Kopieren, Verschieben, Verknüpfen (Auswahl beim Loslassen)
- zwischen Tabs, Bereichen, Fenstern sowie zwischen lokalen und Netzwerkpfaden
- in andere Anwendungen und auf den Desktop, aus anderen Anwendungen in Nolphin
- sinnvolle Konfliktbehandlung

---

## 27. ZWISCHENABLAGE

Kopieren, Ausschneiden, Einfügen von mehreren Dateien und Ordnern, mit internen und externen Anwendungen, über GTK/GDK mit den Formaten text/uri-list und x-special/gnome-copied-files.

---

## 28. EIGENSCHAFTEN UND BERECHTIGUNGEN

- Basis: Name, Typ, MIME-Typ, Größe, Ordnergröße, Speicherort, Dateisystem, Verknüpfungsziel
- Zeit: Erstellung (soweit das Dateisystem sie liefert), Änderung, letzter Zugriff
- Besitz: Benutzer, Gruppe, Berechtigungen
- Technisch: Inode, Dateisystem-ID
- Berechtigungen ändern: Lesen, Schreiben, Ausführen für Besitzer, Gruppe und Andere; Besitzer und Gruppe ändern; Schreibschutz setzen/entfernen; rekursiv anwenden

Ordnergrößen blockieren nie die GUI. Keine stillen Root-Aktionen; Operationen, die Root-Rechte benötigen, werden klar gemeldet.

---

## 29. FILTER UND SUCHE (GRUNDFUNKTIONEN)

- Filterleiste (Strg+I): GtkSearchEntry, filtert die aktuelle Ansicht sofort
- Suche (Strg+F): Dateiname, Ordnername, Erweiterung, Pfad
- Ergebnisse: filtern, sortieren, öffnen, zum Speicherort springen
- keine Blockierung der GUI, auch bei großen Verzeichnissen

---

## 30. VORSCHAU UND INFORMATIONSBEREICH (GRUNDFUNKTIONEN)

Rechter Bereich (F11):

- Datei: Name, Typ, Größe, Datum, Berechtigungen, Besitzer, Gruppe
- Speicher: Speicherort, Gerät, Dateisystem, Speicherverbrauch
- Bilder (JPG, PNG, SVG, WebP): Vorschau über GdkPixbuf mit erhaltenem Seitenverhältnis, Breite, Höhe
- Text und Markdown: Textvorschau
- nicht unterstützte Dateien: sinnvolle Fallback-Anzeige

Architektur: eigenständiges Modul mit Backend-Struktur, alle Operationen asynchron. Beispiel-API: `nolphin_preview_new()`, `nolphin_preview_set_file()`, `nolphin_preview_clear()`, `nolphin_preview_update()` (anpassbar, solange das Modul getrennt bleibt).

---

## 31. EINSTELLUNGEN

Vollständig über GSettings/dconf mit XML-Schema `data/org.nolphin.gschema.xml`. Keine fest codierte Konfiguration, wenn sie sinnvoll konfigurierbar ist.

- Allgemein: Startordner, Standardansicht, Fenstergröße, Verhalten beim Öffnen (Einfach-/Doppelklick), Bestätigungsdialoge
- Navigation: Tabs, Split View, Verlauf, Breadcrumbs, Tabs beim Start wiederherstellen
- Darstellung: Symbolgröße, Vorschauen, Spalten, Zeilenhöhe in der Listenansicht, Sortierung, Gruppierung
- Dateioperationen: Kopierverhalten, Überschreibverhalten, Papierkorb (Aufbewahrung, Größenlimit), Löschbestätigung, Warteschlange
- Terminal: Standardzustand, Höhe
- Suche: Suchpfade, Standardfilter
- Netzwerk: Standardprotokoll im Verbindungsdialog, gespeicherte Verbindungen
- Vorschau: aktivieren/deaktivieren, automatische Vorschau, maximale Dateigröße, unterstützte Typen, Breite des Informationsbereichs
- 3D/CAD: 3D-Vorschau aktivieren/deaktivieren, maximale Dateigröße, erlaubte Formate
- Kontextmenü: Aktionen und Skripte ein-/ausblenden

Nach Schemaänderungen: `glib-compile-schemas`. Einstellungen sind persistent.

---

## 32. SYSTEMINTEGRATION

- Linux-Desktop-Integration über GIO/GVFS und XDG; optimiert für Cinnamon, ohne harte Abhängigkeit von einem Desktop
- Benachrichtigungen (GNotification) für längere Vorgänge; nie ein vorgetäuschter Erfolg
- Desktop-Datei `org.nolphin.FileManager.desktop`, Icon `org.nolphin.FileManager.svg`
- Standard-Dateimanager: `xdg-mime default org.nolphin.FileManager.desktop inode/directory` – die Registrierung wird tatsächlich geprüft
- Dateiüberwachung: Ansichten aktualisieren sich automatisch bei Änderungen (GFileMonitor)

---

# PHASE 2 – ERWEITERT

## 33. ERWEITERTE SUCHE

- Kriterien: Dateityp, Größe, Änderungsdatum, Erstellungsdatum, Besitzer, Gruppe, Berechtigungen
- Inhaltssuche: Textinhalt, Tags, Kommentare (aus GVFS-Metadaten)
- Operatoren: UND, ODER, NICHT, exakte Übereinstimmung, Platzhalter, reguläre Ausdrücke (GRegex)
- Ergebnisse: gruppieren, als Liste speichern, Suche speichern, Suchverlauf

---

## 34. ERWEITERTE VORSCHAU

- Bilder: EXIF über libexif, Seitenverhältnis; RAW-Fotos nur, wenn ein passender Thumbnailer installiert ist
- PDF über Poppler-GLib
- Office-Dokumente: Vorschaubild über installierte Thumbnailer; Seitenvorschau nur, wenn LibreOffice installiert ist
- Video und Audio: Dauer, Auflösung, Codec-Informationen über GStreamer; Vorschaubilder über Thumbnailer
- fehlt eine Komponente, wird das verständlich angezeigt

---

## 35. METADATEN UND TAGS

Gespeichert über GVFS-Metadaten:

- Tags vergeben, entfernen, danach suchen und filtern
- Bewertung (0–5 Sterne)
- Kommentare
- Emblems

Nur lesend, sofern in der Datei vorhanden: Autor, Titel, Beschreibung (z. B. aus PDF-, Bild- oder Audio-Metadaten).

Anzeige im Informationsbereich und im Eigenschaften-Dialog.

---

## 36. ARCHIVE

- Erstellen: ZIP, TAR, TAR.GZ, TAR.BZ2, TAR.XZ, 7Z
- Entpacken: hier, nach …, einzelne oder mehrere Dateien extrahieren
- Verwaltung: Archiv öffnen, Inhalt anzeigen, Dateien hinzufügen, entfernen, ersetzen, Archiv testen, Archivinformationen
- weitere Formate, soweit die installierten Werkzeuge sie unterstützen

Abstrahierte Backend-Struktur über file-roller bzw. Systemwerkzeuge (Regeln aus Abschnitt 53.5). Bei TAR-basierten Formaten erfordert Bearbeiten ein Neupacken; der Benutzer wird bei großen Archiven darauf hingewiesen.

---

## 37. NETZWERK UND REMOTE-DATEIEN

- Netzwerk durchsuchen, Netzwerkgeräte, Freigaben, Server anzeigen
- Protokolle (soweit GVFS-Backends installiert): SMB (`smb://`), SFTP/SSH (`sftp://`), FTP (`ftp://`), WebDAV (`dav://`, `davs://`), NFS (`nfs://`), HTTP/HTTPS (`http://`, `https://`, nur lesend)
- Netzwerkressourcen hinzufügen, bearbeiten, entfernen, als Ort speichern
- Remote-Dateien: öffnen, kopieren, verschieben, löschen, umbenennen, herunter- und hochladen (nicht bei HTTP/HTTPS)
- Zugangsdaten über GtkMountOperation und den Systemschlüsselbund, nie im Klartext
- nicht installierte Backends werden dynamisch erkannt und gemeldet; keine vorgetäuschte Unterstützung

---

## 38. GERÄTE UND DATEISYSTEM

- Geräte: Festplatten, SSDs, USB-Sticks, SD-Karten, externe Festplatten, optische Laufwerke
- Aktionen: öffnen, einbinden, aushängen, sicher entfernen (soweit vom System unterstützt), Informationen anzeigen, LUKS-verschlüsselte Laufwerke entsperren (über GVFS/udisks)
- Speicherinformationen: Gesamtkapazität, belegt, frei, Dateisystemtyp, Mountpoint
- Verzeichnisse: Root `/`, `/home`, `/tmp`, `/var`, `/etc` und Benutzerverzeichnisse durchsuchbar (Schreibzugriff nur mit den vorhandenen Benutzerrechten)
- fehlerhafte oder nicht zugängliche Mountpoints werden sauber behandelt; Gerätezustände aktualisieren sich automatisch

---

## 39. SICHERHEITSFUNKTIONEN

- Prüfsummen: MD5, SHA-1, SHA-256, SHA-512 (GChecksum), BLAKE2 (b2sum); berechnen und mit einem eingegebenen Wert vergleichen
- Verschlüsselung über gpg: Datei verschlüsseln, entschlüsseln; Ordner verschlüsseln (als verschlüsseltes Archiv)
- ACL anzeigen und bearbeiten über getfacl/setfacl, sofern das Dateisystem ACL unterstützt

---

## 40. GIT

Über das Systemwerkzeug git: Status von Dateien anzeigen (als Emblem bzw. Spalte), Hinzufügen, Commit, Pull, Push, Log anzeigen, Diff anzeigen. Ohne installiertes git wird die Funktion ausgeblendet und der Grund angezeigt.

---

## 41. ARBEITSBEREICHE

Ein Arbeitsbereich umfasst: Tabs, Bereiche und Layout, Pfade, geöffnete Panels, Terminalzustand, Fensterposition und -größe.

- Arbeitsbereich speichern (mit Namen), laden, duplizieren, löschen
- letzten Arbeitsbereich beim Start automatisch wiederherstellen
- automatisches Speichern beim Beenden

---

# PHASE 3 – ZUSATZFUNKTIONEN

## 42. 3D- UND CAD-DATEIEN

Mindestens: STL, STEP, STP, FCStd (FreeCAD).

- STL: Erkennung, Unterscheidung ASCII/Binär, Dateigröße, Geometrieinformationen (z. B. Anzahl Dreiecke), Vorschau soweit Backend verfügbar
- STEP/STP: Erkennung, Dateiinformationen, technische Metadaten aus dem Dateikopf, Vorschau soweit Backend verfügbar
- FCStd: Erkennung (ZIP-Container), Dateiinformationen, Dokumentinformationen aus Document.xml, eingebettetes Vorschaubild, sofern vorhanden

Architektur getrennt vom Bild-Backend:

```
Vorschau
├── Bild
├── Text
├── PDF
├── Video/Audio
├── Dokument
└── 3D
    ├── STL
    ├── STEP/STP
    └── FCStd
```

Dateityp-Erkennung → Backend-Auswahl → Metadaten → Vorschau → Aktionen. Neue Formate lassen sich ohne Umbau ergänzen. Weitere Formate (IGES, OBJ, 3MF, DXF, DWG) werden erkannt und als nicht unterstützt ausgewiesen.

Ein 3D-Renderer wird erst nach Prüfung verfügbarer Komponenten vorgeschlagen und nur nach meiner Freigabe eingebunden. Ohne Backend: Dateityp korrekt erkennen, Datei normal verwalten, fehlende Vorschau verständlich erklären. Keine vorgetäuschte 3D-Ansicht.

---

## 43. AKTIONEN UND AUTOMATISIERUNG

- Benutzerdefinierte Aktionen als Schlüsseldateien (z. B. `~/.local/share/nolphin/actions/*.nolphin_action`) mit Name, Befehl, Dateikontext (MIME-Typen, Erweiterungen), Auswahl (eine/mehrere Dateien, Ordner), Parameter, Sichtbarkeit, Ausführungsart (im Hintergrund oder im integrierten Terminal)
- Skripte: Ordner `~/.local/share/nolphin/scripts/`, erscheinen im Kontextmenü unter „Skripte"
- Stapelverarbeitung: Aktion auf viele Dateien anwenden
- Massenumbenennung: Suchen/Ersetzen, Nummerierung, Groß-/Kleinschreibung, Vorschau vor dem Ausführen, rückgängig machbar
- Regeln: „Wenn Dateityp/Name/Größe/Datum … dann Aktion …" – werden nur manuell auf einen gewählten Ordner angewendet, mit Vorschau der betroffenen Dateien vor der Ausführung

---

## 44. SYNCHRONISATION

Über rsync:

- Ordner vergleichen und Unterschiede anzeigen
- einseitig synchronisieren: lokal → Ziel oder Ziel → lokal (Ziel kann ein eingebundener Netzwerkordner sein)
- Konflikte: Quelle behalten, Ziel behalten, beide behalten, pro Datei entscheiden
- Vorschau („Trockenlauf") vor jeder Synchronisation

---

## 45. VERSIONIERUNG

Nolphin speichert Versionen selbst unter `~/.local/share/nolphin/versions/`:

- Version einer Datei speichern (manuell)
- Versionen anzeigen mit Zeitstempel
- Version wiederherstellen, Version löschen
- Unterschiede anzeigen (bei Textdateien)

---

## 46. DUPLIKATERKENNUNG

Duplikate in einem gewählten Ordner finden nach gleichem Namen, gleicher Größe und gleicher Prüfsumme. Ergebnisse gruppiert anzeigen; Löschen nur in den Papierkorb und nur nach Bestätigung.

---

## 47. PLUGINS UND ERWEITERUNGEN

- Plugins als Shared Libraries über GModule, mit definierter Plugin-Schnittstelle
- Plugin-Typen: Vorschau, Dateityp, Kontextmenü, Werkzeugleiste, Seitenleiste
- Plugin-Verwaltung: Liste, aktivieren/deaktivieren, Fehlerstatus
- ein fehlerhaftes Plugin darf Nolphin nicht unbemerkt instabil machen; Ladefehler werden angezeigt

Die Architektur ist so gebaut, dass Erweiterungen keine Änderungen am gesamten Hauptprogramm erfordern.

---

## 48. VERWALTUNG UND DIAGNOSE

- Einstellungen exportieren und importieren (dconf), auf Werkseinstellungen zurücksetzen
- Protokolle lokal speichern und anzeigen
- Fehlerberichte lokal als Datei erzeugen (kein automatisches Senden)
- Plugin-Status, Systeminformationen (Version, verfügbare GVFS-Backends, gefundene Werkzeuge)

---

# ALLGEMEINE REGELN (gelten ab Phase 1 für jede Funktion)

## 49. SPRACHE

Nolphin ist eine deutschsprachige Anwendung. Alle sichtbaren Texte (Menüs, Buttons, Dialoge, Meldungen, Tooltips, Statusleiste, Desktop-Datei, Einstellungen) sind auf Deutsch, unabhängig von der Systemsprache. Diese Entscheidung ist verbindlich und wird nicht rückgängig gemacht.

- Die vorhandene deutsche Umstellung des Projekts bleibt erhalten. Du baust sie nicht auf Englisch oder auf gettext zurück, ohne dass ich es ausdrücklich verlange.
- Jede neue Funktion bringt ihre Texte direkt auf Deutsch mit.
- Namen aus GTK, GLib, GIO und anderen Bibliotheken (Funktionen, Typen, Signale, Attribute, Icon-Namen) bleiben unverändert. Findest du Stellen, an denen solche Namen übersetzt wurden, meldest du sie als Fehler.
- Eigene Funktions- und Variablennamen im Code bleiben so, wie sie im Projekt vorliegen; sie werden nicht nachträglich zwischen Deutsch und Englisch umbenannt.

Test: Normaler Start aus dem Mint-Menü → alle Texte deutsch, keine englischen Reste.

---

## 50. BUILD

```
meson setup build        (bzw. meson setup --reconfigure build)
ninja -C build
ninja -C build install
```

Nach jeder Änderung: kompilieren, Compiler- und Linkerfehler beheben, Warnungen prüfen, Anwendung starten, Funktion testen. Optionale Komponenten (z. B. Poppler, GStreamer) werden in Meson als optionale Abhängigkeiten behandelt.

---

### 50.1 Abhängigkeiten und Installationen

- Erlaubt sind ausschließlich die Bibliotheken und Werkzeuge aus Abschnitt 3.
- Du installierst keine Pakete (apt, pip, npm oder andere) und fügst keine neue Abhängigkeit in meson.build ein, ohne vorher zu fragen. Format:

```
NEUE ABHÄNGIGKEIT: …
WOFÜR: …
PFLICHT ODER OPTIONAL: …
OHNE SIE: …
ALTERNATIVE OHNE NEUE ABHÄNGIGKEIT: …
```

  Danach auf meine Entscheidung warten.
- Reine Build-Pakete (z. B. `libgtk-3-dev`, `libvte-2.91-dev`, `meson`) aus Abschnitt 3 dürfen in der Entwicklungsumgebung installiert werden, um kompilieren zu können; jede solche Installation wird im Änderungsprotokoll genannt.
- Alle nicht zwingend nötigen Komponenten sind in Meson optionale Abhängigkeiten (`required: false` bzw. Feature-Optionen). Pflicht sind nur: GTK3, GLib/GIO, GdkPixbuf, VTE 2.91.
- Die Datei `docs/DEPENDENCIES.md` listet alle Abhängigkeiten mit Paketnamen für Linux Mint und einem fertigen `sudo apt install …`-Befehl (Pflicht und optional getrennt). Sie wird bei jeder Änderung aktualisiert.
- Tritt ein Build-Fehler wegen einer fehlenden Komponente auf, installierst du nicht einfach etwas Neues, sondern meldest den Fehler und die Ursache.

---

## 51. RECHERCHE UND UMGANG MIT DATEIEN UND BILDERN

Vor Beginn einer Aufgabe wird festgestellt, welche Recherche nötig ist: Software (Code, Bibliotheken, APIs, Systemkomponenten), Dateien (Formate, Metadaten, Zusammenhänge), Bilder (Referenzen, technische Bilder), 3D/CAD (Formate, verfügbare Linux-Werkzeuge).

Bei technischen Dateien: Datei identifizieren, Format feststellen, Inhalt/Metadaten prüfen, Analysewerkzeuge bestimmen, externe Quellen nur ergänzend nutzen, Unsicherheiten kennzeichnen, keine Eigenschaften erfinden.

Bei mehreren technischen Lösungen vorher prüfen: vorhandene Mint-/GTK-Komponenten, Systemwerkzeuge, Lizenzen, Einschränkungen, Performance, Wartbarkeit.

Ergebnisse werden getrennt ausgewiesen: DATEIINFORMATION (aus der Datei), RECHERCHE (externe Quellen), REFERENZBILD, TECHNISCHE BEWERTUNG, UNSICHER.

Nie behaupten, etwas geprüft zu haben, was nicht geprüft wurde. Nicht erreichbare Ressourcen: „NICHT VERIFIZIERT" angeben.

---

## 52. NICHT BLOCKIERENDE GUI

Asynchron oder außerhalb des UI-Threads: große Verzeichnisse, Suche, Inhaltssuche, Ordnergrößen, Kopieren, Verschieben, Löschen, Vorschauen, Metadaten, Netzwerk-, Archiv-, Prüfsummen-, Geräte-, Sync-, Duplikat- und 3D-Operationen.

---

## 53. CODEQUALITÄT UND SICHERHEIT

### 53.1 Codequalität
C11-konform, modular, wartbar, GTK3- und GLib-konform, speichersicher. Achten auf NULL-Prüfungen, GObject-Lebenszyklen, Referenzzählung, Signalverbindungen, Speicherfreigabe, Thread-Sicherheit, Datei-, URI- und Berechtigungsfehler.

### 53.2 Fehlerbehandlung
Fehler werden erkannt, protokolliert, verständlich angezeigt und möglichst mit einer sinnvollen Aktion versehen. Keine stillen Fehler, keine Fake-Erfolgsmeldungen.

### 53.3 Sicherheit
Besondere Vorsicht bei Löschen, Berechtigungs- und Besitzänderungen, symbolischen Links, externen Befehlen, Skripten, ausführbaren Dateien, Archiven, Netzwerkressourcen, Verschlüsselung, Regeln und Stapelverarbeitung. Benutzereingaben nie unsicher in Shell-Befehle einbauen.

### 53.4 Zugangsdaten
Passwörter werden nie im Klartext gespeichert oder protokolliert.

### 53.5 Externe Befehle
Pfad und Verfügbarkeit prüfen, fehlendes Programm erkennen und melden, Argumente als Liste über GSubprocess übergeben (keine unnötige Shell), Exit-Code und stdout/stderr auswerten, Prozess abbrechbar machen, Fehler anzeigen.

---

## 54. TESTS

Jede Funktion wird getestet. Mindestens:

- Dateioperationen: kopieren, verschieben, umbenennen, löschen, Papierkorb, wiederherstellen, Rückgängig
- Navigation: Zurück, Vorwärts, übergeordnet, Pfad, URI, Tabs
- Split: vertikal, horizontal, Kopieren zwischen Bereichen
- Terminal: F4, Pfadwechsel, Befehl, schließen
- Suche: Name, Inhalt, Typ, Größe, Datum, Operatoren
- Vorschau: Bild, Text, PDF, unbekannter Typ, große Datei, ungültige Datei
- Geräte: erkennen, einbinden, aushängen
- Netzwerk: URI öffnen, fehlendes Backend, Verbindungsfehler
- Archive: erstellen, entpacken, testen
- 3D/CAD: STL, STEP, STP, FCStd erkennen, fehlendes Backend korrekt melden
- Einstellungen: Persistenz
- Sprache: alle Texte deutsch, keine englischen Reste (Abschnitt 49)
- Symbole: normale Symbole, Schloss nur bei fehlenden Rechten (Abschnitt 18.1)
- Eigenständigkeit: Build und Start ohne die optionalen Komponenten aus Abschnitt 3

Nach jeder größeren Implementierung:

- IMPLEMENTIERT: …
- GETESTET: …
- TESTERGEBNIS: …
- NOCH OFFEN: …

---

## 55. DEFINITION VON „FERTIG"

Eine Funktion ist nur dann IMPLEMENTIERT, wenn Code vorhanden ist, kompiliert, tatsächlich ausführbar ist, der Anwendungsfall getestet wurde, Fehlerfälle behandelt werden und keine Fake-Implementierung vorliegt. Ein Menüeintrag oder Button allein bedeutet nicht, dass die Funktion implementiert ist.

---

## 56. ABSCHLUSS-CHECKLISTE PRO ABSCHNITT

- [ ] Gesamtziel und relevante Anforderungen verstanden
- [ ] vorhandener Code, Dateien, Referenzen geprüft
- [ ] Implementierung durchgeführt
- [ ] alle neuen Texte auf Deutsch
- [ ] keine neue Abhängigkeit ohne Freigabe
- [ ] Funktion im richtigen Menü eingetragen (Abschnitt 11.1)
- [ ] Build erfolgreich, Warnungen geprüft
- [ ] Funktion gestartet und getestet, Fehlerfälle getestet
- [ ] Installation geprüft
- [ ] Dokumentation aktualisiert
- [ ] Status korrekt angegeben

---

## 57. VERBOTENE VORGEHENSWEISEN

- Code, APIs oder nicht existierende Dateien erfinden
- nicht getestete Funktionen als fertig markieren
- Anforderungen still entfernen oder ohne Meldung vereinfachen
- fremden Code kopieren
- Qt- oder KDE-Bibliotheken verwenden
- Pakete installieren oder Abhängigkeiten hinzufügen ohne meine Freigabe (Abschnitt 50.1)
- Abhängigkeiten zu Nemo oder Nemo-Paketen
- die deutsche Oberfläche auf Englisch oder gettext zurückbauen
- Pseudocode oder Platzhalter als fertige Funktion ausgeben
- Build- oder Testfehler ignorieren
- UI-Anforderungen eigenmächtig ändern
- Rechercheergebnisse als Dateieigenschaften ausgeben
- Vorschauen (Bild, PDF, 3D, CAD) vortäuschen
- mit einer späteren Phase beginnen, ohne dass ich die vorherige freigegeben habe
- weitere Hauptmenüs neben Datei, Bearbeiten, Ansicht, Gehe zu, Lesezeichen, Hilfe anlegen

Keine Begründung wie „zu komplex", „nicht notwendig" oder „das wäre einfacher" erlaubt das Weglassen. Stattdessen: analysieren, Einschränkungen dokumentieren, bei echtem Konflikt nachfragen.

---

## 58. UNKLARHEITEN, KONFLIKTE UND PRIORITÄTEN

Eindeutige Anforderung: umsetzen. Mehrdeutige Anforderung: Interpretation dokumentieren, bei wesentlichen Entscheidungen nachfragen.

Konfliktformat:

```
KONFLIKT: …
ANFORDERUNG A: …
ANFORDERUNG B: …
TECHNISCHE AUSWIRKUNG: …
VORSCHLAG: …
```

Dann auf meine Entscheidung warten.

Prioritäten: 1. meine ausdrücklichen Anforderungen, 2. tatsächliche Funktionalität, 3. Datenintegrität, 4. Stabilität, 5. korrekte Linux-Integration, 6. UI-Konsistenz, 7. Performance, 8. Erweiterbarkeit, 9. Codequalität.

---

## 59. GOLDENE REGEL

Ich entscheide, was Nolphin sein soll. Du analysierst, planst, implementierst, baust und testest. Du darfst technische Verbesserungen vorschlagen, aber keine Produktanforderung eigenständig entfernen, ersetzen oder abschwächen.

---

ENDE DES NOLPHIN MASTER DEVELOPMENT CONTRACT