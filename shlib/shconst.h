#ifndef _SHCONST_H_
#define _SHCONST_H_


//#define PERIF

#define PORTSERVPERI 1791

#ifdef PERIF
#define PINLED 0                    //  0 = ESP-12  ; 2 = ESP-01
#define LEDON LOW
#define LEDOFF HIGH
#endif PERIF
#ifndef PERIF
#define PINLED 13
#define LEDON HIGH
#define LEDOFF LOW
#endif ndef PERIF

    // messages

#define LENMESS     190             // longueur buffer message
#define MPOSFONC    0               // position fonction
#define MPOSLEN     11              // position longueur (longueur fonction excluse)
#define MPOSNUMPER  MPOSLEN+5       // position num périphérique
#define MPOSMAC     MPOSNUMPER+3    // position adr Mac
#define MPOSDH      MPOSMAC+18      // Date/Heure
#define MPOSPERREFR MPOSDH+15       // période refr
#define MPOSPITCH   MPOSPERREFR+6   // pitch
#define MPOSINTCDE  MPOSPITCH+5     // 4 commandes sw 0/1 (periIntVal)
#define MPOSINTPAR0 MPOSINTCDE+5    // paramétres sw (4*HH+1sep)*4
#define MPOSPULSON0 MPOSINTPAR0+MAXSW*9 // Timers On (4bytes)*4
#define MPOSPULSOF0 MPOSPULSON0+MAXSW*9 // Timers Off (4bytes+1sep)*4
#define MPOSPULSMOD MPOSPULSOF0+MAXSW*9 // paramètres timers (2*HH+1sep)*4
#define MPOSMDIAG   MPOSPULSMOD+MAXSW*5 // texte diag
#define MLMSET      MPOSMDIAG+5     // longueur message fonction incluse

    // fonctions

#define LENNOM 10   // nbre car nom de fonction
#define NBVAL 28    // nbre maxi fonct/valeurs pour getnv() --- les noms de fonctions font 10 caractères + 2 séparateurs + la donnée associée = LENVAL
                    // taille macxi pour GET (IE)=2048  ; pour POST plusieurs mega
                    // avec un arduino Atmel la limitation vient de la taille de la ram
#define LENVAL 64   // nbre car maxi valeur (vérifier datasave)
#define MEANVAL 6   // pour donner un param de taille à valeurs[]
#define LENVALEURS NBVAL*MEANVAL+1   // la taille effective de valeurs[]
#define LENPSW 16   // nbre car maxi pswd
#define RECHEAD 28                          // en-tete date/heure/xx/yy + <br> + crlf
#define RECCHAR NBVAL*(MEANVAL+3)+RECHEAD+8  // longueur maxi record histo

#define PERIPREF 900                        // periode refr perif par défaut

#define LBUFSERVER RECCHAR

#define SRVPASS "17515A\0"
#define LENSRVPASS 6

#define TOINCHCLI 4000        // msec max attente car server
#define TO_HTTPCX 4000        // nbre maxi retry connexion serveur

#define SLOWBLINK 2000
#define FASTBLINK 500
#define PULSEBLINK 40
/* code blink  (valeurs impaires bloquantes) */
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

#define PMTOE_PB PBITE      // pulse Mode time one enable pos bit
#define PMTOE_VB VBITE      // val bit
#define PMTTE_PB PBITD      // time two enable pos bit
#define PMTTE_VB VBITD
#define PMSRE_PB PBITC      // server enable pos bit
#define PMSRE_VB VBITC
#define PMFRE_PB PBITB      // free run pos bit
#define PMFRE_VB VBITB
#define PMPHE_PB PBITA      // phase pos bit
#define PMPHE_VB VBITA
#define PMDINMS_PB PBIT9    // Det on number pos ms bit
#define PMDINMS_VB VBIT9
#define PMDINLS_PB PBIT8    // det on number pos ls bit
#define PMDINLS_VB VBIT8
#define PMDONMS_PB PBIT7    // det off number pos ms bit
#define PMDONMS_VB VBIT7
#define PMDONLS_PB PBIT6    // det off number pos ls bit
#define PMDONLS_VB VBIT6
#define PMDIE_PB PBIT5      // det on enable pos bit
#define PMDIE_VB VBIT5
#define PMDIU_PB PBIT4      // det on UP/DOWN pos bit
#define PMDIU_VB VBIT4
#define PMDIH_PB PBIT3      // det on H/L pos bit
#define PMDIH_VB VBIT3
#define PMDOE_PB PBIT2
#define PMDOE_VB VBIT2
#define PMDOU_PB PBIT1
#define PMDOU_VB VBIT1
#define PMDOH_PB PBIT0
#define PMDOH_VB VBIT0

#define PMDIN_MSK (PMDINMS_VB | PMDINLS_PB)>>PMDINLS_PB // mask de la valeur du numéro de détecteur on
#define PMDON_MSK (PMDONMS_VB | PMDONLS_VB)>>PMDONLS_PB // mask de la valeur du numéro de détecteur off

// codage car différenciation de fonction

#define PMFNCVAL    0X30    // car d'offset ('0') de la valeur de fonction (*valf)
#define PMSWCVAL    0x30    // car d'offset ('0') du num de switch dans le nom de fonction

#define PMFNCHAR    0x40    // car d'offset ('@') dans nom de fonction
#define PMFNSWLS_PB PBIT4   // position ls bit du numéro de sw

#endif // _SHCONST_H_
