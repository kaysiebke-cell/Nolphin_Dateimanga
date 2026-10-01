# Nolphin als Nemo-Ersatz – Bridge-Dokumentation

Diese Datei dokumentiert, wie Nolphin auf einem laufenden Linux Mint/Cinnamon-System
alle praktischen Funktionen von Nemo übernimmt, **ohne** Nemo zu deinstallieren.
Gedacht als Schritt-für-Schritt-Anleitung zum Nachvollziehen und Wiederholen
(z. B. auf einem anderen Rechner oder nach einer Neuinstallation).

## 1. Warum Nemo nicht deinstalliert wird

Das Debian/Ubuntu/Mint-Paket `cinnamon` (enthält die eigentlichen Shell-Binaries
wie `/usr/bin/cinnamon`, `cinnamon-settings` usw.) hat `nemo` als harte
Paketabhängigkeit:

```bash
apt-cache depends cinnamon | grep nemo
#   Hängt ab von: nemo
#   Hängt ab von: gir1.2-nemo-3.0
#   Hängt ab von: nemo-fileroller
```

`apt remove nemo` würde deshalb auch `cinnamon` selbst mit entfernen und die
Anmeldefähigkeit des Systems gefährden. **Es gibt keine Laufzeit-Prüfung**, die
Nemo zum Login erzwingt (das `org.cinnamon.desktop.session`-Schema hat in
aktuellen Cinnamon-Versionen kein `required-components`-Feld mehr) – das
Problem ist rein paketbasiert.

**Lösung:** Nemo bleibt installiert (erfüllt die Paketabhängigkeit), wird aber
durch Umkonfiguration nie mehr tatsächlich aufgerufen. Nolphin übernimmt alle
Funktionen parallel.

## 2. Voraussetzung: Nolphin ist gebaut und installiert

```bash
cd /home/kaysiebke/nolphin-file-manager-master
meson setup build          # einmalig, oder: meson setup --reconfigure build
ninja -C build
sudo ninja -C build install
```

Installiert u. a. `/usr/local/bin/nolphin`, `/usr/local/bin/nolphin-desktop`,
`/usr/local/share/applications/org.nolphin.FileManager.desktop` und
`/usr/local/share/applications/nolphin-autostart.desktop`, kompiliert die
GSettings-Schemas neu und aktualisiert Icon-Cache/MIME-Datenbank.

## 3. Desktop-Icons: Nolphin statt Nemo

Nemo-Desktop läuft normalerweise über einen XDG-Autostart-Eintrag
(`/usr/share/applications/nemo-autostart.desktop`, `OnlyShowIn=X-Cinnamon`,
`AutostartCondition=GSettings org.nemo.desktop show-desktop-icons`).
Nolphin hat ein exaktes Äquivalent unter `data/nolphin-autostart.desktop`.

```bash
# 1. Nolphins Autostart-Eintrag an die Stelle kopieren, die XDG tatsächlich liest
#    (NICHT /usr/local/share/applications/ – das wird nur für den App-Starter
#    gescannt, nicht für Autostart)
mkdir -p ~/.config/autostart
cp /home/kaysiebke/nolphin-file-manager-master/data/nolphin-autostart.desktop \
   ~/.config/autostart/nolphin-autostart.desktop

# 2. Nemos Desktop-Icon-Verwaltung abschalten (NICHT Nemo deinstallieren)
gsettings set org.nemo.desktop show-desktop-icons false
```

`nolphin-desktop` hat eine eingebaute Kollisionserkennung: Es prüft via
`_NET_WM_WINDOW_TYPE_DESKTOP`-Fenstern (WM_CLASS-basiert), ob bereits ein
anderes Programm den Desktop verwaltet, und tritt sonst automatisch zurück
(siehe GSettings-Key `org.nolphin.desktop ignored-desktop-handlers`). Dadurch
ist die Reihenfolge der obigen zwei Schritte unkritisch – mit aktivem Nemo
bleibt Nolphin inaktiv, bis Schritt 2 ausgeführt wird.

Wirkt **erst nach Ab-/Neuanmelden** vollständig (XDG-Autostart wird nur beim
Sitzungsstart ausgewertet). Für einen sofortigen Test ohne Neustart:

```bash
pkill -f nolphin-desktop   # falls schon was läuft
nohup /usr/local/bin/nolphin-desktop >/tmp/nolphin-desktop.log 2>&1 &
disown
```

## 4. Standard-Dateimanager: Nolphin statt Nemo

```bash
xdg-mime default org.nolphin.FileManager.desktop inode/directory application/x-directory
```

Test: `gio open ~` sollte jetzt Nolphin statt Nemo öffnen.

## 5. Manuelle Schritte (nicht automatisierbar)

Taskleisten-/Menü-Favoriten, die Nemo direkt angepinnt haben (z. B. im
`grouped-window-list`-Applet, Datei
`~/.config/cinnamon/spices/grouped-window-list@cinnamon.org/<instanz>.json`,
Feld `pinned-apps`/`value`), zeigen weiterhin auf `nemo.desktop`. Das lässt
sich nicht gefahrlos automatisiert umschreiben (laufendes Cinnamon besitzt
diese Datei) – stattdessen per Rechtsklick auf das Icon in der Taskleiste
→ „Lösen", dann Nolphin manuell anpinnen.

## 6. Bekannter Bug und Fix: Arbeitsbereich-Leiste auf dem Desktop

**Symptom:** Nach Aktivierung von `nolphin-desktop` erschien die rechte
Arbeitsbereich-Leiste (Vorschau/Eigenschaften) auf dem Desktop selbst, neben
den Desktop-Icons.

**Ursache:** `NolphinDesktopWindow` erbt von `NolphinWindow`
(`src/nolphin-desktop-window.c`, `G_DEFINE_TYPE (..., NOLPHIN_TYPE_WINDOW)`).
Die gemeinsame `nolphin_window_constructed()`-Funktion ruft unbedingt
`nolphin_window_set_show_preview()` auf, gesteuert vom GSettings-Default
`start-with-preview = true` – das trifft dadurch auch das Desktop-Fenster,
das dafür nie gedacht war.

**Fix** (`src/nolphin-window.c`, in `nolphin_window_constructed()`):

```c
/* Die Arbeitsbereich-Leiste gehört nur ins normale Datei-Fenster -
 * das Desktop-Fenster (NolphinDesktopWindow erbt von NolphinWindow)
 * zeigt sonst Vorschau/Eigenschaften eines "x-nolphin-desktop"-
 * Objekts über den eigentlichen Desktop-Icons an. */
if (!NOLPHIN_IS_DESKTOP_WINDOW (window)) {
    nolphin_window_set_show_preview (window,
                                     g_settings_get_boolean (nolphin_window_state,
                                                              NOLPHIN_WINDOW_STATE_START_WITH_PREVIEW));
}
```

Das etablierte Muster `if (!NOLPHIN_IS_DESKTOP_WINDOW (window))` existierte im
selben File bereits an zwei anderen Stellen (Zeile ~394, ~2953) für
vergleichbare "nur im echten Fenster"-Ausnahmen.

**Allgemeine Lehre für künftige Features:** Jede neue Funktion, die in
`nolphin_window_constructed()` oder einer anderen von `NolphinDesktopWindow`
geerbten Stelle UI-Chrome hinzufügt (Leisten, Panels, Menüs), muss explizit
auf `NOLPHIN_IS_DESKTOP_WINDOW (window)` prüfen, sonst taucht sie
unbeabsichtigt auch auf dem Desktop auf.

## 7. Notfall-Wiederherstellung

Falls nach einem Neustart etwas nicht stimmt: Strg+Alt+F3 (reine
Text-Konsole, kein Cinnamon nötig), dort einloggen, dann:

```bash
gsettings set org.nemo.desktop show-desktop-icons true
rm ~/.config/autostart/nolphin-autostart.desktop
xdg-mime default nemo.desktop inode/directory application/x-directory
# zurück zur grafischen Oberfläche: Strg+Alt+F1 (oder F2)
```

Nemo war zu keinem Zeitpunkt deinstalliert; diese drei Befehle stellen den
Zustand von vor der Bridge-Aktivierung exakt wieder her. Nur falls das
Nemo-Paket selbst beschädigt sein sollte (normalerweise nicht nötig):

```bash
sudo apt install --reinstall nemo nemo-data nemo-fileroller
```

## 8. Lektion zu Projekt-Ordnern (Mehrfachkopien vermeiden)

Bei der Umsetzung stellte sich heraus, dass auf diesem Rechner mehrere
unabhängige Klone dieses Repos existierten (u. a. unter `~/nolphin-file-manager`,
dem inzwischen entfernten `~/Dropbox/Desktop/nolphin-file-manager-master` und
einem unter anderem GitHub-Namen laufenden `~/Downloads/nolphin-file-manager`),
die jeweils unterschiedlich weit auseinandergelaufen waren. Das führte dazu,
dass Änderungen aus einer Sitzung in einer anderen nicht sichtbar waren.

**Vor jeder neuen Arbeitssitzung prüfen:**

```bash
pwd
git remote -v
git log --oneline -1
git status --short | wc -l
```

Das verbindliche Arbeitsverzeichnis ist seit dieser Aufräumaktion
ausschließlich `/home/kaysiebke/nolphin-file-manager-master` (außerhalb von
Dropbox, direkt unter `$HOME`).
