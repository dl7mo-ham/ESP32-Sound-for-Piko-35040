/*
 * fork_version.h  -  Piko-35040-Fork (projektlokal)
 *
 * Der Fork hat eine eigene Versionsachse: `<upstream-Basis>+dl7mo.<Zaehler>`.
 * Sie steht nur in docs/fork-changelog.md und in der README-Fork-Sektion.
 * Die `Version`-Zeile im Sketch bleibt dagegen upstream-identisch (v0.15),
 * damit Upstream-Updates dort nicht haengen bleiben.
 *
 * CRSF braucht ein wechselndes Byte als "Parameter Version Number", damit
 * Konfiguratoren (z. B. TBS Agent) eine gecachte Parameterliste verwerfen.
 * Dieses Byte kommt ab jetzt aus dieser Datei und nicht mehr aus `Version`.
 * Der Bereich 200..255 ist gegenueber upstreams major*100+minor (0..199)
 * reserviert. FORK_SEQ bei jeder Aenderung UND bei jedem Upstream-Merge
 * erhoehen - dann aendert sich der Token immer, ohne eine Upstream-Datei
 * anzufassen. Muss < 56 bleiben, damit 200 + FORK_SEQ <= 255 gilt.
 */
#ifndef FORK_VERSION_H
#define FORK_VERSION_H

#define FORK_SEQ 11
#define APP_PARAM_VERSION (200 + FORK_SEQ)   // aktuell 211

// Nur informativ (z. B. fuer serielle Ausgabe); passt nicht in
// char versionString[6] und wird deshalb nicht ueber Web/Lua angezeigt.
#define FORK_VERSION_LABEL "v0.15+dl7mo.11"

#endif
