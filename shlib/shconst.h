#ifndef _SHCONST_H_
#define _SHCONST_H_


//#define PERIF

#define PORTSERVPERI 1791

#ifdef PERIF
#define PINLED 0                    //  0 = ESP-12  ; 2 = ESP-01
#define LEDON LOW
#define LEDOFF HIGH
#endif defPERIF
#ifndef PERIF
#define PINLED 13
#define LEDON HIGH
#define LEDOFF LOW
#endif ndef PERIF

    // messages

#define LENMESS     240             // longueur buffer message
#define MPOSFONC    0               // position fonction
#define MPOSLEN     11              // position longueur (longueur fonction excluse)
#define MPOSNUMPER  MPOSLEN+5       // position num périphérique
#define MPOSMAC     MPOSNUMPER+3    // position adr Mac
#define MPOSDH      MPOSMAC+18      // Date/Heure
#define MPOSPERREFR MPOSDH+15       // période refr
#define MPOSPITCH   MPOSPERREFR+6   // pitch
#define MPOSINTCDE  MPOSPITCH+5     // 4 commandes sw 0/1 (periSwVal)
#define MPOSINTPAR0 MPOSINTCDE+5    // paramétres sw (4*HH+1sep)*4
#define MPOSPULSON0 MPOSINTPAR0+MAXSW*9 // Timers On (4bytes)*4
#define MPOSPULSOF0 MPOSPULSON0+MAXSW*9 // Timers Off (4bytes+1sep)*4
#define MPOSPULSMOD MPOSPULSOF0+MAXSW*9 // paramètres timers (2*HH+1sep)*4
#define MPOSMDIAG   MPOSPULSMOD+MAXSW*5 // texte diag
#define MLMSET      MPOSMDIAG+5     // longueur message fonction incluse

    // fonctions

#define LENNOM 10   // nbre car nom de fonction
#define NBVAL 64    // nbre maxi fonct/valeurs pour getnv() --- les noms de fonctions font 10 caractères + 2 séparateurs + la donnée associée = LENVAL
                    // taille macxi pour GET (IE)=2048  ; pour POST plusieurs mega
                    // avec un arduino Atmel la limitation vient de la taille de la ram
#define LENVAL 64   // nbre car maxi valeur (vérifier datasave)
#define MEANVAL 6   // pour donner un param de taille à valeurs[]
#define LENVALEURS NBVAL*MEANVAL+1   // la taille effective de valeurs[]
#define LENPSW 16   // nbre car maxi pswd
#define RECHEAD 28                           // en-tete strSD date/heure/xx/yy + <br> + crlf
#define RECCHAR NBVAL*(MEANVAL+3)+RECHEAD+8  // longueur maxi record histo

#define PERIPREF 900                         // periode refr perif par défaut

#define LBUFSERVER LENMESS+16                // longueur bufserver (messages in/out periphériques)

#define SRVPASS "17515A\0"
#define LENSRVPASS 6

#define TOINCHCLI 4000        // msec max attente car server
#define TO_HTTPCX 4000        // nbre maxi retry connexion serveur

#define SLOWBLINK 2000
#define FASTBLINK 500
#define PULSEBLINK 40
/* code blink  (valeurs impaires bloquantes) */
/* code courant+100 reset (sauf impairs ofcourse) */
#define BCODEONBLINK      98  // allume jusqu'au prochain blink
#define BCODEPERIRECLEN   3   // PERIRECLEN trop petit -> blocage
#define BCODEPBNTP        2   // pas de service NTP
#define BCODEWAITWIFI     4   // attente WIFI
#define BCODESDCARDKO     5   // pas de SDCARD
#define BCODELENVAL       7   // LENVAL trop petit


enum {FAUX,VRAI};
enum {OFF,ON};


#define MAXSW   4       // nbre maxi switchs par périphérique
#define MAXDET  4       // nbre maxi détecteurs par périphérique
#define MAXSDE  4       // nbre maxi sondes par périphérique
#define MAXTAC  4       // 4 types actions sur pulses (start/stop/mask/force)

// messages diag

#define MESSTO    0
#define MESSDEC  -1
#define MESSOK    1
#define MESSCRC  -2
#define MESSLEN  -3
#define MESSFON  -4
#define MESSFULL -5
#define MESSSYS  -6
#define MESSCX   -7
#define MESSNUMP -8
#define MESSMAC  -9

#define NBMESS   8  // OK ne compte pas

#define TEXTTO      "*TO_\0"
#define TEXTDEC     "*OVF\0"
#define TEXTCRC     "*CRC\0"
#define TEXTLEN     "*LEN\0"
#define TEXTFON     "*FON\0"
#define TEXTFULL    "*FUL\0"
#define TEXTSYS     "*SYS\0"
#define TEXTCX      "*CX_\0"

#define TEXTMESS "\0   \0*TO_\0*OVF\0*CRC\0*LEN\0*FON\0*FUL\0*SYS\0*CX_\0"

#define LPERIMESS  5   // len texte diag en réception de message

/* bits structures definitions */

// bit coding

// mask-values

#define VBIT0 0x0001
#define VBIT1 0x0002
#define VBIT2 0x0004
#define VBIT3 0x0008
#define VBIT4 0x0010
#define VBIT5 0x0020
#define VBIT6 0x0040
#define VBIT7 0x0080
#define VBIT8 0x0100
#define VBIT9 0x0200
#define VBITA 0x0400
#define VBITB 0x0800
#define VBITC 0x1000
#define VBITD 0x2000
#define VBITE 0x4000
#define VBITF 0x8000
#define VBITG 0x00010000
#define VBITH 0x00020000
#define VBITI 0x00040000
#define VBITJ 0x00080000
#define VBITK 0x00100000
#define VBITL 0x00200000
#define VBITM 0x00400000
#define VBITN 0x00800000
#define VBITO 0x01000000
#define VBITP 0x02000000
#define VBITQ 0x04000000
#define VBITR 0x08000000
#define VBITS 0x10000000
#define VBITT 0x20000000
#define VBITU 0x40000000
#define VBITV 0x80000000

// positions

#define PBIT0 0x0000
#define PBIT1 0x0001
#define PBIT2 0x0002
#define PBIT3 0x0003
#define PBIT4 0x0004
#define PBIT5 0x0005
#define PBIT6 0x0006
#define PBIT7 0x0007
#define PBIT8 0x0008
#define PBIT9 0x0009
#define PBITA 0x000A
#define PBITB 0x000B
#define PBITC 0x000C
#define PBITD 0x000D
#define PBITE 0x000E
#define PBITF 0x000F

/* SWITCHS CONTROL */

// PULSE MODE = PM
// time one = TO ; time two = TT ; current = C ; duration = D
// Server = SR ; free run = FR ; Phase H/L-LH = PH
// enable = E ; UP/DOWN = U ; H/L = H
// number = N ; detector on = DI ; off = DO ; ON = I ; OFF = O
// most significant = MS ; least = LS
//

/* description periSwPulseCtl (3+4*10 bits =48) */

#define PMLEN     MAXSW*PMSWLEN  // nbre de bytes total periSwPulseCtl (4*6)

/* bits compteurs */

#define PMTOE_PB (6*8)-1     // time one enable numéro du bit
#define PMTOE_VB 0x800000000000 //              valeur du bit
#define PMTTE_PB (6*8)-2     // time two enable
#define PMTTE_VB 0x400000000000
#define PMFRO_PB (6*8)-3     // freeRun/OneShoot
#define PMFRO_VB 0x200000000000

/* bits détecteurs */

#define PMSWLEN   (PMNBDET*PMDLEN/8+1) // nbre bytes par sw
#define MPNBSWCB   PMNBDET*PMNBDCB     // nbre checkbox par switch

#define PMNBDET    4         // nbre détecteurs pulse

#define PMDLEN     10        // longueur (bits) desciption détecteur

#define PMNBDCB    4         // nbre checkbox/détecteur
#define PMDLNU     3         // nbre bits numéro détecteur
#define PMDLAC     3         // nbre bits code action

#define PMDNMS_PB  PMDNLS_PB+PMDLNU-1   // msb numéro (3 bits)
#define PMDNMS_VB  0x200
#define PMDNLS_PB  PMDEN_PB+1       // lsb numéro
#define PMDNLS_VB  0x080
#define PMDEN_PB   PMDLE_PB+1       // enable (1 bit)
#define PMDEN_VB   0x040
#define PMDLE_PB   PMDMO_PB+1       // local/externe (1 bit)
#define PMDLE_VB   0x020
#define PMDMO_PB   PMDHL_PB+1       // mode flanc/état (1 bit)
#define PMDMO_VB   0x010
#define PMDHL_PB   PMDAMS_PB+1      // H/L (1 bit)
#define PMDHL_VB   0x008
#define PMDAMS_PB  PMDALS_PB+PMDLAC-1 // msb action (3 bits)
#define PMDAMS_VB  0x004
#define PMDALS_PB  0         // lsb action
#define PMDALS_VB  0x001

/* codes actions */

#define PMDCA_RESET 0       // reset
#define PMDCA_RAZ   1       // raz
#define PMDCA_STOP  2       // stop  clk
#define PMDCA_START 3       // start clk
#define PMDCA_SHORT 4       // short pulse
#define PMDCA_END   5       // end pulse

/* codes mode */

#define PMDM_STAT   0       // statique
#define PMDM_TRANS  1       // transitionnel

/* codes etats générateur (bit droite=valeur) */

#define PM_DISABLE  0x10
#define PM_IDLE0    0x20
#define PM_IDLE1    0x21
#define PM_RUN1     0x30
#define PM_RUN2     0x31
#define PM_END1     0x40
#define PM_END2     0x41

#define PM_FREERUN  0
#define PM_ONESHOOT 1
#define PM_ACTIF    1


// détecteurs (memDetec)

#define LENDET 8            // nbre bits / détecteur

#define DETBITEN_VB VBIT0     // enable
#define DETBITEN_PB PBIT0
#define DETBITCU_VB VBIT1     // état courant
#define DETBITCU_PB PBIT1
#define DETBITLE_VB VBIT2     // local/externe
#define DETBITLE_PB PBIT2
#define DETBITNU_VB VBIT3     // numéro (3 bits)
#define DETBITNU_PB PBIT3
#define DETBITDI_VB VBIT6     // dispo
#define DETBITDI_PB PBIT6

// codage car différenciation de fonction

#define PMFNCVAL    0X30    // car d'offset ('0') de la valeur de fonction (*valf)

#define PMFNCHAR    0x40    // car d'offset ('@') dans nom de fonction
#define PMFNSWLS_PB PBIT4   // position ls bit du numéro de sw

#endif // _SHCONST_H_
