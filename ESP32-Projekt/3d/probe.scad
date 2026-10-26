// Prüfskript: Schnittvolumen zwischen Körper, Haube, Rückplatte, Zellen und Elektronik-Hüllkörpern.
// Aufruf über check.sh; jede Probe muss leer sein (OpenSCAD schreibt dann keine STL) oder 0 mm³ haben.
include <megafon.scad>
part = "none";
probe = "hood";

// ESP32 DevKitC als Hüllkörper: Platine, Modul/USB-C/Taster oben, Stiftleisten unten; dx verlängert alles nach hinten (Einschubweg)
module esp_env_parts(dx = 0) {
    x0 = esp_x - esp_size[0] / 2; x1 = esp_x + esp_size[0] / 2; y0 = esp_y - esp_size[1] / 2;
    translate([x0 - dx, y0, esp_z]) cube([esp_size[0] + dx, esp_size[1], pcb_t]);
    translate([x0 - 1.5 - dx, y0 + 3, esp_z + pcb_t]) cube([esp_size[0] + 1.5 + dx - 6.6, esp_size[1] - 6, esp_top_h]);
    translate([x1 - 6.6 - dx, y0 + 5.5, esp_z + pcb_t]) cube([6.6 + dx, esp_size[1] - 11, 1]);                       // Antennenteil des Moduls
    for (s = [0, 1]) translate([x0 + 1 - dx, y0 + (s > 0 ? esp_size[1] - 9.3 : 3.3), esp_z + pcb_t]) cube([6 + dx, 6, 3.5]);   // EN/BOOT-Taster
    for (s = [0, 1]) translate([x1 - esp_pin_x[1] - dx, y0 + (s > 0 ? esp_size[1] - esp_pins[1] : esp_pins[0]), esp_z - esp_pin_len])
        cube([esp_pin_x[1] - esp_pin_x[0] + dx, esp_pins[1] - esp_pins[0], esp_pin_len]);
}
module esp_env() { esp_env_parts(0); }
module boost_env() { translate([boost_x - boost_size[0] / 2, boost_y - boost_size[1] / 2, boost_z]) cube([boost_size[0], boost_size[1], pcb_t + boost_h]); }
module amp_env() { handle_frame() translate([-amp_size[0] / 2, -cav_y / 2 + 1, z_cav_bot + 0.5]) cube([amp_size[0], amp_size[1] + amp_pins, amp_h]); }
module toggle_env() { translate([toggle_x - 6.5, 20.5, toggle_z - 4]) cube([13, 10, 8]); }   // Körper 10 mm tief ab Wandinnenseite
module mic_env() {   // Platine + Kapsel + 2,5 mm Bauteile auf der Rückseite
    translate([-barrel_len + rear_t + mic_ledge, mic_c[0] - mic_board[0] / 2, mic_c[1] - mic_board[1] / 2]) cube([pcb_t + 2.5, mic_board[0], mic_board[1]]);
    translate([0, mic_cap[0], mic_cap[1]]) xcyl(mic_capsule_d, -barrel_len, -barrel_len + rear_t + mic_ledge);
}
module electronics() { esp_env(); boost_env(); amp_env(); toggle_env(); mic_env(); }
module cells_real() { for (c = cells) cell_cyl(c, 0, pack_in_x[0] + 1.5, pack_in_x[0] + 1.5 + aa_len); }
module lift() { for (c = cells) hull() { cell_cyl(c, 0, pack_in_x[0] + 1.5, pack_in_x[0] + 1.5 + aa_len); translate([0, 0, 40]) cell_cyl(c, 0, pack_in_x[0] + 1.5, pack_in_x[0] + 1.5 + aa_len); } }
module slide() { esp_env_parts(60); }
module boost_drop() { hull() { boost_env(); translate([0, 0, 18]) boost_env(); } }   // bis z 0, danach seitlich zur Haubenöffnung

if (probe == "hood") intersection() { body(); hood(); }
else if (probe == "shell") intersection() { body(); grip_shell(); }
else if (probe == "cells_body") intersection() { body(); cells_real(); }
else if (probe == "cells_hood") intersection() { hood(); cells_real(); }
else if (probe == "esp_body") intersection() { body(); esp_env(); }
else if (probe == "boost_body") intersection() { body(); boost_env(); }      // nur die vier Nasen der Klammern dürfen auftauchen (ca. 7 mm³)
else if (probe == "amp_body") intersection() { body(); amp_env(); }
else if (probe == "toggle_body") intersection() { body(); toggle_env(); }
else if (probe == "mic_body") intersection() { body(); mic_env(); }
else if (probe == "elec_cells") intersection() { cells_real(); electronics(); }
else if (probe == "elec_hood") intersection() { hood(); electronics(); }
else if (probe == "elec_plate") intersection() { rear_plate(); union() { esp_env(); boost_env(); toggle_env(); } }
else if (probe == "plate_body") intersection() { body(); rear_plate(); }
else if (probe == "plate_hood") intersection() { hood(); rear_plate(); }
else if (probe == "lift") intersection() { body(); lift(); }
else if (probe == "slide") intersection() { body(); slide(); }
else if (probe == "boost_drop") intersection() { body(); boost_drop(); }      // wie boost_body: nur die Nasen
