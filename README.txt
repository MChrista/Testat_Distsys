Autoren: Michael Christa, Florian Hink
Datum: 15.07.2015
Letzte Änderung: 15.07.2015

Arbeitsaufteilung:
Michael Christa hat den Parser bearbeitet und hat dann in der Implementierung der Tinyweb unterstützt.
Florian Hink entwickelte eine Struktur für die Tinyweb und implementierte dort. 

Hinweise zum Programm:
- Es wurde ein HTTP Server entwickelt.
- Es werden nicht alle mallocs überprüft
Parser:
- Statusline, cgi und range werden anhand von regulären Ausdrücken geparst.
	-> Damit ist die Struktur für strtok klar

Tinyweb:
- Es wird ein Socket aufgemacht bei dem auf Verbindungen von Clients angenommen werden.
- In der handle_client wird nach der Art des Requests unterschieden.
- Es wird eine Antwort entsprechend des Requests generiert und zurückgeschickt.


Verwendete Quellen:
Man pages:
- man 2 bind
- man 3 getaddrinfo
- man 3 getopt
- man 2 fork
- man 2 accept
- man 2 open
- man 2 close
- man 3 fopen
- man 2 stat
- man 2 signal
- man 2 waitpid
- man 2 read
- man 3 strcpy
- man 3 strchr
- man 3 memcpy
- man 2 send
- man 3 strcat

Absprachen:
Rücksprachen mit den Gruppen 1 und 4. 
