#ifndef _CONST_H_
#define _CONST_H_

//#define WEMOS

#define _MODE_DEVT    // change l'adresse Mac de la carte IP, l'adresse IP et le port
#define VERSION "1.1g"
/* 1.1 ajout voltage dans données data_read_ ; modif unpackMac
 * 1.1a ajout volts et version dans table
 * 1.1b suppression dht ; ajout periDetVal et periIntVal avec affichage/saisie dans la table ; gestion serveur dev
 * 1.1c ajout periIpAddr avec aff dans la table        
 * 1.1d ajout periIntPulse, periIntNb, periDetSs, periDetOo avec affichage dans la table et tfr aux périphériques
 *      ajout longueur message après GET / pour com avec serveur périph (les modifs de messages ne perturbent plus le crc)
 * 1.1e modif gestion des données reçues avec GET ou POST : ajout tableau d'offsets dans valeurs[] (nvalf[]) pour ne plus 
 *      perdre de l'espace avec une longueur de données fixe. Les données sont séparées par '\0';
 *      Rédaction des "principes" et mise en lib de shconst.h, shutil et messages : 
 *            conversion du soft (version 1.9 pour périphériques).
 *      Peritable : N°,nom,temp,freq,pitch,nbre int*(etat,cde,timer,nbre det, état), mac addr,ip addr,last in time,last out time
 *                  ajout bouton "refresh" et config per refr.
 *      ntp cassé
 * 1.1f restructuration communications (dataread/save;set;ack)      
 *      shmess reçoit assySet, inpSet (à venir assyRead/Save inpRead/Save)
 * 1.1g suppression accueil et fonctions associées, mise en place perisend(cliext)      
 *     
 *      
 * à faire :
 *     
 *      créer une table mac/ip/date-heure des password valides (qui ont donné un mot de passe ok) et effacer après un délai d'inutilisation
 *      bug sur les enregitrements de peritable : les valeurs de l'enregitrement précédent se propagent... (créer 3 peri, mettre l'addr mac du 2nd à 0, redémarrer)
 */

#define DS3231_I2C_ADDRESS 0x68         // adresse 3231 sur bus I2C

#define PINDHT11 2


#ifdef _MODE_DEVT
#define IPMACADDR {0x90, 0xA2, 0xDA, 0x0F, 0xDF, 0xAC}   //adresse mac carte ethernet AB service ; AC devt
#define LOCALSERVERIP {192, 168, 0, 35}                  //adresse IP    ---- 34 service, 35 devt
#define PORTSERVER 1790
#define NOMSERV "sweet hdev"
#endif _MODE_DEVT

#ifndef _MODE_DEVT
#define IPMACADDR {0x90, 0xA2, 0xDA, 0x0F, 0xDF, 0xAB}   //adresse mac carte ethernet AB service ; AC devt
#define LOCALSERVERIP {192, 168, 0, 34}                  //adresse IP    ---- 34 service, 35 devt
#define PORTSERVER 1789
#define NOMSERV "sweet home"
#endif _MODE_DEVT


#define LENNOM 10   // nbre car nom de fonction
#define NBVAL 64    // nbre maxi fonct/valeurs pour getnv() --- les noms de fonctions font 10 caractères + 2 séparateurs + la donnée associée = LENVAL
                    // taille macxi pour GET (IE)=2048  ; pour POST plusieurs mega 
                    // avec un arduino Atmel la limitation vient de la taille de la ram
#define LENVAL 52   // nbre car maxi valeur 
#define MEANVAL 6   // pour donner un param de taille à valeurs[]
#define LENVALEURS NBVAL*MEANVAL+1           // la taille effective de valeurs[]
#define LENPSW 16   // nbre car maxi pswd
#define RECHEAD 28                           // en-tete date/heure/xx/yy + <br> + crlf
#define RECCHAR NBVAL*(MEANVAL+1)+RECHEAD+8  // longueur maxi record histo

#define PERIPREF 60                          // periode refr perif par défaut
#define NBPERIF 10                           
#define PERINAMLEN 16+1                      // longueur nom perif
#define PERIRECLEN 162 // V1.1f              // longueur record périph


#define SDOK 1
#define SDKO 0 

//enum {FAUX,VRAI};

#define AFF_getdate  1
#define AFF_getnv    2
#define AFF_temp_hum 4
#define AFF_dumpSD   8
#define AFF_SDstore  16
#define AFF_SDerr    32
#define AFF_SDhtml   64
#define AFF_Ether   128

/* codes fonctions
00 per_temp__ valorisation période de la mesure et enregistrement de température du serveur
01 
02 peri_pass_ mot de passe periphériques 
03 dump_sd___ affichage page dump contenu SD
04 
05 
06 per_refr__ valorisation periode rafraichissement de la page d'accueil
07 
08 per_date__ valorisation periode de l'enregistrement date dans SD
09 reset_____ reset serveur
10 password__ affichage page saisie mdp
11 sd_pos____ valorisation pointeur pour dump sd
12 data_save_ données formatées pour maj table et enregistrement dans SD 
13 data_read_ demande de paramètres pour un périphérique au serveur 
14 peri_parm_ (rien... actuellement accueil... à recycler)
15 peri_cur__ valorisation numéro périphérique courant pour la table
16 peri_refr_ valorisation periode de mesure/action du périphérique courant
17 peri_nom__ valorisation nom du périphérique courant
18 peri_mac__ valorisation adresse Mac du périphérique courant
19 acceuil___ affichage page d'accueil du serveur
20 peri_table affichage page table des périphériques
21 peri_prog_ valorisation flag "programmable" du périphérique courant
22 peri_sonde valorisation flag "sonde" du périphérique courant
23 peri_pitch valorisation valeur "pitch" (écart significatif de mesure) du périphérique courant
24 peri_inter *** obsolète ** remplacé par peri_intnb valorisation flag "interrupteur" du périphérique courant
25 peri_detec valorisation nombre de détecteurs sur le périphérique
26 peri_intnb valorisation nombre d'interrupteurs sur le périphérique
27 peri_intv0 valorisation de l'état de l'interrupteur 0 dans *periIntVal (4 interrupteurs possibles, 2 bits par inter)
28 peri_intv1 valorisation de l'état de l'interrupteur 1 dans *periIntVal (4 interrupteurs possibles, 2 bits par inter)
29 peri_intv2 valorisation de l'état de l'interrupteur 2 dans *periIntVal (4 interrupteurs possibles, 2 bits par inter)
30 peri_intv3 valorisation de l'état de l'interrupteur 3 dans *periIntVal (4 interrupteurs possibles, 2 bits par inter)
31 
32 
33 
34 
35 peri_puls0 valorisation durée de l'impulsion (on ou off) si 0 pas d'impulsion sinon durée en secondes
36 peri_puls1
37 peri_puls2
38 peri_puls3
39 

  PRINCIPES DE FONCTIONNEMENT

  Le frontal utilise le protocole http pour communiquer avec les périphériques et l'utilisateur

  Il y a 2 types de connexions possibles :
    1) mode serveur capable de recevoir des commandes GET et POST et d'envoyer des pages html 
    2) mode client des périphériques serveurs dont il a l'adresse IP capable d'envoyer des commandes et recevoir des pages html

  Les périphériques ont au minimum un mode client (lorsqu'ils fonctionnent sur batteries en particulier).

  Le frontal et les périphériques échangent des "messages" encapsulés soit dans des commandes GET ou POST
  soit dans le <body></body> d'une page html selon le mode de connexion utilisé.
  
  La totalité des "messages" est imprimable. Un "message" peut contenir plusieurs fonctions concaténées séparées par "?"
  Les fonctions sont de de la forme :
      
      nom_fonction/longueur/[données]/crc 
          
      Le nom de fonction fait 11 caractères ; il se termine par '=', il ne contient pas d'espaces.
      La longueur des données est au maximum est de 9999 caractères (4 digits complétés avec des 0 à gauche).
      Le crc est sur 2 caractères (1 octet hexadécimal représenté en ascii). Il ne prend pas en compte le nom de fonction.

      Une fonction a donc une longueur minimale de 17 caractères et maximale de 10016.

      Le frontal peut aussi recevoir via GET ou POST des fonctions en provenance des pages html qu'il a fourni à des navigateurs. 
      Elles n'ont ni longueur ni crc.

  Une couche d'encryptage sera ajoutée lorsque l'ensemble sera stabilisé.

  FONCTIONS

  (voir liste des fonctions du serveur du frontal au-dessus)
  (fonctions des périphériques serveurs : etat______ ; set_______ ; ack_______ ; reset_____ ; sleep_____ )

  Certaines n'ont pas d'arguments donc il n'y a pas de données dans les messages correspondants.
    (pour assurer la variabilité des encryptages ultérieurs, des données aléatoires seront insérées)
  Certaines ont une simple valeur numérique comme argument.
  Certaines ont une collection d'arguments (avec ou sans séparateurs). 

  L'ordre des arguments est fixe. La liste se termine au premier absent.

  PRINCIPE DES ECHANGES

  Table des périphériques :
  
  Le frontal reçoit de façon asynchrone des messages avec les fonctions (GET)data_read (réponse (page.html)set_______) et (
  (GET)data_save_ (réponse (page.html)ack_______.)
  Les données contenues dans ces messages sont enregistrées dans la carte SD sous la forme d'une table des périphériques.
  La dernière réception est datée.
  L'affichage de la table via un navigateur provoque une demande d'état ((GET)etat______) à tous les périphériques serveurs.
  La réponse normale est (GET)data_save_ (qui génère (page.html)ack_______) ; elle n'est pas attendue.
  Sur action de l'utilisateur via un navigateur, la configuration d'un périphérique peut être modifiée ce qui génère 
  l'envoi d'un message (GET)set_______ (réponse non attendue : (GET)data_save_ suivie de (page.html)ack_______ ).
  Le dernier envoi de message est daté.
  Lorsqu'un message sortant ou entrant se déroule anormalement, l'incident est noté et daté. 
  En cas d'anomalie, les périphériques doublent la période de com au serveur jusqu'à 7200sec.

  Le frontal ne reçoit des messages que par GET et POST (donc les périphériques n'envoient pas de pages html).
  Un périphérique sans serveur ne reçoit des messages que par page html.
  Un périphérique avec serveur reçoit des messages par les 2 modes.
  Le frontal envoie des messages par les 2 modes.

  Historique :

  Un fichier séquentiel accumule et date tous les échanges qui se produisent.

  MISE EN OEUVRE

  (le mot fonction désigne maintenant les fonctions du C)

  Pour l'envoi de message :

  int messToServer(*client,const ip,const port,const *données)  connecte à un serveur http et envoie un message
          renvoie  0 connexion pas réussie 
                ou 1 ok, la donnée est transmise, la connexion n'est pas fermée, le client est valide

  void periSend()  envoie une page html minimale contenant un message set___ dans <body></body>
          le client doit être connecté ; pas de contrôle                                
          periSend utilise periParamsHtml(*client,*host,port) avec port!=0

  int periParamsHtml(client,char* host,int port) assemble et envoie un message set___
          si port=0 dans page html en réponse à une requête GET ou POST ; sinon via une commande GET
          utilise messToServer

  Pour la réception de message :

  int getHttpResponse(*client,*données,*diag,lmax,*fonction)     attente d'une page html, 
          reçoit et contrôle un message d'une seule fonction
          le client doit être connecté ; renvoie un code MESSxx (voir shconst.h) 
                                                 diag pointe une chaine imprimable selon le code.
                                                 le numéro de fonction reçue, la donnée (nnnn...cc)
          contrôle du temps, de dépassement de capacité, de la longueur et du crc 
          
  le frontal comme les périphériques sont dans une boucle d'attente de connexion pour la réception de messages via GET OU POST
  
  Pour le controle de messages :

  int checkData(*données)    renvoie un code MESSxx (voir shconst.h)

  int checkHttpData(*données, *fonction)   utilise checkData renvoie un code MESSxx et le numéro de la fonction

  Pour l'assemblage des messages :

  int buildMess(*fonction, *données, *séparateur)      utilise bufServer espace permanent de bricolage
                                                       renvoie 0 si décap (ctle préalable) longueur totale sinon
                                                       les messages sont concaténés et le séparateur inséré

  void assySet(char* message,int pericur,char* diag,char* date14);
                                                       assemble les données des fonction Set/Ack

  int8_t readFonc(char* message,uint8_t foncWaited)    reçoit et des-assemble les données des fonction Set/Ack
                                                       utilise bufServer espace permanent de bricolage
                                                       renvoie periMess (MESSFON si inex ou invalide)
                                                       
  int read_save(char* fonction,char* data)             assemble et envoie dataread/datasave
  

  STRUCTURE TABLE DES PERIPHERIQUES

      Un fichier dans la carte SD par ligne avec numéro de ligne dans le nom du fichier
        
      Elements de table : N°,nom,temp,freq,pitch,nbre switchs*(etat,cde,mode,timer(mode,durées,état),nbre det*(mode,état)
                           mac addr,ip addr,last in time,last out time,last com error(time,code)

      Principe des opérations :

        vocabulaire : 
                activation/désactivation
                      un switch activé peut être manipulé (ON ou OFF) par la source qui l'active
                      desactivé il est inerte et ne peut changer d'état tant qu'il existe au moins une source désactivante (sauf forçage).
                      la désactivation est prioritaire sur l'activation.
                forçage
                      le switch prend la valeur imposée 
                      le forçage ON est prioritaire sur l'activation/désactivation
                      le forçage OFF est prioritaire sur tout
         
         mécanisme :
                scrutation des forçages OFF ; si l'un est valide (enable et source dans l'état spécifié), le switch est positionné OFF, fin
                sinon scrutation des forçages ON ; si l'un est valide, le switch est positionné ON, fin
                sinon scrutation des désactivations d'impulsion ; si l'une est valide, fin
                sinon scrutation des activations ; si l'une est "enable" prendre sa position selon la table : HH=H, HL=L, LH=L, LL=H
                      
         3 sources différentes pour forçages et activation/désactivation d'un switch : le serveur, un détecteur parmi 4, son générateur d'impulsions
                chaque source dispose d'un bit "enable" qui la démasque et d'un bit H/L (active haute ou basse)

         mécanisme générateur d'impulsion :
                2 phases de durée paramétrée ; 
                  si la 2nde phase est enable, elle se déclenche à la fin de la première
                  si la première a le bit freeRun set elle se déclenche a la fin de la seconde
                la phase déclenchée peut commander l'état d'un switch via son bit H/L dans lae paramétrage des sources (si bit enable set)
                
                En mode one-shot ou pour démarrer le free-run Le déclenchement de la première phase provient d'un détecteur ou du serveur
                  
                si impulsion arrêtée (durée = curr) controler condition de démarrage :
                  bit tONE enable et source de déclenchement valide
                si impulsion en cours (durée!=curr) et enable màj curr (maxi = durée)
                  (soit tONE soit tTWO en cours ; forçage fin = arrêt : curr = durée) 
                si tONE en fin (durée=curr) start tTWO si enable
                si tTWO en fin si mode oneshoot fin sinon start tONE si enable
         
         valeur d'état générateur :
                valeur courante de l'impulsion en cours selon phase
                si arrêt -> pulse invalide

         les détecteurs sont représentés par une variable en mémoire ; pas d'accès direct aux ports
                le changement d'état d'un port et la mise à jour de la variable sont gérés par une ISR.
                le débounce est intégré dans la loop
                la variable mémoire stocke la valeur courante et la valeur précédente.
                la valeur précédente est mise à jour lors de la lecture par le gestionnaire des impulsions

*  int8          numéro
*  16 bytes      nom  
*  uint8         nbre switchs    maximum 4 (MAXSW)
*  byte          etat/cde        2 bits par switch (etat/cde)
*  16 bytes (4 / switch)
*     1 byte        sources activation         2 bits N° detec
*                                              1 bit détec enable 
*                                              1 bit HIGH/LOW active
*                                              1 bit server cde enable
*                                              1 bit H/L active
*                                              1 bit pulse enable
*                                              1 bit H/L active  
*     1 byte        sources désactivation 
*     1 byte        sources forçage ON
*     1 byte        sources forçage OFF
*  66 bytes (16+2 bytes / switch) 
*     4 bytes       durée ONE 4Giga (ou 99 999 999) * 1/100sec   (stop si durée=curr ; start si condition remplie))
*     4 bytes       curr tONE
*     4 bytes       durée TWO 
*     4 bytes       curr tTWO
*     2 bytes       controle                   1 bit tONE enable
*                                              1 bit tTWO enable
*                                              1 bit stop/start serveur (passe à stop en fin d'impulsion en mode one-shot)
*                                              1 bit détecteur déclenchement enable
*                                              2 bits numéro détecteur pour déclenchement pulseONE (4 détec) 
*                                              1 bit declenchement sur état ou front
*                                              1 bit déclenchement sur H/L
*                                              1 bit détecteur arrêt enable
*                                              2 bits numéro détecteur arrêt pulseONE / start pulseTWO (si bit tTWO enable set) / arrêt pilseTWO
*                                              1 bit arrêt sur état ou front
*                                              1 bit arrêt sur H/L
*                                              1 bit mode (one shoot/free run)
*                                              1 bit phase H->H-L L->L-H
*                                              1 bits dispos
*  uint8         nbre détecteurs maximum 4 (MAXDET)
*  byte          1 bits etat ; 1 bit enable
*  uint8         nbre sondes     maximum 256 (MAXSDE)
*  6 bytes       MacAddr
*  4 bytes       IpAddr
*  6 bytes       temps dernier envoi (AAMMJJHHMMSS BCD)
*  6 bytes       temps derniere réception
*  6 bytes       temps dernière anomalie com
*  1 byte        code dernière anomalie (MESSxxx)
*  2 bytes       version         XX.XX BCD
*  
total 121

    fichier sondes (contient numéro de périphérique et numéro de sonde) (à implanter)

    6 bytes     MacAddr périphérique
    uint8       numéro sonde
    byte        type            (1 temp, 2 tension, 3 courant, vitesse etc...
    float       valeur courante
    uint16      période         période d'appel au serveur (secondes)
    uint8       pitch           variation minimum significative 
    6 bytes     temps dernière valeur
    1 byte      param           (bit 0 affichage O/N, bit 1 présente O/N, ... 
    
*/

#endif // _CONST_H_
