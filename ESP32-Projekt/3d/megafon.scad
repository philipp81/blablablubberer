// Blablablubber Megafon – parametrisches 3D-Modell (OpenSCAD), Stand 6: Lauf hinten offen, Rückplatte mit Mikrofon, ESP32 als Schublade
//
// Koordinaten: X nach vorn (Richtung Trichter), Y seitlich (+Y links, von hinten gesehen; x, y, z rechtshändig), Z nach oben.
// Teile:  part = "koerper"      Lauf (hinten offen) mit Batteriefach, Schlitten für den ESP32, Deck für den MT3608 + Pistolengriff
//         part = "haube"        obere Schale des Laufs, abnehmbar; darunter liegen die drei Zellen; trägt den gelben Taster
//         part = "rueckplatte"  verschließt das Laufende, 4 × M3 x 10; trägt das Mikrofon (Kapsel bündig) und den USB-C-Schlitz
//         part = "kegel"        Schalltrichter, 3 × M3 x 10; sichert vorn zusätzlich die Haube
//         part = "griffschale"  linke Seitenschale des Griffs (Fenster zu Abzug und Verstärker), 4 × M3 x 10
//         part = "tastermutter" drei gedruckte Muttern für die 7-mm-Taster (schneiden sich selbst ein)
//         part = "alle"  zusammengebaut,  part = "offen"  auseinandergezogen,  part = "innen"  von oben ohne Haube,
//         part = "einschub"  von hinten, Rückplatte ab und ESP32 halb eingeschoben,  part = "zellen" / "schnitt" / "griff"  Bilder für die Anleitung,
//         part = "none"  nichts (Prüfskripte)
// Alle Maße in mm. Bauteilmaße vom 24.10.2026 nachgemessen.

part = "alle";
$fn = 96;

// ---------------- Lauf ----------------
wall          = 3;
barrel_r      = 36;      // Außenradius, Ø 72
barrel_len    = 100;     // Lauf von x = -barrel_len bis x = 0
front_wall    = 3;
cone_screw_x  = -12;
cone_screw_a  = [0, 120, 240];   // drei Schrauben M3 im Kragen
badge_d       = 30;

// ---------------- Haube ----------------
hood_x        = [-93, -16];   // Öffnung im Lauf (X)
hood_half_deg = 48;           // halber Öffnungswinkel um die Oberseite, ergibt ca. 54 mm Breite außen
lip_w         = 2;            // Auflagelippe rundum (innerer Teil der Wand bleibt stehen)
lip_r         = 34.5;         // Lippe von r = 33 bis 34,5; Haube außen 34,5..36, innen 32..34,5
hood_gap      = 0.3;
snap_r        = 1.0;          // Rastnasen (Kugeln) an der Haube
snap_x        = [-72, -34];   // zwei Nasen je Seite
hood_screw_x  = -90;          // Sicherungsschraube M3 x 12 hinten, durch die Haube in den Block / die hintere Stirnwand des Batteriefachs

// ---------------- Rückplatte ----------------
rear_t        = 4;            // Platte, sitzt im offenen Laufende, außen bündig
rear_fit      = 0.2;          // radiales Spiel
rear_screws   = [[26, 11], [-26, 11], [10, -27], [-10, -27]];   // (y, z) der vier M3 x 10, Pfosten im Lauf
rear_boss_d   = 7;  rear_boss_len = 8;

// ---------------- Batteriefach: 3 × AA im Dreieck, gedruckt, Kontakte selbst gebaut ----------------
aa_d = 14.5; aa_len = 50.5; cell_pitch = 15.5;
pack_in_x     = [-86, -32];   // Innenflächen der beiden Stirnwände (54 mm: Zelle 50,5 + Feder + Spiel; Taschen geben je 1,5 mm dazu)
pack_wall_t   = 3;            // Stirnwände
pocket_w      = 12;  pocket_d = 1.5;   // Kontakttasche je Zellenende in der Stirnwand-Innenseite (12 x 12, zwischen A/B und B/C bleibt ein 1,4-mm-Steg)
groove_w      = 4;   groove_d = 1.0;   // Nut für die Brückenstreifen zwischen zwei Taschen
wire_hole_d   = 3.2;
cell_z        = 10;           // Achse der beiden unteren Zellen (A rechts, C links, wie gehalten); B liegt oben im Tal
cells = [[-cell_pitch / 2, cell_z], [0, cell_z + cell_pitch * sin(60)], [cell_pitch / 2, cell_z]];   // A, B, C (y, z)
rib_x         = [pack_in_x[0] + 6, pack_in_x[1] - 6];   // zwei U-Rippen unter A und C
rib_t         = 3;
rib_z0        = -1;           // Unterkante der Rippen und Stirnwände, 2 mm über dem USB-C-Stecker des ESP32
cell_fit      = 0.25;

// ---------------- Lautsprecher (gemessen) ----------------
speaker_d = 28; speaker_depth = 9; speaker_open = 24; speaker_clamp_pcd = 33;   // 3 × M3 mit Unterlegscheibe klemmen den Rand

// ---------------- Elektronik (gemessen) ----------------
pcb_t         = 1.6;
// ESP32 DevKitC (30 Pins): liegt mit der Antenne nach vorn, USB-C nach hinten; Stiftleisten zeigen nach unten und hängen neben den Schienen
esp_size      = [54, 29];  esp_x = -67;  esp_y = -14;  esp_z = -8;   // esp_z = Unterseite der Platine; USB-C ragt 1,5 mm über die Hinterkante
esp_pins      = [0.4, 2.9];   // Stiftleiste: Abstand von der Längskante
esp_pin_len   = 8.1;          // Kunststoff + Stifte unter der Platine
esp_pin_x     = [4.1, 42.2];  // Stiftleisten ab der Antennenkante; die hinteren 11,8 mm sind frei (EN/BOOT-Taster, USB-C)
esp_top_h     = 3.3;          // USB-C und Modul über der Platinenoberseite
esp_rail_in   = 3.5;          // Auflageschienen 3,5..6,5 mm innerhalb der Längskanten
boost_size    = [31, 18];  boost_x = -56; boost_y = 18; boost_z = -18; boost_h = 7;   // MT3608 (XY-016) auf dem linken Deck, Trimmer nach hinten links
boost_deck_y  = [5, 30];   deck_t = 2.5;
amp_size      = [28, 24];  amp_h = 13;   amp_pins = 6;                 // MAX98306, liegt im Griff; abgewinkelte Stiftleiste ragt 6 mm über eine 28-mm-Kante
mic_board     = [20, 14]; mic_capsule_d = 10; mic_cap_off = 7.5; mic_cap_h = 10.4;   // Platine lang x breit; Kapselmitte 7,5 mm von der Schmalkante; Kapsel 10,4 über der Platine
mic_hole_dy   = 4.8;  mic_hole_dz = 5;                                 // zwei M2-Löcher neben der Kapsel (quer ±4,8, 5 mm zur Schmalkante hin)
mic_c         = [14, -10.8];  // Mitte der Mikrofonplatine auf der Rückplatte (y, z): links unten, quer liegend, Kapselseite zur Mitte
mic_cap       = [mic_c[0] - mic_board[0] / 2 + mic_cap_off, mic_c[1]];   // Kapselmitte (y, z)
mic_box_wall  = 2;
mic_ledge     = mic_cap_h - rear_t;   // Platine liegt 6,4 mm hinter der Platteninnenfläche, dann ist die Kapsel außen bündig

// ---------------- Bedienung ----------------
button_d = 7.3; toggle_d = 6.3; voice_x = -22; usb_slot = [13, 7.5];
nut_af = 10; nut_h = 4; nut_d = 7; nut_pitch = 0.75;   // gedruckte Mutter für die Taster (M7 x 0,75, ohne Mutter geliefert): 10 mm Schlüsselweite, modelliertes Innengewinde
nut_clear = [0.15, 0.25, 0.35];   // radiales Spiel der drei Probemuttern; Kerben an einer Ecke: 1, 2, 3
toggle_x = -79; toggle_z = -8;      // Kippschalter in der linken Seitenwand, Hebel wippt vor/zurück

// ---------------- Griff: hohler Hals mit Drücker, darunter Kammer für den Verstärker ----------------
neck = 28; neck_w = 30;             // offener Durchgang in den Lauf, Drücker sitzt hier
plate_t = 6;                        // Platte zwischen Hals und Kammer, Bodenplatte; tragen die Schalenschrauben
cav_x = 30; cav_y = 32; cav_len = 40;   // Verstärkerkammer (MAX98306 28 x 24 + 6 mm Stifte, 13 hoch)
handle_t = 40; handle_w = 38; handle_tilt = 12; handle_x = -62; handle_r = 9; embed = 12;
trigger_z = 16;
shell_t = 4;                        // Griffschale (linke Seite, +Y), liegt bündig in einer Tasche; 4 mm wegen der Senkungen
shell_screw_x = 8;                  // Schrauben M3 bei x = ±8
window_w = 32;                      // Fenster in der linken Griffwand: Hals und Kammer
grip_wire_d = 4;                    // zwei Löcher durch die Platte für die Verstärkerkabel
handle_len = neck + plate_t + cav_len + plate_t;   // 80
z_plate_top = -neck;
z_cav_top   = -neck - plate_t;
z_cav_bot   = z_cav_top - cav_len;
shell_screw_z = [z_plate_top - plate_t / 2, z_cav_bot - plate_t / 2];

// ---------------- Trichter, Schrauben ----------------
cone_len = 90; cone_end_r = 75; cone_wall = 2.5; collar_len = 22; fit = 0.3;
screw_tap_d = 2.6; screw_thru_d = 3.3; screw_head_d = 6.2; screw_head_h = 2.2;   // M3: Kernloch, Durchgang, Senkung für Linsenkopf
m2_tap_d = 1.8;                                                                   // Mikrofonplatine

// =====================================================================
module rrect(x, y, r) { hull() for (sx = [-1, 1], sy = [-1, 1]) translate([sx * (x / 2 - r), sy * (y / 2 - r)]) circle(r = r); }
module handle_frame() { translate([handle_x, 0, -barrel_r + embed]) rotate([0, handle_tilt, 0]) children(); }
module xcyl(d, x0, x1) { translate([x0, 0, 0]) rotate([0, 90, 0]) cylinder(d = d, h = x1 - x0); }
module shell(r0, r1, x0, x1) { difference() { xcyl(2 * r1, x0, x1); xcyl(2 * r0, x0 - 1, x1 + 1); } }
// Keil um die Oberseite: halber Winkel deg, in X von x0 bis x1
module sector(deg, x0, x1) {
    R = 60;
    translate([x0, 0, 0]) rotate([90, 0, 90]) linear_extrude(x1 - x0)
        polygon(concat([[0, 0]], [for (i = [0 : 48]) let(a = -deg + 2 * deg * i / 48) [R * sin(a), R * cos(a)]]));
}
module barrel_outer() { xcyl(2 * barrel_r, -barrel_len, 0); }
module barrel_cavity() { xcyl(2 * (barrel_r - wall), -barrel_len - 1, -front_wall); }   // hinten offen
module cell_cyl(c, extra = 0, x0 = pack_in_x[0], x1 = pack_in_x[1]) { translate([x0, c[0], c[1]]) rotate([0, 90, 0]) cylinder(d = aa_d + 2 * extra, h = x1 - x0); }
module neck_prism() { handle_frame() translate([0, 0, -neck - 1]) linear_extrude(neck + 1.01) rrect(neck_w, neck_w, 3); }

function inner_deg(r) = asin((barrel_r * sin(hood_half_deg) - lip_w) / r);   // Öffnungswinkel im Wandinneren: außen minus Lippenbreite

// ---------- Kantenklammern: Platine von oben einrasten, Nasen an den Längskanten, Anschläge mittig an den Enden ----------
module edge_clips(L, W, z0, xs, gap = 0.3) {
    h = pcb_t + gap;
    for (x = xs, s = [-1, 1]) {
        py = s * (W / 2 + gap);
        translate([x - 1.5, py - (s > 0 ? 0 : 3), z0]) cube([3, 3, h + 1.6]);                       // Pfosten außen an der Kante
        hull() {                                                                                      // Nase: flache Unterseite auf Platinenhöhe, schräge Oberseite
            translate([x - 1.5, py - (s > 0 ? 1.2 : 0), z0 + h]) cube([3, 1.2, 0.01]);
            translate([x - 1.5, py - (s > 0 ? 0.01 : 0), z0 + h]) cube([3, 0.01, 1.6]);
        }
    }
    for (s = [-1, 1]) translate([s * (L / 2 + gap) - (s > 0 ? 0 : 3), -3, z0]) cube([3, 6, h + 1.6]);   // Anschläge an den Enden (Lötpads sitzen in den Ecken)
}

// ---------- Freiraum für den Innenteil der Haube: im Öffnungswinkel nur bis r = 31,7 ----------
module hood_clear() { intersection() { sector(inner_deg(lip_r) + 1.5, hood_x[0] - 1, hood_x[1] + 1); shell(31.7, barrel_r + 5, hood_x[0] - 1, hood_x[1] + 1); } }

// ---------- Batteriefach: zwei Stirnwände + zwei U-Rippen ----------
module pack_frame() {
    difference() {
        union() {
            translate([pack_in_x[0] - pack_wall_t, -barrel_r, rib_z0]) cube([pack_wall_t, 2 * barrel_r, barrel_r]);   // hintere Stirnwand
            translate([pack_in_x[1], -barrel_r, rib_z0]) cube([pack_wall_t, 2 * barrel_r, barrel_r]);                 // vordere Stirnwand
            for (x = rib_x) translate([x - rib_t / 2, -barrel_r, rib_z0]) cube([rib_t, 2 * barrel_r, cell_z - rib_z0]);   // U-Rippen bis zur Achse von A und C
        }
        for (c = [cells[0], cells[2]]) cell_cyl(c, cell_fit, pack_in_x[0] + 0.5, pack_in_x[1] - 0.5);                   // Zellenbetten in den Rippen
        for (x = rib_x) translate([x - rib_t / 2 - 1, -3, cell_z - 6]) cube([rib_t + 2, 6, 20]);                            // Steg der Rippen zwischen A und C niedrig halten
        hood_clear();
    }
}

// ---------- Schlitten für den ESP32: zwei Schienen unter der Platine (zwischen den Stiftleisten), vorn Anschlag mit Lippe ----------
// Die Platine wird bei abgenommener Rückplatte von hinten eingeschoben; die Stiftleisten hängen außen neben den Schienen und führen seitlich.
module esp_sled() {
    x0 = esp_x - esp_size[0] / 2; x1 = esp_x + esp_size[0] / 2;
    y0 = esp_y - esp_size[1] / 2 + esp_rail_in;  y1 = esp_y + esp_size[1] / 2 - esp_rail_in;   // Außenflächen der Schienen
    translate([x0, y0, -barrel_r]) cube([x1 - x0, 3, barrel_r + esp_z]);
    translate([x0, y1 - 3, -barrel_r]) cube([x1 - x0, 3, barrel_r + esp_z]);
    translate([x1 + 0.3, y0, esp_z - 1]) cube([2, y1 - y0, pcb_t + 1 + 3.0]);         // Anschlag vorn (Brücke zwischen den Schienen)
    translate([x1 - 2.5, y0, esp_z + pcb_t + 1.5]) cube([2.8, y1 - y0, 1.5]);         // Lippe über der Vorderkante: Antennenzone des Moduls (1 mm hoch, 4 mm ohne Stifte), 0,5 mm Luft
}

// ---------- Pfosten für die Rückplatte, bis in die Wand gezogen ----------
module rear_bosses() {
    for (p = rear_screws) hull() {
        translate([0, p[0], p[1]]) xcyl(rear_boss_d, -barrel_len + rear_t, -barrel_len + rear_t + rear_boss_len);
        translate([0, p[0] * 1.15, p[1] * 1.15]) xcyl(rear_boss_d, -barrel_len + rear_t, -barrel_len + rear_t + rear_boss_len);
    }
}

// ================= Körper =================
module body_solid() {
    union() {
        barrel_outer();
        handle_frame() translate([0, 0, -handle_len]) linear_extrude(handle_len) rrect(handle_t, handle_w, handle_r);
    }
}
module deck(x0, x1, y0, y1, z0, z1) { intersection() { translate([x0, y0, z0]) cube([x1 - x0, y1 - y0, z1 - z0]); barrel_cavity(); } }

module body_inner_features() {
    deck(-76, -32, boost_deck_y[0], boost_deck_y[1], boost_z - deck_t, boost_z);                       // Deck links für den MT3608
    deck(-76, -32, boost_deck_y[0], boost_deck_y[0] + 3, -barrel_r, boost_z);                          // Stützwand unter der linken Deckkante, begrenzt den Kabelkanal
    translate([boost_x, boost_y, 0]) edge_clips(boost_size[0], boost_size[1], boost_z, [-8, 8]);
    esp_sled();
    rear_bosses();
    intersection() { pack_frame(); xcyl(2 * (barrel_r - wall + 0.5), -barrel_len, 0); }                 // 0,5 mm in die Wand hinein
    xcyl(speaker_d + 12, -front_wall - speaker_depth - 2, -front_wall);                                   // Lautsprecherring
    for (a = cone_screw_a) rotate([a, 0, 0]) translate([cone_screw_x, 0, barrel_r - wall - 6]) cylinder(r = 4, h = 7);
    // Block hinten oben für die Haubenschraube: unter dem Innenteil der Haube (r < 31,7), verschmilzt mit der hinteren Stirnwand
    intersection() { translate([hood_x[0] + lip_w, -6, 25.5]) cube([hood_screw_x + 2.5 - hood_x[0] - lip_w, 12, 10]); xcyl(2 * 31.7, -barrel_len, 0); }
}

module body_cutouts() {
    barrel_cavity();
    // Haube: Außenteil voll durch, Innenteil (r < lip_r) schmaler
    intersection() { sector(hood_half_deg, hood_x[0], hood_x[1]); shell(lip_r, barrel_r + 1, hood_x[0], hood_x[1]); }
    intersection() { sector(inner_deg(lip_r), hood_x[0] + lip_w, hood_x[1] - lip_w); shell(barrel_r - wall - 1, lip_r + 0.01, hood_x[0] + lip_w, hood_x[1] - lip_w); }
    // Rastmulden in den Lippenflächen
    for (x = snap_x, s = [-1, 1]) { a = s * inner_deg(lip_r); translate([x, 33.6 * sin(a), 33.6 * cos(a)]) sphere(r = snap_r + 0.3); }
    // Lautsprecher
    xcyl(speaker_open, -front_wall - 1, 1);
    xcyl(speaker_d + 0.6, -front_wall - speaker_depth - 3, -front_wall);
    // Kippschalter links in der Seitenwand
    translate([toggle_x, 0, toggle_z]) rotate([-90, 0, 0]) cylinder(d = toggle_d, h = barrel_r + 1);
    // Aufkleber links
    difference() {
        translate([-55, 0, 8]) rotate([-90, 0, 0]) cylinder(d = badge_d, h = barrel_r + 5);
        xcyl(2 * (barrel_r - 1), -barrel_len - 1, 1);
    }
    // Griff
    handle_frame() {
        translate([0, 0, -neck]) linear_extrude(neck + 0.01) rrect(neck_w, neck_w, 3);                   // Hals, oben offen in den Lauf
        translate([0, 0, z_cav_bot]) linear_extrude(cav_len) rrect(cav_x, cav_y, 3);                      // Verstärkerkammer
        translate([0, 0, -trigger_z]) rotate([0, 90, 0]) cylinder(d = button_d, h = handle_t / 2 + 1);   // Abzug vorn
        translate([neck_w / 2 - 1, 0, -trigger_z]) rotate([0, 90, 0]) cylinder(d = 13, h = (handle_t - neck_w) / 2 - 3 + 1);   // Senkung innen für die Mutter, 3 mm Wand bleiben
        // Fenster in der linken Wand: Hals (Drücker) und Kammer (Verstärker); die Platte dazwischen bleibt als Riegel stehen
        translate([-window_w / 2, neck_w / 2 - 1, -neck + 2]) cube([window_w, 20, neck - 2 - 7]);
        translate([-window_w / 2, cav_y / 2 - 1, z_cav_bot + 2]) cube([window_w, 20, cav_len - 4]);
        // Tasche für die Griffschale (shell_t tief, außen bündig), oben vom Lauf begrenzt
        translate([0, 0, -handle_len - 1]) linear_extrude(handle_len + 1 - 7 + 0.3) intersection() { rrect(handle_t + 0.6, handle_w + 0.6, handle_r + 0.3); translate([-40, handle_w / 2 - shell_t]) square([80, 20]); }
        // Kabellöcher durch die Platte in den linken Ecken
        for (sy = [-1, 1]) translate([-cav_x / 2 + 3, sy * (cav_y / 2 - 4), z_cav_top - 1]) cylinder(d = grip_wire_d, h = plate_t + 2);
        // Schraubenkernlöcher für die Griffschale, in beide Platten, von links
        for (sx = [-1, 1], zc = shell_screw_z)
            translate([sx * shell_screw_x, handle_w / 2 - shell_t - 10, zc]) rotate([-90, 0, 0]) cylinder(d = screw_tap_d, h = 11);
    }
}

// Taschen, Nuten und Kabellöcher in den Stirnwänden des Batteriefachs (werden zuletzt abgezogen)
module pack_contacts() {
    xr = pack_in_x[0]; xf = pack_in_x[1];                                     // Innenflächen hinten / vorn
    module pocket(xface, dir, c) { translate([xface + (dir > 0 ? -pocket_d : 0), c[0] - pocket_w / 2, c[1] - pocket_w / 2]) cube([pocket_d + 0.01, pocket_w, pocket_w]); }
    module groove(xface, dir, c1, c2) { hull() for (c = [c1, c2]) translate([xface + (dir > 0 ? -groove_d : 0), c[0] - groove_w / 2, c[1] - groove_w / 2]) cube([groove_d + 0.01, groove_w, groove_w]); }
    // hinten (Innenfläche zeigt nach +x): Taschen A, B, C; Brücke A+ -> B-; Kabelloch am Plus-Anschluss C+
    for (c = cells) pocket(xr, 1, c);
    groove(xr, 1, cells[0], cells[1]);
    translate([xr - pack_wall_t - 1, cells[2][0], cells[2][1] - 5]) rotate([0, 90, 0]) cylinder(d = wire_hole_d, h = pack_wall_t + 2);
    // vorn (Innenfläche zeigt nach -x): Taschen A, B, C; Brücke B+ -> C-; Kabelloch am Minus-Anschluss A-
    for (c = cells) pocket(xf, -1, c);
    groove(xf, -1, cells[1], cells[2]);
    translate([xf - 1, cells[0][0], cells[0][1] - 5]) rotate([0, 90, 0]) cylinder(d = wire_hole_d, h = pack_wall_t + 2);
}

module body_holes_after() {
    for (a = cone_screw_a) rotate([a, 0, 0]) translate([cone_screw_x, 0, barrel_r - wall - 7]) cylinder(d = screw_tap_d, h = 12);
    translate([hood_screw_x, 0, 20]) cylinder(d = screw_tap_d, h = 20);                                   // Haubenschraube: Kernloch im Block / in der Stirnwand
    xcyl(speaker_d + 0.6, -front_wall - speaker_depth - 3, -front_wall);
    for (a = [90, 210, 330]) rotate([a, 0, 0]) translate([-front_wall - speaker_depth - 3, 0, speaker_clamp_pcd / 2]) rotate([0, 90, 0]) cylinder(d = screw_tap_d, h = speaker_depth + 2);
    for (p = rear_screws) translate([0, p[0], p[1]]) xcyl(screw_tap_d, -barrel_len + rear_t - 1, -barrel_len + rear_t + rear_boss_len - 1);   // Kernlöcher der Rückplatte, 7 mm tief
    pack_contacts();
}

module body() {
    difference() {
        union() {
            difference() { body_solid(); body_cutouts(); }
            intersection() { difference() { body_inner_features(); neck_prism(); } body_solid(); }   // Schienen und Stützwand enden über dem Hals
        }
        body_holes_after();
    }
}

// ================= Haube =================
module hood() {
    g = hood_gap;
    difference() {
        union() {
            // Außenteil: liegt auf der Lippe, bündig mit dem Lauf
            intersection() { sector(hood_half_deg - 0.5, hood_x[0] + g, hood_x[1] - g); shell(lip_r + 0.15, barrel_r, hood_x[0] + g, hood_x[1] - g); }
            // Innenteil: sitzt innerhalb der Lippe, rundum 0,3 mm Luft
            intersection() { sector(inner_deg(lip_r) - 0.7, hood_x[0] + lip_w + g, hood_x[1] - lip_w - g); shell(32, lip_r + 0.01, hood_x[0] + lip_w + g, hood_x[1] - lip_w - g); }
            // Rastnasen an den Seitenflächen des Innenteils
            for (x = snap_x, s = [-1, 1]) { a = s * (inner_deg(lip_r) - 0.7); translate([x, 33.6 * sin(a), 33.6 * cos(a)]) sphere(r = snap_r); }
        }
        // gelber Taster vorn, Sicherungsschraube hinten
        translate([voice_x, 0, 20]) cylinder(d = button_d, h = 20);
        translate([hood_screw_x, 0, 20]) cylinder(d = screw_thru_d, h = 20);   // Linsenkopfschraube M3 x 12
    }
}

// ================= Rückplatte =================
// Scheibe im offenen Laufende; innen ein Kasten für die Mikrofonplatine (Kapsel durch die Platte, außen bündig) und zwei Nasen,
// die die hinteren Ecken des eingeschobenen ESP32 niederhalten. Außen flach: Senkungen für die vier M3, USB-C-Schlitz, Kapselloch.
module rear_plate() {
    xi = -barrel_len + rear_t;                                                      // Innenfläche
    bw = mic_board[0] + 0.6 + 2 * mic_box_wall;  bh = mic_board[1] + 0.6 + 2 * mic_box_wall;
    box_len = mic_ledge + pcb_t + 1.2;                                              // Auflage + Platine + 1 mm Rand
    difference() {
        union() {
            xcyl(2 * (barrel_r - wall - rear_fit), -barrel_len, xi);
            intersection() {                                                        // Mikrofonkasten, Ecken von der Laufwand beschnitten
                translate([xi - 0.01, mic_c[0] - bw / 2, mic_c[1] - bh / 2]) cube([box_len + 0.01, bw, bh]);
                xcyl(2 * (barrel_r - wall - 0.5), -barrel_len, 0);
            }
            for (s = [-1, 1]) { ye = esp_y + s * esp_size[1] / 2;                   // Haltenasen über den hinteren Ecken des ESP32 (2 mm Überdeckung)
                translate([xi - 0.01, s > 0 ? ye - 2.5 : ye - 1, esp_z + pcb_t + 0.3]) cube([4.01, 3.5, 2]); }
        }
        translate([0, mic_cap[0], mic_cap[1]]) xcyl(mic_capsule_d + 0.6, -barrel_len - 1, xi + mic_ledge + 0.01);                 // Kapsel
        translate([xi + mic_ledge, mic_c[0] - (mic_board[0] + 0.6) / 2, mic_c[1] - (mic_board[1] + 0.6) / 2]) cube([box_len, mic_board[0] + 0.6, mic_board[1] + 0.6]);   // Platinentasche, nach innen offen
        for (s = [-1, 1]) translate([0, mic_cap[0] - mic_hole_dz, mic_cap[1] + s * mic_hole_dy]) xcyl(m2_tap_d, xi + mic_ledge - 3, xi + mic_ledge + 1);   // M2 von innen
        translate([-barrel_len - 1, esp_y - usb_slot[0] / 2, esp_z + pcb_t + esp_top_h / 2 - usb_slot[1] / 2]) cube([rear_t + 2, usb_slot[0], usb_slot[1]]);   // USB-C, mittig zum Stecker
        for (p = rear_screws) translate([0, p[0], p[1]]) { xcyl(screw_thru_d, -barrel_len - 1, xi + 1); xcyl(screw_head_d, -barrel_len - 1, -barrel_len + screw_head_h); }
    }
}

// ================= Trichter =================
function horn_r(t, r0, r1) = r0 + (r1 - r0) * pow(t, 1.7);
module cone_part() {
    n = 32; r_in = barrel_r + fit; r_out = r_in + cone_wall;
    outer = concat([[r_out, 0], [r_out, collar_len]], [for (i = [0 : n]) [horn_r(i / n, r_out, cone_end_r), collar_len + cone_len * i / n]]);
    inner = concat([[horn_r(1, r_out, cone_end_r) - cone_wall, collar_len + cone_len]],
                   [for (i = [n : -1 : 0]) [horn_r(i / n, r_out, cone_end_r) - cone_wall, collar_len + cone_len * i / n]],
                   [[r_in - 2.5, collar_len + 2], [r_in - 2.5, collar_len], [r_in, collar_len], [r_in, 0]]);
    difference() {
        rotate([0, 90, 0]) rotate_extrude() polygon(concat(outer, inner));
        for (a = cone_screw_a) rotate([a, 0, 0]) translate([collar_len + cone_screw_x, 0, r_in - 1]) cylinder(d = screw_thru_d, h = cone_wall + 3);
    }
}

// ================= Griffschale =================
// Rechte Seitenwand des Griffs als eigenes Teil: Außenkontur des Griffs, shell_t dick, oben vom Lauf begrenzt
module grip_shell() {
    difference() {
        handle_frame() translate([0, 0, -handle_len]) linear_extrude(handle_len - 7)
            intersection() { rrect(handle_t, handle_w, handle_r); translate([-40, handle_w / 2 - shell_t]) square([80, 20]); }
        xcyl(2 * (barrel_r + 0.8), -barrel_len - 1, 1);                                              // Luft zum Lauf
        handle_frame() for (sx = [-1, 1], zc = shell_screw_z) {
            translate([sx * shell_screw_x, handle_w / 2 - shell_t - 1, zc]) rotate([-90, 0, 0]) cylinder(d = screw_thru_d, h = shell_t + 2);
            translate([sx * shell_screw_x, handle_w / 2 - screw_head_h, zc]) rotate([-90, 0, 0]) cylinder(d = screw_head_d, h = screw_head_h + 1);   // Senkung außen
        }
    }
}
// Schale flach aufs Bett legen: Innenseite unten
module grip_shell_flat() {
    rotate([90, 0, 0]) translate([0, -(handle_w / 2 - shell_t), 0]) rotate([0, -handle_tilt, 0]) translate([-handle_x, 0, barrel_r - embed]) grip_shell();
}

// Vorschau: Zellen und Elektronik als Hüllkörper (grün ESP32 mit Stiften, magenta MT3608, rot MAX98306 im Griff, orange Kippschalter, blau Mikrofon)
module cells_preview() { %for (c = cells) cell_cyl(c, 0, pack_in_x[0] + 1.5, pack_in_x[0] + 1.5 + aa_len); }
module electronics_preview(esp_dx = 0, show_mic = true) {   // esp_dx: ESP32 um so viel nach hinten gezogen (Einschub zeigen)
    x0 = esp_x - esp_size[0] / 2; x1 = esp_x + esp_size[0] / 2; y0 = esp_y - esp_size[1] / 2;
    color("LimeGreen") translate([-esp_dx, 0, 0]) {
        translate([x0, y0, esp_z]) cube([esp_size[0], esp_size[1], pcb_t]);
        translate([x0 - 1.5, y0 + 3, esp_z + pcb_t]) cube([esp_size[0] + 1.5 - 6.6, esp_size[1] - 6, esp_top_h]);   // Modul, USB-C (ragt 1,5 nach hinten), Taster
        translate([x1 - 6.6, y0 + 5.5, esp_z + pcb_t]) cube([6.6, esp_size[1] - 11, 1]);                              // Antennenteil des Moduls, 1 mm hoch
        for (s = [0, 1]) translate([x1 - esp_pin_x[1], y0 + (s > 0 ? esp_size[1] - esp_pins[1] : esp_pins[0]), esp_z - esp_pin_len])
            cube([esp_pin_x[1] - esp_pin_x[0], esp_pins[1] - esp_pins[0], esp_pin_len]);
    }
    color("Magenta") translate([boost_x - boost_size[0] / 2, boost_y - boost_size[1] / 2, boost_z]) cube([boost_size[0], boost_size[1], pcb_t + boost_h]);
    color("Crimson") handle_frame() translate([-amp_size[0] / 2, -cav_y / 2 + 1, z_cav_bot + 0.5]) cube([amp_size[0], amp_size[1] + amp_pins, amp_h]);
    color("DarkOrange") translate([toggle_x - 6.5, 22, toggle_z - 4]) cube([13, 10, 8]);                   // Kippschalter-Körper innen
    if (show_mic) color("DodgerBlue") {
        translate([-barrel_len + rear_t + mic_ledge, mic_c[0] - mic_board[0] / 2, mic_c[1] - mic_board[1] / 2]) cube([pcb_t, mic_board[0], mic_board[1]]);
        translate([0, mic_cap[0], mic_cap[1]]) xcyl(mic_capsule_d, -barrel_len, -barrel_len + rear_t + mic_ledge);
    }
}

// ================= Tastermutter =================
// Sechskant 10 mm Schlüsselweite, 4 mm hoch, Innengewinde M7 x 0,75 (ISO-Profil, rechtsgängig) mit radialem Spiel c.
// Drei Probemuttern mit 0,15 / 0,25 / 0,35 mm Spiel, markiert mit 1 / 2 / 3 Kerben. Flach drucken, 0,1-mm-Schichten.
// Gewindekörper als verformter Zylinder: Radius hängt von der Phase (z - Steigung * Winkel / 360) ab, damit immer geschlossen.
function thread_r(u, c) =          // u = Phase 0..1 innerhalb einer Steigung
    let(rmin = (nut_d - 1.0825 * nut_pitch) / 2 + c, rmax = nut_d / 2 + c + 0.1, f = u - floor(u))
    f < 0.125 ? rmax :
    f < 0.4375 ? rmax - (rmax - rmin) * (f - 0.125) / 0.3125 :
    f < 0.6875 ? rmin :
    rmin + (rmax - rmin) * (f - 0.6875) / 0.3125;
module thread_cutter(c, h) {
    n = 72; dz = nut_pitch / 12; m = ceil((h + 2) / dz);
    pts = [for (j = [0 : m]) for (i = [0 : n - 1])
              let(a = 360 * i / n, z = -1 + j * dz, r = thread_r((z - nut_pitch * i / n) / nut_pitch, c)) [r * cos(a), r * sin(a), z]];
    side = [for (j = [0 : m - 1]) for (i = [0 : n - 1]) let(i2 = (i + 1) % n) [j * n + i, (j + 1) * n + i, (j + 1) * n + i2, j * n + i2]];
    caps = [[for (i = [0 : n - 1]) i], [for (i = [n - 1 : -1 : 0]) m * n + i]];   // OpenSCAD: von außen gesehen im Uhrzeigersinn
    polyhedron(points = pts, faces = concat(side, caps));
}
module button_nut(c = 0.25, marks = 2) {
    difference() {
        cylinder(d = nut_af / cos(30), h = nut_h, $fn = 6);
        thread_cutter(c, nut_h);
        for (z = [-0.01, nut_h - 0.6]) translate([0, 0, z]) cylinder(d1 = z < 0 ? nut_d + 1.2 : nut_d, d2 = z < 0 ? nut_d : nut_d + 1.2, h = 0.61);   // Fasen beidseitig
        for (k = [0 : marks - 1]) rotate([0, 0, (k - (marks - 1) / 2) * 12]) translate([nut_af / cos(30) / 2 - 0.4, 0, nut_h - 1.2]) cylinder(d = 1, h = 2, $fn = 12);   // Kerben
    }
}

// Zellen für die Anleitung: massiv, mit Pluspol-Nase und Buchstaben (A und C: Plus nach hinten, B: Plus nach vorn)
module cells_solid() {
    x0 = pack_in_x[0] + 1.5;
    for (i = [0 : 2]) {
        c = cells[i]; plus_rear = (i != 1);
        color("Khaki") cell_cyl(c, 0, x0 + 1.5, x0 + aa_len);
        color("DimGray") translate([plus_rear ? x0 : x0 + aa_len, c[0], c[1]]) rotate([0, 90, 0]) cylinder(d = 5.5, h = 1.5);
        color("Black") translate([x0 + aa_len / 2, c[0], c[1] + aa_d / 2 - 0.2]) rotate([0, 0, -90]) linear_extrude(0.8) text(["A", "B", "C"][i], size = 7, halign = "center", valign = "center");
        color("Black") translate([plus_rear ? x0 + 8 : x0 + aa_len - 8, c[0], c[1] + aa_d / 2 - 0.2]) rotate([0, 0, -90]) linear_extrude(0.8) text("+", size = 6, halign = "center", valign = "center");
    }
}
module below(z) { translate([-200, -100, z - 200]) cube([400, 200, 200]); }

// ================= Ausgabe =================
if (part == "koerper") {
    rotate([0, 90, 0]) body();                                                // Vorderwand auf dem Bett, Laufende oben offen
} else if (part == "haube") {
    translate([0, 0, -hood_x[0] - hood_gap]) rotate([0, -90, 0]) hood();     // steht auf der hinteren Stirnfläche
} else if (part == "rueckplatte") {
    translate([0, 0, barrel_len]) rotate([0, -90, 0]) rear_plate();          // Außenseite auf dem Bett, Kasten und Nasen oben
} else if (part == "tastermutter") {
    for (i = [0 : 2]) translate([i * 16, 0, 0]) button_nut(nut_clear[i], i + 1);   // drei Probemuttern, 1/2/3 Kerben
} else if (part == "tastermutter1" || part == "tastermutter2" || part == "tastermutter3") {
    k = ord(part[12]) - ord("1"); button_nut(nut_clear[k], k + 1);          // einzelne Probemutter
} else if (part == "kegel") {
    translate([0, 0, collar_len + cone_len]) rotate([0, 90, 0]) cone_part();
} else if (part == "griffschale") {
    grip_shell_flat();
} else if (part == "offen") {
    color("DeepSkyBlue") body();
    color("Orange") translate([0, 0, 45]) hood();
    color("MediumPurple") translate([-30, 0, 0]) rear_plate();
    color("Tomato") translate([0, 30, 0]) grip_shell();
    translate([0, 0, 22]) cells_preview();
} else if (part == "innen") {                                              // Blick von oben ohne Haube
    color("DeepSkyBlue") body();
    color("MediumPurple") rear_plate();
    cells_preview();
    electronics_preview();
} else if (part == "zellen") {                                             // Haube ab: Lage und Polung der Zellen
    color("DeepSkyBlue") body();
    cells_solid();
} else if (part == "schnitt") {                                            // Lauf oberhalb z = 3 weggeschnitten: wo liegt was
    color("DeepSkyBlue") intersection() { body(); below(3); }
    color("MediumPurple") intersection() { rear_plate(); below(3); }
    electronics_preview();
} else if (part == "griff") {                                              // Griffschale ab: Abzug und Verstärkerkammer
    color("DeepSkyBlue") body();
    electronics_preview();
} else if (part == "einschub") {                                           // Rückplatte ab, ESP32 halb eingeschoben
    color("DeepSkyBlue") body();
    electronics_preview(45, false);
} else if (part == "none") {
} else {
    color("DeepSkyBlue") body();
    color("Orange") hood();
    color("MediumPurple") rear_plate();
    color("Gold") translate([-collar_len, 0, 0]) cone_part();
    color("Tomato") grip_shell();
    cells_preview();
}
