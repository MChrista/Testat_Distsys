
Hier ein paar Hinweise zum Programmentwurf Support-Addon
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~


=== Allgemein ===

Im Verzeichnis 't' befinden sich Perl-Testskripte. Diese
können mit 'prove' gestartet werden, nachdem evtl. fehlende
Perlmodule installiert sind; Installationshinweise siehe unten.

Die Installationshinweise habe ich basierend auf einer neuen
Ubuntu 14.04 Installation erstellt. Damit waren dann alle
benötigten Perlmodule vorhanden. Wenn es trotzdem Probleme geben
sollte, bitte einfach per Email melden.

Die Testfälle in den Skripten sind in Tabellen zusammengefasst.
Diese Tabellen können für eigene Tests erweitert oder am Anfang
der Entwicklung einzelne Tests auskommentiert werden.
Die Testskripte verbinden sich auf 'localhost:8080'. Mit einer
kleinen Änderung funktionieren die Testskripts auch mit dem
Apache Server meiner Homepage. Dazu ist aber wichtig, dass die
im Skript aufgerufenen Dateien auf der Homepage und die lokal
im Verzeichnis web identisch sind.

Das Skript run.sh startet den Server mit Port 8080 und Verzeichnis
web. Logs werden auf die Standardausgabe geschrieben. Damit muss
nicht explizit der build-Pfad immer mit angegeben werden.

Für den ersten Test Ihres Programmentwurfs führe ich die folgenden
Schritte aus:

> make clean
> make
> ./run.sh >>log.txt 2>&1 &
> prove


Im Verzeichnis web/cgi-bin befindet sich nun ein weiteres Perl
CGI Skript, welches einen größeren, zufällig erzeugten Textblock
zurückliefert (80.000.000 Bytes im Message Body). Dieses Skript
setzt 'Content-Length'.

Anwendungsbeispiel:

> wget -S localhost:8080/cgi-bin/chargen.pl



=== Korrekturen in der Aufgabenstellung ===
Tabelle 1: Option -f (wie in tinyweb.c) anstatt -l
Tabelle 3: Status Code 403 (Forbidden) anstatt 401
Tabelle 3: Status Code 416 anstatt 406



=== Installationshinweise ===


CPAN ist der Paketmanager von Perl. Er wird gestartet mit:

> sudo cpan



Die Testscripte verwenden verschiedene Perlmodule, die bei
einer Standardinstallation nachinstalliert werden müssen.

cpan> install File::MimeInfo IO::Socket::IP POSIX::strptime LWP::UserAgent



=== CURL ===

HTTP Requests können auch mit dem Tool curl an den Server
gesendet werden. Hierzu habe ich das Skript curltest.sh mit
einem einfachen Beispiel beigefügt. Info zum Programm und den
zahlreichen Optionen gibt die man-page.
Für das Skript 'curltest.sh' muss evtl. das Paket curl
nachinstalliert werden:

> sudo apt-get install curl



=== Zeitzonen und Datum einlesen ===

In den Request- und Response-Header werden die Zeiten in der
Zeitzone GMT angegeben.
Geben Sie die Zeitzone bei strftime nicht mit "%Z" aus, sondern
geben Sie stattdessen "GMT" als Zeichenkette aus. Je nach
Systemkonfiguration vermeiden Sie damit den Fall, das bei "%Z"
"UTC" anstatt "GMT" ausgegeben wird und deshalb evtl. ein
Vergleich der Zeitstempel mit strncmp fehlschlägt.

Die Funktion strptime berücksichtigt nicht die Zeitzone, sondern
füllt mit den Daten erstmal nur eine Struktur struct tm. Die
Interpretation, welche Zeitzone denn verwendet werden soll, ist
aber wichtig, wenn Sie diese Struktur in ticks umwandeln.
Verwenden Sie hier timegm anstatt mktime, um GMT zu erzwingen.

    if (strptime(s, "%a, %d %b %Y %H:%M:%S", tm_ptr) != NULL) {
        ticks = timegm(tm_ptr);
    } else {
        fprintf(stderr, "Warning: cannot parse time\n");
    } /* end if */



Anzeigen der Zeitzone
> cat /etc/timezone


Ändern der Zeitzone
> sudo dpkg-reconfigure tzdata


