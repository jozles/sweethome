#ifndef _SHCONST_H_
#define _SHCONST_H_


#define PERIF

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

#define TOINCHCLI 4000          // msec max attente car server


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


#endif // _SHCONST_H_
