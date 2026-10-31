#ifndef IXFILM_LANG_H
#define IXFILM_LANG_H

typedef struct {
    const char *none, *color, *bw, *quality, *beep;
    const char *full_quality, *full_fast, *compact, *off, *on;
    const char *boot_hint, *film_sim, *done, *error, *play_hint;
} ixfilm_texts;

/* Canon propset 2/3/4 language values. ASCII spellings are intentional:
 * IXFilm does not load a CHDK codepage, so these render on every camera. */
static const ixfilm_texts ixfilm_languages[] = {
 /* 0 English */
 {"No film simulation","Color","Black & white","Conversion quality","Completion beep",
  "Full quality 4000x3000","Full fast 4000x3000","Compact 1600x1200","Off","On",
  "Hold DISP for film simulation menu","Film simulation","done","error","PLAY: playback + retract lens"},
 /* 1 German */
 {"Keine Filmsimulation","Farbe","Schwarzweiss","Konvertierungsqualitaet","Signal nach Konvertierung",
  "Volle Qualitaet 4000x3000","Voll schnell 4000x3000","Kompakt 1600x1200","Aus","Ein",
  "DISP halten: Filmsimulationsmenu","Filmsimulation","fertig","Fehler","PLAY: Wiedergabe + Objektiv ein"},
 /* 2 French */
 {"Sans simulation film","Couleur","Noir et blanc","Qualite de conversion","Bip apres conversion",
  "Qualite max 4000x3000","Pleine rapide 4000x3000","Compact 1600x1200","Desactive","Active",
  "Maintenez DISP: menu simulation film","Simulation film","termine","erreur","PLAY: lecture + objectif rentre"},
 /* 3 Dutch */
 {"Geen filmsimulatie","Kleur","Zwart-wit","Conversiekwaliteit","Piep na conversie",
  "Volledige kwaliteit 4000x3000","Volledig snel 4000x3000","Compact 1600x1200","Uit","Aan",
  "Houd DISP ingedrukt voor filmsimulaties","Filmsimulatie","gereed","fout","PLAY: afspeelstand + lens intrekken"},
 /* 4 Finnish */
 {"Ei filmisimulaatiota","Vari","Mustavalkoinen","Muunnoslaatu","Aanimerkki muunnoksen jalkeen",
  "Taysi laatu 4000x3000","Taysi nopea 4000x3000","Kompakti 1600x1200","Pois","Paalla",
  "Pida DISP: filmisimulaatiovalikko","Filmisimulaatio","valmis","virhe","PLAY: toisto + objektiivi sisaan"},
 /* 5 Italian */
 {"Nessuna simulazione","Colore","Bianco e nero","Qualita conversione","Bip dopo conversione",
  "Qualita completa 4000x3000","Completa veloce 4000x3000","Compatta 1600x1200","Disattivato","Attivato",
  "Tieni DISP: menu simulazione film","Simulazione film","completato","errore","PLAY: riproduzione + ritira obiettivo"},
 /* 6 Ukrainian (transliterated) */
 {"Bez imitatsii plivky","Kolir","Chorno-bile","Yakist konvertatsii","Syhnal pislia konvertatsii",
  "Povna yakist 4000x3000","Povnyi shvydkyi 4000x3000","Kompaktnyi 1600x1200","Vymk","Uvmk",
  "Utrymuite DISP: menu plivky","Imitatsiia plivky","hotovo","pomylka","PLAY: perehliad + skladannia obiektyva"},
 /* 7 Spanish */
 {"Sin simulacion de pelicula","Color","Blanco y negro","Calidad de conversion","Pitido tras conversion",
  "Calidad completa 4000x3000","Completa rapida 4000x3000","Compacta 1600x1200","Desactivado","Activado",
  "Mantenga DISP: menu simulacion pelicula","Simulacion pelicula","listo","error","PLAY: reproduccion + retraer objetivo"},
 /* 8 Russian (transliterated) */
 {"Bez imitacii plenki","Cvet","Cherno-beloe","Kachestvo konversii","Signal posle konversii",
  "Polnoe kachestvo 4000x3000","Polnoe bystro 4000x3000","Kompaktno 1600x1200","Vykl","Vkl",
  "Uderzhivayte DISP: menu plenki","Imitaciya plenki","gotovo","oshibka","PLAY: prosmotr + ubrat obektiv"},
 /* 9 Portuguese */
 {"Sem simulacao de filme","Cor","Preto e branco","Qualidade da conversao","Bip apos conversao",
  "Qualidade total 4000x3000","Total rapida 4000x3000","Compacta 1600x1200","Desligado","Ligado",
  "Segure DISP: menu simulacao de filme","Simulacao de filme","concluido","erro","PLAY: reproducao + recolher objetiva"},
 /* 10 Greek (transliterated) */
 {"Choris prosomoiosi film","Chroma","Mavroasprom","Poiotita metatropis","Bip meta ti metatropi",
  "Pliris poiotita 4000x3000","Pliris grigori 4000x3000","Sympaktiki 1600x1200","Kleisto","Anoikto",
  "Kratiste DISP: menu film","Prosomoiosi film","etoimo","sfalma","PLAY: anaparagogi + kleisimo fakou"},
 /* 11 Polish */
 {"Bez symulacji filmu","Kolor","Czarno-biale","Jakosc konwersji","Dzwiek po konwersji",
  "Pelna jakosc 4000x3000","Pelna szybka 4000x3000","Kompaktowa 1600x1200","Wyl","Wl",
  "Przytrzymaj DISP: menu filmu","Symulacja filmu","gotowe","blad","PLAY: odtwarzanie + schowanie obiektywu"},
 /* 12 Czech */
 {"Bez simulace filmu","Barva","Cernobila","Kvalita prevodu","Pipnuti po prevodu",
  "Plna kvalita 4000x3000","Plna rychla 4000x3000","Kompaktni 1600x1200","Vyp","Zap",
  "Podrzte DISP: nabidka simulace filmu","Simulace filmu","hotovo","chyba","PLAY: prehravani + zasunout objektiv"},
 /* 13 Hungarian */
 {"Nincs filmszimulacio","Szines","Fekete-feher","Konverzios minoseg","Hang a konverzio utan",
  "Teljes minoseg 4000x3000","Teljes gyors 4000x3000","Kompakt 1600x1200","Ki","Be",
  "Tartsa a DISP-et: filmszimulacio menu","Filmszimulacio","kesz","hiba","PLAY: lejatszas + objektiv behuzasa"},
 /* 14 Turkish */
 {"Film simulasyonu yok","Renkli","Siyah beyaz","Donusum kalitesi","Donusum sonrasi bip",
  "Tam kalite 4000x3000","Tam hizli 4000x3000","Kompakt 1600x1200","Kapali","Acik",
  "DISP basili tut: film simulasyonu menu","Film simulasyonu","tamam","hata","PLAY: oynatma + lensi geri cek"},
 /* 15 Romanian */
 {"Fara simulare film","Culoare","Alb-negru","Calitate conversie","Bip dupa conversie",
  "Calitate completa 4000x3000","Completa rapida 4000x3000","Compacta 1600x1200","Oprit","Pornit",
  "Tineti DISP: meniu simulare film","Simulare film","gata","eroare","PLAY: redare + retragere obiectiv"}
};

static int ixfilm_language_index(int value)
{
    switch (value & 0x7f) {
    case 1: return 1; case 2: return 2; case 3: return 3;
    case 5: return 4; case 6: return 5; case 8: return 6;
    case 10:return 7; case 12:return 8; case 13:return 9;
    case 14:return 10;case 15:return 11;case 16:return 12;
    case 17:return 13;case 18:return 14;case 23:return 15;
    default:return 0;
    }
}

#endif
