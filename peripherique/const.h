#ifndef CONST_H_INCLUDED
#define CONST_H_INCLUDED

#define VERSION "1.a_"
/* 1.1 allumage/extinction modem
 * 1.2 ajout voltage (n.nn) dans message ; modif unpackMac
 * 1.3 deep sleep (PERTEMP) ; gestion EEPROM ; conversion temp pendant sleep
 * 1.4 IP fixe ; system.option(4 )modem off ; RTCmem
 * 1.5 correction calcul temp (ds1820.cpp) ; 
 *     tempage remplacé par dateon pour mémo durée connexion (durée totale env 1,25 durée jusqu'à datasave)
 * 1.6 IP en ram RTC produit par le router à la mise sous tension 
 *     en cas de raz de numperiph, raz IP pour produire nouvelle demande au routeur.
 * 1.7 talkServer renvoie 0 ou NBRETRY si pas de connexion.
 *     prochaine lecture température (donc tentative cx) dans 2 heures + dataread
 *     installation ssid2/password2
 * 1.8 ajout PINSWA et B et PINDET + leur transfert dans dataread et datasave
 *     ajout compilation non _DS_MODE ; 
 *     ajout led sur pin 0 ;
 *     option S DS1820 (ds1820.cpp) ; 
 *     Mode server avec décodage, comptage, action et réponse pour messages OPEN et CLOSE
 *     gestion actionneur manuel sur interruptions
 *     longueur message en HEXA ASCII après GET / pour comm avec clients
 * 1.9 normalisation du format de communication
 *     automate pour talkServer
 * 1.a paramètres pour switchs, pulse etc ; 3 modes pour l'alim : DS PO NO    
 *     réception commandes (ordreExt) opérationnelle (cdes testa_on__ testb_on__ testaoff__ testboff__)
 *     config hardware selon carte.
 *  
Modifier : 
  
  alarme lorsque connexion impossible... 
  trouver une combine pour charger ssid et password sans reprogrammer le 8266
  idem pour adresse IP et port du host
  ---> soluce : connexion usb sur le module
*/

#include "Arduino.h"
#include <ESP8266WiFi.h>
#include <shconst.h>

#define PERIF       // pour compatibilité avec shconst etc

/* Modes de fonctionnement  */
/*
   2 cycles emboités :
   
      1) timing de base = période de lecture de la sonde thermomètre ; 
            si timer externe c'est la période d'allumage,
            sinon PERTEMP est la période par défaut en Sec,
            cstRec.tempPer la période courante (directement utilisé dans le timer si deep sleep),
            et cstRec.tempTime le point de départ en millis() si loop.
            Donc, si millis()>temptime+tempPer lecture thermo et +tempPer à serverTime (ci-après)
            Si la valeur lue a plus de cstRec.pitch d'écart avec la valeur précédente -> accés serveur
      2) fréquence d'accés au serveur pour enregistrer une/des valeurs de sondes/détecteurs
            comptage du timing de base commun à tous les modes
            PERSERV est la période par défaut en "timing de base"
            cstRec.serverPer la période courante en secondes.
            et cstREc.serverTime le compteur de "timing de base"
            Donc, si serverTime>serverPer accès server (talkServer)
            
            Gestion totale dans readTemp().

    Stockage des constantes :

        en mode Power Off (PO_MODE) les variables sont stockées en EEPROM 
        dans les autres modes mémoire RTC 
        
        Dans tous les cas :
            la première variable est la longueur totale de la structure (maxi 256)
            la dernière variable le crc de ce qui le précède
            l'avant dernière variable la version du soft qui a alimenté la structure
            Les accés se font via 4 fonctions : 
                readConstant() (ctle du crc), writeConstant() (géné crc), initConstant() et printConstant
*/
#define DS_MODE 'D' // deep Sleep     (pas de loop ; pas de server ; ESP12 reset-GPIO6 connected)
#define PO_MODE 'P' // power off Mode (pas de loop ; pas de server ; ESP01 GPIO3/RX-Done connected)
#define NO_MODE 'N' // No stop mode   (loop et server)

#define VR      'V'
#define THESP01 '1'
#define THESP12 '2'

#define CARTE THESP01                 // <------------- modèle carte
#define POWER_MODE PO_MODE            // <------------- mode compil 

#if POWER_MODE==NO_MODE
  #define _SERVER_MODE
  /* Mode server */
#endif PM==NO_MODE

// stockage deep sleep / power off  (EEPROM pour PO_MODE seul )

#define EEPROMSAVED 'E'
#define RTCSAVED    'R'

#if  POWER_MODE!=PO_MODE
  #define CONSTANT RTCSAVED
  #define CONSTANTADDR 64    // adresse des constantes dans la mémoire RTC (mots 4 octets = 256)
//  #define NBRECEEPROM 53  
// 525000 minutes par an soit 10,5 millions sur 20 ans soit 105 records @ 100K Eeprom write
#endif PM!=PO_MODE

#if POWER_MODE==PO_MODE
  #define CONSTANT EEPROMSAVED
  #define CONSTANTADDR 0   // adresse des constantes dans l'EEPROM
#endif PM==PO_MODE


// matériel

#if CARTE==VR
#define WPIN   4    // 1 wire ds1820
#define NBSW   2    // nbre switchs
#define PINSWA 5    // pin sortie switch A
#define CLOSA  1    // valeur pour fermer (ouvert=!CLOSA)
#define OPENA  0
#define PINSWB 2    // pin sortie switch B
#define CLOSB  0    // valeur pour fermer (ouvert=!CLOSB)
#define OPENB  1
#define PINSWC 5    // pin sortie switch C
#define CLOSC  1    // valeur pour fermer (ouvert=!CLOSA)
#define OPENC  0
#define PINSWD 2    // pin sortie switch D
#define CLOSD  0    // valeur pour fermer (ouvert=!CLOSB)
#define OPEND  1
#define NBDET  4
#define PINDTA 12   // pin entrée détect bit 0 
#define PINDTB 14   // pin entrée détect bit 1 
#define PINDTC 13   // pin entrée détect bit 2  sur carte VR 3 entrées donc bit 2 et 3
#define PINDTD 13   // pin entrée détect bit 3  sur la même entrée.
#define PININTA 12  // in interupt
#define PININTB 14  // in interupt
#define PINPOFF 3   // power off TPL5111 (RX ESP01)
#endif CARTE==VR

#if CARTE==THESP01
#define WPIN   2    // ESP01=GPIO2 ; ESP12=GPIO4 ... 1 wire ds1820
#define NBSW   0    // nbre switchs
#define PINSWA 5    // pin sortie switch A
#define CLOSA  1    // valeur pour fermer (ouvert=!CLOSA)
#define OPENA  0
#define PINSWB 5    // pin sortie switch B
#define CLOSB  1    // valeur pour fermer (ouvert=!CLOSB)
#define OPENB  0
#define PINSWC 5    // pin sortie switch C
#define CLOSC  1    // valeur pour fermer (ouvert=!CLOSA)
#define OPENC  0
#define PINSWD 5    // pin sortie switch D
#define CLOSD  1    // valeur pour fermer (ouvert=!CLOSB)
#define OPEND  0
#define NBDET  0
#define PINDTA 5    // pin entrée détect bit 0 
#define PINDTB 5    // pin entrée détect bit 1 
#define PINDTC 5    // pin entrée détect bit 2  sur carte VR 3 entrées donc bit 2 et 3
#define PINDTD 5    // pin entrée détect bit 3  sur la même entrée.
#define PININTA 5   // in interupt
#define PININTB 5   // in interupt
#define PINPOFF 3   // power off TPL5111 (RX ESP01)
#endif CARTE==THESP01

#if CARTE==THESP12
#define WPIN   4    // ESP01=GPIO2 ; ESP12=GPIO4 ... 1 wire ds1820
#define NBSW   2    // nbre switchs
#define PINSWA 5    // pin sortie switch A
#define CLOSA  1    // valeur pour fermer (ouvert=!CLOSA)
#define OPENA  0
#define PINSWB 2    // pin sortie switch B
#define CLOSB  0    // valeur pour fermer (ouvert=!CLOSB)
#define OPENB  1
#define PINSWC 5    // pin sortie switch C
#define CLOSC  1    // valeur pour fermer (ouvert=!CLOSA)
#define OPENC  0
#define PINSWD 2    // pin sortie switch D
#define CLOSD  0    // valeur pour fermer (ouvert=!CLOSB)
#define OPEND  1
#define NBDET  4
#define PINDTA 12   // pin entrée détect bit 0 
#define PINDTB 14   // pin entrée détect bit 1 
#define PINDTC 13   // pin entrée détect bit 2  sur carte VR 3 entrées donc bit 2 et 3
#define PINDTD 13   // pin entrée détect bit 3  sur la même entrée.
#define PININTA 12  // in interupt
#define PININTB 14  // in interupt
#define PINPOFF 3   // power off TPL5111 (RX ESP01)
#endif CARTE==THESP12


// timings

#define TCONVERSION 375         // millis délai conversion temp
#define PERTEMP 60              // secondes période par défaut lecture temp
#define PERTEMPKO 7200          // secondes période par défaut lecture temp si connexion wifi ko
#define PERSERV 3600            // secondes période max entre 2 accès server
#define TOINCHCLI 4000          // msec max attente car server
#define WIFI_TO_CONNEXION 5000  // msec
#define NBRETRY 3               // wifiConnexion
#define TSERIALBEGIN 100
#define TDEBOUNCE 50            // msec
#define PERACT 100              // millis période scrutation actions switchs 



typedef struct {
  uint8_t   cstlen;
  char      numPeriph[2];
  uint16_t  serverTime;       // temps écoulé depuis la dernière cx au serveur
  uint16_t  serverPer;        // période (sec) de cx au serveur
  uint16_t  tempPer;          // période (sec) de mesure thermomètre (PERTEMP ou PERTEMPKO)
  uint8_t   tempPitch;        // seuil de variation de la temp pour cx au serveur
  int16_t   oldtemp;
  uint8_t   talkStep;           // pointeur pour l'automate talkServer()
  byte      intCde;             // 2 bits par sw cde/état = *periIntVal = bits 8(sw4), 6(sw3), 4(sw2), 2(sw1) commande
  byte      actCde[MAXSW];      // cdes d'activation (MAXSW=4 1 mot)
  byte      desCde[MAXSW];      // cdes désactivation
  byte      onCde[MAXSW];       // cdes forçage ON
  byte      offCde[MAXSW];      // cdes forçage OFF
  uint16_t  pulseMode[MAXSW];   // mode pulse
  uint32_t  intIPulse[MAXSW];   // durée pulse ON
  uint32_t  intOPulse[MAXSW];   // durée pulse OFF
  IPAddress IpLocal;            // 1 mot
  char      cstVers[4];       
  uint8_t   cstcrc;             // doit toujours être le dernier : utilisé pour calculer sa position
                        // total 76 = 19 mots
} constantValues;

/*enum rst_reason {
 REASON_DEFAULT_RST      = 0,   // normal startup by power on 
 REASON_WDT_RST          = 1,   // hardware watch dog reset 
 REASON_EXCEPTION_RST    = 2,   // exception reset, GPIO status won't change 
 REASON_SOFT_WDT_RST     = 3,   // software watch dog reset, GPIO status won't change 
 REASON_SOFT_RESTART     = 4,   // software restart ,system_restart , GPIO status won't change 
 REASON_DEEP_SLEEP_AWAKE = 5,   // wake up from deep-sleep 
 REASON_EXT_SYS_RST      = 6    // external system reset 
};
*/

#endif // CONST_H_INCLUDED


