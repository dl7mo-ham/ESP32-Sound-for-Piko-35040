# Gehaeuse fuer das ESP32 Sound Mini C3 (ESP32-C3 Pro Mini, MAX98357A umgedreht auf der Versorgungsreihe)
# Parametrisch mit CadQuery. Aufruf: python gehaeuse.py [Ordner] [--pruefen]  -> STL/STEP
#
# Aufbau (Martins Foto vom 12.11.2026): Der MAX98357A sitzt mit der Bauteilseite zum ESP auf dessen
# Rueckseite, seine 7-polige Stiftleiste steckt in 5V, GND, 3V3, IO4..IO1 (IO0 frei). 2,5 mm Abstand
# (Kunststoff der Stiftleiste). Der MAX deckt die ganze Rueckseite bis IO1 ab, deshalb sind alle
# Kabel (Port A: 5V, GND, IO20, IO21; Port B: IO5, IO6) auf der Bauteilseite des ESP angeloetet.
# Lautsprecher am JST-PH 2-pol. (liegend) auf der Aussenseite des MAX.
#
# Modell-Koordinaten: x entlang ESP (0 = Platinenkante am USB-Ende), z quer (+ = Versorgungsreihe
# 5V/GND/3V3/IO4..IO0, - = Reihe IO5..IO21), y = Hoehe, ESP-Platine y 0..1, Bauteilseite des ESP
# bei y < 0, MAX-Seite bei y > 0. Im Gehaeuse liegt die ESP-Bauteilseite OBEN:
#   Gehaeuse X = x, Y = z, Z = Z_OFF - y   (reine Drehung, keine Spiegelung)
# Von oben gesehen, USB links: Versorgungsreihe hinten (+Y), BOOT-Taste vorne links.
import os, sys
import cadquery as cq

# ---------------- Parameter (alles in mm) ----------------
WAND    = 1.6     # Wandstaerke
BODEN   = 1.6     # Bodenstaerke
DECKEL  = 1.6     # Deckelstaerke
LIPPE   = 2.0     # Hoehe der Klemmlippe am Deckel
TOL     = 0.15    # Spiel Lippe <-> Innenwand (bei strammem Sitz auf 0,2 erhoehen)

# ESP32-C3 Pro Mini (aus Produktfoto geschaetzt, bitte nachmessen)
ESP_L, ESP_B = 23.4, 17.8          # Laenge (USB -> Antenne), Breite
P = 2.54
PIN0 = 1.1                         # Mitte Pin 1 (5V bzw. IO5) ab USB-Kante
def px(k): return PIN0 + P * k     # Pinraster, k = 0 am USB-Ende
ROW_V, ROW_IO = 7.62, -7.62        # Versorgungsreihe / IO-Reihe
# MAX98357A (Klon, lila) 17,8 entlang der Stiftleiste, 19,3 quer, Leiste 1,3 vom Rand
MAX_X  = (px(0) - 1.27, px(0) - 1.27 + 17.8)
MAX_Z  = (ROW_V + 1.3 - 19.3, ROW_V + 1.3)
MAX_Y  = (3.5, 5.1)                # unter dem 2,5-mm-Kunststoff, Platine 1,6

# Innenraum in Modell-Koordinaten
X_MIN, X_MAX = -1.7, 26.0          # USB-C ragt bis -1,5; am Antennen-Ende Platz fuer die Kabelboegen
Z_MIN, Z_MAX = -11.0, 10.2         # MAX/JST bis -10,8; Kabel 5V/GND an der Versorgungsreihe
Y_TOP  = -5.0                      # Deckel-Unterseite: USB-Stecker (6,7 mm hoch) muss passen
Y_BOT  = 9.6                       # Boden-Innenseite: JST-PH liegend endet bei 9,1
Z_OFF  = Y_BOT + BODEN             # Gehaeuse-Z = Z_OFF - y

USB_X  = (-1.5, 5.6)               # USB-C-Buchse auf der Platine
USB_Z  = (-6.4, 6.4)               # USB-C-Stecker 12,5 breit, Mitte z = 0
USB_Y  = (-4.95, 1.75)             # Buchse y -3,2..0, Stecker-Umspritzung 6,7 hoch
SPK_X  = (6.4, 13.6)               # JST-PH 2-pol liegend am MAX, Stecker ca. 6 breit
SPK_Y  = (4.7, 9.6)
JST    = (7.0, 13.0)               # Gehaeuse des JST (x)
BOOT   = (8.9, -3.0)               # (x, z) Taster BOOT (= WLAN-Taste, IO9)
RESET  = (8.9, 2.6)                # Taster RST
LED    = (6.1, -6.3)               # LED (IO8), Lage aus dem Produktfoto geschaetzt
TAST_OBEN = -2.05                  # Oberkante Tasterkappe (y)
ANT    = (21.0, 23.4, -8.5, 2.2)   # Keramikantenne x0, x1, z0, z1

# Kabelschlitze an der Antennen-Stirnseite (z-Bereich), unten offen bis y = SCHLITZ_Y
SCHLITZ_A = (-3.6, 3.6)            # Port A: 4 Adern (5V, GND, IO20, IO21)
SCHLITZ_B = (-10.4, -7.0)          # Port B: 2 Adern (IO5, IO6)
SCHLITZ_Y = -0.6

def box(x0, y0, z0, x1, y1, z1):
    return cq.Workplane("XY").box(x1 - x0, y1 - y0, z1 - z0, centered=False).translate((x0, y0, z0))

def mbox(x0, x1, ya, yb, za, zb):
    """Quader in Modell-Koordinaten (x, y, z) -> Gehaeuse."""
    return box(x0, min(za, zb), Z_OFF - max(ya, yb), x1, max(za, zb), Z_OFF - min(ya, yb))

def rbox(x0, y0, z0, x1, y1, z1, r):
    return (cq.Workplane("XY").workplane(offset=z0).center((x0 + x1) / 2, (y0 + y1) / 2)
            .rect(x1 - x0, y1 - y0).extrude(z1 - z0).edges("|Z").fillet(r))

def cyl(x, y, z0, z1, r):
    return cq.Workplane("XY").workplane(offset=z0).center(x, y).circle(r).extrude(z1 - z0)

# Gehaeuse-Koordinaten
IX0, IX1, IY0, IY1 = X_MIN, X_MAX, Z_MIN, Z_MAX
OX0, OX1, OY0, OY1 = IX0 - WAND, IX1 + WAND, IY0 - WAND, IY1 + WAND
H_BASE = Z_OFF - Y_TOP             # Hoehe Unterteil
ECKE_R = 2.5
def gz(y): return Z_OFF - y

def unterteil():
    b = rbox(OX0, OY0, 0, OX1, OY1, H_BASE, ECKE_R)
    b = b.cut(box(IX0, IY0, BODEN, IX1, IY1, H_BASE + 1))
    # Stuetzen: unter dem MAX (frei zwischen Stiftleiste und JST) und unter dem ESP-Ende hinter dem MAX
    b = b.union(mbox(1.6, 5.8, MAX_Y[1], Y_BOT + 0.1, -3.5, 4.5))
    b = b.union(mbox(19.4, 22.8, 1.0, Y_BOT + 0.1, -5.0, 5.0))
    # USB-C: Kerbe in der Stirnwand, oben offen (der Deckel schliesst sie)
    b = b.cut(box(OX0 - 1, USB_Z[0], gz(USB_Y[1]), IX0 + 0.5, USB_Z[1], H_BASE + 1))
    # Lautsprecher-Stecker JST-PH durch die Laengswand (Seite IO-Reihe)
    b = b.cut(box(SPK_X[0], OY0 - 1, gz(SPK_Y[1]), SPK_X[1], IY0 + 0.5, gz(SPK_Y[0])))
    # Kabelschlitze Port A und Port B an der Antennen-Stirnseite, oben offen
    for z0, z1 in (SCHLITZ_A, SCHLITZ_B):
        b = b.cut(box(IX1 - 0.5, z0, gz(SCHLITZ_Y), OX1 + 1, z1, H_BASE + 1))
    # Rastnuten fuer die Deckelnasen (lange Seiten)
    for y in (IY0, IY1):
        b = b.cut(cq.Workplane("YZ").workplane(offset=6.0).center(y, H_BASE - LIPPE / 2 - 0.1)
                  .circle(0.5).extrude(12.0))
    # Befestigungslaschen an den Stirnseiten (Schraube M2 oder Kabelbinder)
    for x, s in ((OX0, -1), (OX1, 1)):
        t = rbox(x - 6 if s < 0 else x - 1, -4.0, 0, x + 1 if s < 0 else x + 6, 4.0, 1.8, 1.5)
        b = b.union(t).cut(cyl(x + s * 3.3, 0, -1, 4, 1.2))
    # Beschriftung unten (0,5 mm vertieft, gespiegelt, damit sie von unten lesbar ist)
    def txt(s, x, y, size):
        return (cq.Workplane("XY").workplane(offset=0.5).center(x, y)
                .transformed(rotate=(180, 0, 0)).text(s, size, 1.0, halign="center", valign="center"))
    for s, x, y, sz in [("SOUND MINI C3", (IX0 + IX1) / 2, 0.0, 2.4)]:
        b = b.cut(txt(s, x, y, sz))
    return b

def deckel():
    d = rbox(OX0, OY0, 0, OX1, OY1, DECKEL, ECKE_R)
    # Klemmlippe, an USB-Kerbe und Kabelschlitzen ausgespart
    lip = box(IX0 + TOL, IY0 + TOL, -LIPPE, IX1 - TOL, IY1 - TOL, 0.01)
    lip = lip.cut(box(IX0 + TOL + 1.0, IY0 + TOL + 1.0, -LIPPE - 1, IX1 - TOL - 1.0, IY1 - TOL - 1.0, 1))
    lip = lip.cut(box(IX0 - 1, USB_Z[0] - 0.3, -LIPPE - 1, IX0 + 2, USB_Z[1] + 0.3, 1))
    for z0, z1 in (SCHLITZ_A, SCHLITZ_B):
        lip = lip.cut(box(IX1 - 2, z0 - 0.3, -LIPPE - 1, IX1 + 1, z1 + 0.3, 1))
    # Rastnasen
    for y in (IY0 + TOL, IY1 - TOL):
        lip = lip.union(cq.Workplane("YZ").workplane(offset=7.0).center(y, -LIPPE / 2 - 0.1)
                        .circle(0.35).extrude(10.0))
    d = d.union(lip)
    # Niederhalter: auf die USB-Buchse (Oberkante y -3,2) und auf die Keramikantenne (Oberkante ca. y -1,0)
    d = d.union(box(1.0, -2.5, -(-3.2 - Y_TOP - 0.15), 4.6, 2.5, 0.01))
    d = d.union(box(21.6, -5.5, -(-1.0 - Y_TOP - 0.15), 22.8, -1.5, 0.01))
    # BOOT (WLAN) und RST: Fuehrungsrohre, mit Zahnstocher/Bueroklammer druecken
    for (x, z) in (BOOT, RESET):
        unten = -(TAST_OBEN - Y_TOP) + 0.5          # 0,5 mm ueber der Tasterkappe
        d = d.union(cyl(x, z, unten, 0.01, 1.7)).cut(cyl(x, z, unten - 1, DECKEL + 1, 1.05))
    # LED: Lichtrohr bis kurz ueber die Platine
    d = d.union(cyl(LED[0], LED[1], -(-1.6 - Y_TOP), 0.01, 1.7)).cut(cyl(LED[0], LED[1], -5, DECKEL + 1, 1.15))
    # Beschriftung (0,5 mm vertieft)
    def txt(s, x, y, size, rot=0):
        return (cq.Workplane("XY").workplane(offset=DECKEL - 0.5).center(x, y)
                .transformed(rotate=(0, 0, rot)).text(s, size, 1.0, halign="center", valign="center"))
    for s, x, y, sz, r in [("WLAN", BOOT[0] + 5.6, BOOT[1] - 0.6, 1.8, 0), ("RST", RESET[0] + 4.8, RESET[1] + 0.6, 1.8, 0),
                           ("LED", LED[0] - 1.6, LED[1] - 3.0, 1.6, 0), ("USB", IX0 + 1.4, 0, 2.0, 90),
                           ("SPK", SPK_X[1] - 1.0, IY0 + 1.6, 1.8, 0),
                           ("A", IX1 - 1.6, 0, 2.2, 90), ("B", IX1 - 1.6, (SCHLITZ_B[0] + SCHLITZ_B[1]) / 2, 2.0, 90)]:
        d = d.cut(txt(s, x, y, sz, r))
    return d

def baugruppe():
    """Vereinfachter Aufbau fuer Vorschau und Kollisionskontrolle (Modell-Koordinaten)."""
    t = {}
    t["esp"] = mbox(0, ESP_L, 0, 1, -ESP_B / 2, ESP_B / 2)
    t["usb"] = mbox(USB_X[0], USB_X[1], -3.2, 0, -4.5, 4.5)
    t["chip"] = mbox(10.5, 15.5, -0.9, 0, -2.5, 2.5)
    t["boot"] = mbox(BOOT[0] - 1.5, BOOT[0] + 1.5, TAST_OBEN, 0, BOOT[1] - 2.2, BOOT[1] + 2.2)
    t["reset"] = mbox(RESET[0] - 1.5, RESET[0] + 1.5, TAST_OBEN, 0, RESET[1] - 2.2, RESET[1] + 2.2)
    t["led"] = mbox(LED[0] - 0.5, LED[0] + 0.5, -0.6, 0, LED[1] - 0.8, LED[1] + 0.8)
    t["ant"] = mbox(ANT[0], ANT[1], -1.0, 0, ANT[2], ANT[3])
    t["ipex"] = mbox(19.8, 23.0, -1.2, 0, 3.3, 6.3)
    t["xtal"] = mbox(16.9, 19.6, -0.8, 0, -4.9, -1.8)
    t["hdr"] = mbox(px(0) - 1.27, px(6) + 1.27, 1.0, 3.5, ROW_V - 1.25, ROW_V + 1.25)
    for k in range(7):
        if k != 2:   # SD-Stift weggelassen (3V3 frei)
            t["pv%d" % k] = mbox(px(k) - 0.32, px(k) + 0.32, -1.0, 6.0, ROW_V - 0.32, ROW_V + 0.32)
    t["max"] = mbox(MAX_X[0], MAX_X[1], MAX_Y[0], MAX_Y[1], MAX_Z[0], MAX_Z[1])
    t["maxic"] = mbox(6.0, 9.0, 2.6, 3.5, -2.0, 1.0)
    t["jst"] = mbox(JST[0], JST[1], MAX_Y[1], MAX_Y[1] + 4.0, -10.8, -4.6)
    r = None
    for v in t.values():
        r = v if r is None else r.union(v)
    return r

if __name__ == "__main__":
    args = [a for a in sys.argv[1:] if not a.startswith("--")]
    out = args[0] if args else "export"
    os.makedirs(out, exist_ok=True)
    u, d = unterteil(), deckel()
    print("Aussenmasse: %.1f x %.1f x %.1f mm (mit Deckel, ohne Laschen)" % (OX1 - OX0, OY1 - OY0, H_BASE + DECKEL))
    cq.exporters.export(u, f"{out}/Gehaeuse_Unterteil.stl", tolerance=0.02, angularTolerance=0.1)
    cq.exporters.export(u, f"{out}/Gehaeuse_Unterteil.step")
    d_druck = d.rotate((0, 0, 0), (1, 0, 0), 180)   # Deckel mit der Oberseite aufs Druckbett
    cq.exporters.export(d_druck, f"{out}/Gehaeuse_Deckel.stl", tolerance=0.02, angularTolerance=0.1)
    cq.exporters.export(d, f"{out}/Gehaeuse_Deckel.step")
    if "--pruefen" in sys.argv:
        bg = baugruppe()
        dd = d.translate((0, 0, H_BASE))
        print("Kollision Baugruppe/Unterteil: %.3f mm3, Baugruppe/Deckel: %.3f mm3"
              % (bg.intersect(u).val().Volume(), bg.intersect(dd).val().Volume()))
