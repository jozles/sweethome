
#include <SPI.h>      //bibliothèqe SPI pour W5100
#include <Ethernet.h> //bibliothèque W5100 Ethernet
#include <Wire.h>     //biblio I2C pour RTC 3231
#include <shconst.h>
#include <shutil.h>
#include <shmess.h>
#include "const.h"
#include "utilether.h"
#include "periph.h"
#include "peritable.h"
#include "pageshtml.h"

#include <MemoryFree.h>;


#ifdef WEMOS
  #define Serial SerialUSB
#endif def WEMOS
#ifndef WEMOS
 // #include <avr/wdt.h>  //biblio watchdog
#endif ndef WEMOS

//#define _AVEC_AES
#ifdef _AVEC_AES
#include "aes.h"      //encryptage AES
#define KEY {0x2d,0x80,0x17,0x18,0x2a,0xb0,0xd4,0xa8,0xad,0xf9,0x17,0x8a,0x0b,0xD1,0x51,0x3e}
#define IV  {0x00,0x01,0x02,0x03,0x04,0x05,0x06,0x07,0x08,0x09,0x0a,0x0b,0x0c,0x0d,0x0e,0x0f}
struct AES_ctx ctx;
uint8_t key[16]=KEY,iv[16]=IV;
uint8_t chaine[16+1]={0}; // chaine à encrypter/décrypter ---> void xcrypt()
#endif _AVEC_AES


extern "C" {
 #include "utility/w5100.h"
}

  EthernetClient cli_a;             // client du serveur periph  local
//  EthernetClient cli_b;           // client du serveur browser local  
  EthernetClient cliext;            // client de serveur externe  

/* >>>> config server <<<<<< */

char configRec[CONFIGRECLEN];

  byte* mac;              // adresse server
  byte* localIp;          // adresse server
  int*  portserver;       //
  char* nomserver;        //
  char* usrpass;          // mot de passe browser
  char* modpass;          // mot de passe modif
  char* peripass;         // mot de passe périphériques
  char* ssid;             // MAXSSID ssid
  char* passssid;         // MAXSSID password SSID
  int*  nbssid;           // inutilisé

  byte* configBegOfRecord;
  byte* configEndOfRecord;

EthernetServer periserv(1790);  // port 1789 service, 1790 devt

  uint8_t remote_IP[4]={0,0,0,0},remote_IP_cur[4]={0,0,0,0},remote_IP_Mac[4]={0,0,0,0};
  byte    remote_MAC[6]={0,0,0,0,0,0};

#define TO_PASSWORD 600      // sec
  uint16_t toPassword=0;     // rémanence du mot de passe browser (sec)
  long     timePassword=0;          
  bool     periPassOk=FAUX;  // contrôle du mot de passe des périphériques

/*  EthernetServer monserveur(1792);        // port pour browser
  uint8_t remote_IP_b[4]={0,0,0,0},remote_IP_b_cur[4]={0,0,0,0};
  char    pass_b[]=SRVPASS;           
*/
  byte    macMaster[6]={0,0,0,0,0,0};  // adresse mac d'un browser chargée lors de la saisie du mot de passe dans peritable
                                       // permet d'identifier le browser pour ne pas avoir à re-saisir le mot de passe 
                                       // donne l'habilitation pour effectuer des modifs dans peritable
                                       // (sinon renvoi à l'accueil)
  int8_t  numfonct[NBVAL];             // les fonctions trouvées  (au max version 1.1k 23+4*57=251)
  char*   fonctions="per_temp__macmaster_peri_pass_password__testhtml__done______per_refr__peri_tofs_switchs___reset_____dump_sd___sd_pos____data_save_data_read_peri_swb__peri_cur__peri_refr_peri_nom__peri_mac__accueil___peri_tableperi_prog_peri_sondeperi_pitchperi_pmo__peri_detnbperi_intnbperi_intv0peri_intv1peri_intv2peri_intv3peri_t_sw_peri_otf__peri_imn__peri_imc__peri_pto__peri_ptt__peri_thminperi_thmaxperi_vmin_peri_vmax_peri_dsv__mem_dsrv__ssid______passssid__cfgserv___pwdcfg____modpcfg___peripcfg__maccfg____last_fonc_";  //};
                                       // nombre fonctions, valeur pour accueil, data_save_ fonctions multiples etc
  int     nbfonct=0,faccueil=0,fdatasave=0,fperiSwVal=0,fperiDetSs=0,fdone=0,fpericur=0,fperipass=0,fpassword=0,fmacmaster=0;
  char    valeurs[LENVALEURS];         // les valeurs associées à chaque fonction trouvée
  uint16_t nvalf[NBVAL];               // offset dans valeurs[] des valeurs trouvées (séparées par '\0')
  char*   valf;                        // pointeur dans valeurs en cours de décodage
  char    libfonctions[NBVAL*2];       // les 2 caractères de fin des noms de fonctions
  int     nbreparams=0;                // 
  int     what=0;                      // ce qui doit être fait après traitement des fonctions (0=rien)

#define LENCDEHTTP 6
  char*   cdes="GET   POST  \0";       // commandes traitées par le serveur
  char    strSD[RECCHAR]={0};          // buffer enregistrements sur SD
  char*   strSdEnd="<br>\r\n\0";
  char    buf[6];

  char bufServer[LBUFSERVER];          // buffer entrée/sortie dataread/save

  byte      msb=0,lsb=0;                   // pour temp DS3231
  float     tempf;
  long      curtemp=0;                     // mémo last millis() pour temp
#define PTEMP 3600                         // sec
  uint32_t  pertemp=PTEMP;                 // période ech temp       
  uint16_t  perrefr=0;                     // periode rafraichissement de l'affichage
  
  int   stime=0;int mtime=0;int htime=0;
  long  curdate=0;

/* iùage mémoire détecteurs du serveur */

  byte memDetServ=0;    // image mémoire NBDSRV détecteurs (8)  

/*  enregistrement de table des périphériques ; un fichier par entrée
    (voir periInit() pour l'ordre physique des champs + periSave et periLoad=
*/
  char      periRec[PERIRECLEN];                // 1er buffer de l'enregistrement de périphérique
  char      periCache[PERIRECLEN*(NBPERIF+1)];  // cache des périphériques
  byte      periCacheStatus[(NBPERIF+1)];       // indicateur de validité du cache d'un périph  
  
  uint16_t   periCur=0;                         // Numéro du périphérique courant
  
  uint16_t* periNum;                        // ptr ds buffer : Numéro du périphérique courant
  uint32_t* periPerRefr;                    // ptr ds buffer : période datasave minimale
  float*    periPitch;                      // ptr ds buffer : variation minimale de température pour datasave
  float*    periLastVal;                    // ptr ds buffer : dernière valeur de température  
  float*    periAlim;                       // ptr ds buffer : dernière tension d'alimentation
  char*     periLastDateIn;                 // ptr ds buffer : date/heure de dernière réception
  char*     periLastDateOut;                // ptr ds buffer : date/heure de dernier envoi  
  char*     periLastDateErr;                // ptr ds buffer : date/heure de derniere anomalie com
  int8_t*   periErr;                        // ptr ds buffer : code diag anomalie com (voir MESSxxx shconst.h)
  char*     periNamer;                      // ptr ds buffer : description périphérique
  char*     periVers;                       // ptr ds buffer : version logiciel du périphérique
  char*     periModel;                      // ptr ds buffer : model du périphérique
  byte*     periMacr;                       // ptr ds buffer : mac address 
  byte*     periIpAddr;                     // ptr ds buffer : Ip address
  byte*     periSwNb;                       // ptr ds buffer : Nbre d'interrupteurs (0 aucun ; maxi 4(MAXSW)            
  byte*     periSwVal;                      // ptr ds buffer : état/cde des inter  
  byte*     periSwMode;                     // ptr ds buffer : Mode fonctionnement inters (4 par switch)           
  uint32_t* periSwPulseOne;                 // ptr ds buffer : durée pulses sec ON (0 pas de pulse)
  uint32_t* periSwPulseTwo;                 // ptr ds buffer : durée pulses sec OFF(mode astable)
  uint32_t* periSwPulseCurrOne;             // ptr ds buffer : temps courant pulses ON
  uint32_t* periSwPulseCurrTwo;             // ptr ds buffer : temps courant pulses OFF
  byte*     periSwPulseCtl;                 // ptr ds buffer : mode pulses
  byte*     periSwPulseSta;                 // ptr ds buffer : état clock pulses
  uint8_t*  periSondeNb;                    // ptr ds buffer : nbre sonde
  boolean*  periProg;                       // ptr ds buffer : flag "programmable" (périphériques serveurs)
  byte*     periDetNb;                      // ptr ds buffer : Nbre de détecteurs maxi 4 (MAXDET)
  byte*     periDetVal;                     // ptr ds buffer : flag "ON/OFF" si détecteur (2 bits par détec))
  float*    periThOffset;                   // ptr ds buffer : offset correctif sur mesure température
  float*    periThmin;                      // ptr ds buffer : alarme mini th
  float*    periThmax;                      // ptr ds buffer : alarme maxi th
  float*    periVmin;                       // ptr ds buffer : alarme mini volts
  float*    periVmax;                       // ptr ds buffer : alarme maxi volts
  byte*     periDetServEn;                  // ptr ds buffer : 1 byte 8*enable detecteurs serveur
    
  int8_t    periMess;                       // code diag réception message (voir MESSxxx shconst.h)
  byte      periMacBuf[6]; 

  byte*     periBegOfRecord;
  byte*     periEndOfRecord;

  byte      lastIpAddr[4]; 

/* alimentation DS3231 */

#define PINVCCDS 19   // alim + DS3231
#define PINGNDDS 18   // alim - DS3231

  Ymdhms dt;

//IPAddress servergoogle(74,125,232,128); // google pour l'heure 
/*
 * =========== mécanisme de la librairie 
 * 
 *  Ethernet.begin connecte au réseau (mac, IP dans LAN par DHCP ou forcé) 
 *  
 *  Ethernet.localIP() rend l'adresse IP si elle provient du DHCP
 *  Ethernet.maintain() pour renouveler le bail DHCP
 *                                                                                                        
 *  pour creer/utiliser un serveur : EthernetServer nom_serveur(port) crée l'objet
 *                                   nom_serveur.begin()  activation
 *                                   connexions des appels entrants : nom_client_in=nom_serveur.available()
 *                                   nom_serveur.write(byte) ou (buf,len) envoie la donnée à tous les clients 
 *                                   nom_serveur.print(variable) envoie la variable en ascii
 *                                   
 *  l'objet client est utilisé pour l'accès aux serveurs externes ET l'accès aux clients du serveur interne
 *  EthernetClient nom_client crée les objets clients pour serveur externe ou interne (capacité 4 clients)
 *  
 *  pour connecter un client de serveur externe : nom_client_ext.connect(IP ou nom du serveur,port)  
 *  
 *  pour les 2 types de clients :
 *    nom_client.connected() renvoie l'état de la connexion (indique qu'un message est dispo)
 *    nom_client.available() indique car dispo à lire
 *    nom_client.read() réception d'un car jusqu'à ce que available soit faux
 *    nom_client.write(char) envoie un car au client
 *    nom_client.print(variable) envoie la variable en ascii
 *    nom_client.flush() attente vidage buffer vers client
 *  
 *    en fin de transaction : nom_client.stop() déconnecte le client du serveur 
 *    et on reboucle sur nom_serveur.available ou nom_client.connect selon le cas
 *  
*/


char inch=' ';
char strdate[33]; // buffer date
char strd3[4]={0};
int  wday=0;
byte js=0,js2=0;
uint32_t amj=0, hms=0, amj2=0, hms2=0;
 
File fhisto;            // fichier histo sd card
File fhtml;             // fichiers pages html

char cxdx[]="RE";       // param pour l'enregistrement dans SD (CX connexion, 
uint32_t sdsiz=0;       // SD current size
uint32_t sdpos=0;       // SD current pos. pour dump

boolean autoreset=FAUX;  // VRAI déclenche l'autoreset

int   i=0,j=0;
char  c=' ';
char  b[2]={0,0};
char* chexa="0123456789ABCDEFabcdef\0";
byte  maskbit[]={0xfe,0x01,0xfd,0x02,0xfb,0x04,0xf7,0x08,0xef,0x10,0xdf,0x20,0xbf,0x40,0x7f,0x80};
byte  mask[]={0x00,0x01,0x03,0x07,0x0F};

byte globalEnd;

/* prototypes */

int  getnv(EthernetClient* cli);

int  sdOpen(char mode,File* fileSlot,char* fname);
void conv_atob(char* ascii,uint16_t* bin);
void conv_atobl(char* ascii,uint32_t* bin);

byte decToBcd(byte val);
byte bcdToDec(byte val);
void xcrypt();
//void serialPrintSave();
void periSend();
int  periParamsHtml(EthernetClient* cli,char* host,int port);
void periDataRead();
void packVal2(byte* value,byte* val);
void frecupptr(char* nomfonct,uint8_t* v,uint8_t* b,uint8_t lenpersw);
void bitvSwCtl(byte* data,uint8_t sw,uint8_t datalen,uint8_t shift,byte msk);
void test2Switchs();


void setup() {                              // ====================================

  Serial.begin (115200);delay(1000);
  Serial.print("+");

  pinMode(PINLED,OUTPUT);
  extern uint8_t cntBlink;
  cntBlink=0;

  nbfonct=(strstr(fonctions,"last_fonc_")-fonctions)/LENNOM;
  faccueil=(strstr(fonctions,"accueil___")-fonctions)/LENNOM;
  fdatasave=(strstr(fonctions,"data_save_")-fonctions)/LENNOM;
  fperiSwVal=(strstr(fonctions,"peri_intv0")-fonctions)/LENNOM;
  fdone=(strstr(fonctions,"done______")-fonctions)/LENNOM;
  fpericur=(strstr(fonctions,"peri_cur__")-fonctions)/LENNOM;
  fperipass=(strstr(fonctions,"peri_pass_")-fonctions)/LENNOM;
  fpassword=(strstr(fonctions,"password__")-fonctions)/LENNOM;
  fmacmaster=(strstr(fonctions,"macmaster_")-fonctions)/LENNOM;
  
/* >>>>>>     config     <<<<<< */  
  
  delay(1000);
  Serial.println();Serial.print(VERSION);
  #ifdef _MODE_DEVT
  Serial.print(" MODE_DEVT");Serial.print(" free=");Serial.println(freeMemory(), DEC);
  #endif _MODE_DEVT

  Serial.print("\nSD card ");
  if(!SD.begin(4)){Serial.println("KO");ledblink(BCODESDCARDKO);}
  Serial.println("OK");

  configInit();long configRecLength=(long)configEndOfRecord-(long)configBegOfRecord+1;
  
  Serial.print("CONFIGRECLEN=");Serial.print(CONFIGRECLEN);Serial.print("/");Serial.println(configRecLength);
  delay(100);if(configRecLength!=CONFIGRECLEN){ledblink(BCODECONFIGRECLEN);}
  configPrint();configSave();configLoad();configPrint();
  
  periInit();long periRecLength=(long)periEndOfRecord-(long)periBegOfRecord+1;periCheck(8,"");
  
  Serial.print("PERIRECLEN=");Serial.print(PERIRECLEN);Serial.print("/");Serial.print(periRecLength);
  delay(100);if(periRecLength!=PERIRECLEN){ledblink(BCODEPERIRECLEN);}

  Serial.print(" RECCHAR=");Serial.print(RECCHAR);Serial.print(" LBUFSERVER=");Serial.print(LBUFSERVER);
  Serial.print(" free=");Serial.println(freeMemory(), DEC); 

/* >>>>>>  maintenance fichiers peri  <<<<<< */

/*for(i=0;i<50;i++){
  Serial.print(i);if(i<10){Serial.print(" ");}Serial.print(" ");
  for(j=0;j<10;j++){Serial.print((char)fonctions[i*10+j]);}
  Serial.println();if((strstr(fonctions+i*10,"last")-(fonctions+i*10))==0){i=100;}}
while(1){} 
*/
/* 
    if(SD.exists("fdhisto.txt")){SD.remove("fdhisto.txt");}
    while(1){}
*/    
/*//Modification des fichiers de périphériques   
    Serial.println("conversion en cours...");
    periConvert();
    Serial.println("terminé");
    while(1){};
//*/  
/*  création des fichiers de périphériques
    periInit();
    for(i=11;i<=NBPERIF;i++){
      periRemove(i);
      periCacheStatus[i]=0x01;
      periCur=i;periSave(i);
    }
    while(1){}
*/
/* //correction de valeurs dans les fichiers de périphériques
    Serial.print("correction en cours...");
    periInit();
    for(i=1;i<=NBPERIF;i++){periSave(i);}
    Serial.println("terminé");
    while(1){}
*/
  
  sdOpen(FILE_WRITE,&fhisto,"fdhisto.txt");
  //sdstore_textdh0(&fhisto,".1","RE"," ");

/* >>>>>> ethernet start <<<<<< */

  Wire.begin();

  if(Ethernet.begin(mac) == 0)
    {//Serial.println("Failed with DHCP");
    // **** la connexion mac seul ne fonctionne pas ****    mais le test doit être fait pour que l'UDP marche ???!!!
    Ethernet.begin (mac, localIp); //initialisation de la communication Ethernet
    }
  Serial.print(Ethernet.localIP());Serial.print(" ");
    
  
  periserv.begin();   // serveur périphériques
//  monserveur.begin();  // serveur browser
  
  delay(1000);

#ifndef WEMOS  
 // wdt_enable(WDTO_4S);                // watch dog init (4 sec)
#endif ndef WEMOS

/* >>>>>> RTC ON, check date/heure et maj éventuelle par NTP  <<<<<< */

  digitalWrite(PINGNDDS,LOW);pinMode(PINGNDDS,OUTPUT);  
  digitalWrite(PINVCCDS,HIGH);pinMode(PINVCCDS,OUTPUT);  
  
  i=getUDPdate(&hms,&amj,&js);
  if(!i){Serial.println("pb NTP");ledblink(BCODEPBNTP);} // pas de service date externe 
  else {
    Serial.print(js);Serial.print(" ");Serial.print(amj);Serial.print(" ");Serial.print(hms);Serial.println(" GMT");
    getdate(&hms2,&amj2,&js2);               // read DS3231
    if(amj!=amj2 || hms!=hms2 || js!=js2){
      Serial.print(js2);Serial.print(" ");Serial.print(amj2);Serial.print(" ");Serial.print(hms2);Serial.println(" setup DS3231 ");
      setDS3231time((byte)(hms%100),(byte)((hms%10000)/100),(byte)(hms/10000),js,(byte)(amj%100),(byte)((amj%10000)/100),(byte)((amj/10000)-2000)); // SET GMT TIME      
      getdate(&hms2,&amj2,&js2);sdstore_textdh0(&fhisto,"ST","RE"," ");
    }
  }
  
  sdstore_textdh(&fhisto,".3","RE","<br>\n ");

  Serial.println("fin setup");
}

/*=================== fin setup ============================ */


void getremote_IP(EthernetClient *client,uint8_t* ptremote_IP,byte* ptremote_MAC)
{
    W5100.readSnDHAR(client->getSocketNumber(), ptremote_MAC);
    W5100.readSnDIPR(client->getSocketNumber(), ptremote_IP);
}

void disTrigPwd(){toPassword=0;}
void trigPwd(){startto(&timePassword,&toPassword,TO_PASSWORD);}
bool chkTrigPwd(){return ctlto(timePassword,toPassword);}



// =========================================================================================================

void loop()                             // =========================================
{

/*
    contrôle de la pile
    if(checkStack(originalStack)!=0){---------anomalie-------------}
    initialisation avec :
    char* originalStack=checkStack(0);

char* checkStack(char* refstack)
{
  char ref;  
  return (&ref-refstack);
}
*/


#ifndef WEMOS
//    if(!autoreset){wdt_reset();}        // watch dog reset
#endif ndef WEMOS
  
    if(cli_a = periserv.available())      // attente d'un client
      {
/*
    Si le client est un périphérique, la fonction peripass doit précéder dataread/datasave et le mot de passe est systématiquement contrôlé
    Si le client est un browser (la première fonction n'est pas peripass) et macMaster==ptremote_MAC 
      alors pas de contrôle de mot de passe si le time out du dernier accés n'est pas atteint
*/
        periPassOk=FAUX;
        
        getremote_IP(&cli_a,remote_IP,remote_MAC);

      Serial.print("\n **** loop  IP_cur=");serialPrintIp(remote_IP_cur);Serial.print(" IP_Mac=");serialPrintIp(remote_IP_Mac);Serial.print(" new=");serialPrintIp(remote_IP);
      Serial.print(" MAC=");serialPrintMac(remote_MAC,0);Serial.print("  ");serialPrintMac(macMaster,1);
      
      if (cli_a.connected()) 
        {nbreparams=getnv(&cli_a);Serial.print("\n---- nbreparams ");Serial.println(nbreparams);
        
/*  getnv() décode la chaine GET ou POST ; le reste est ignoré
    forme ?nom1:valeur1&nom2:valeur2&... etc (NBVAL max)
    le séparateur nom/valeur est ':' ou '=' 
    la liste des noms de fonctions existantes est dans fonctions* ; ils sont de longueur fixe (LENNOM) ; 
    les 2 derniers caractères peuvent être utilisés pour passer des paramètres, 
    la recherche du nom dans la table se fait sur une longueur raccourcie si rien n'est trouvé sur la longueur LENNOM.
    (C'est utilisé pour réduire le nombre de noms de fonctions ; par exemple pour periSwMode - 1 car 4 switchs ; 1 car 4 types d'action et 6 bits(sur 16 possibles))
    la liste des fonctions trouvées est dans numfonct[]
    les valeurs associées sont dans valeurs[] (LENVAL car max)
    les 2 derniers car de chaque fonction trouvée sont dans libfonctions[]
    i pointe sur tout ça !!!
    nbreparams le nbre de fonctions trouvées
    il existe nbfonct fonctions 
    la fonction qui renvoie à l'accueil est faccueil (par ex si le numéro de fonction trouvé est > que nbfonct)
    Il y a une fonction par champ de l'enregistrement de la table des périphériques 
    periCur est censé être valide à tout moment si != 0  
    Lorsque le bouton "maj" d'une ligne de la table html des produits est cliqué, les fonctions/valeurs des variables sont listées
    par POST et getnv() les décode. La première est periCur (qui doit etre en premiere colonne).
    Les autres sont transférées dans periRec puis enregistrées avec periSave(periCur).

    Toutes les commandes concernent les browsers clients html sauf 3 : peri_pass_ / data_read_ / data_save_ 
    qui réalisent l'interface avec les périphériques. Format de la donnée : nnnn_...._cc 
    (HTTP est respecté et on peut simuler un périphérique depuis un browser html)
    Lorsqu'un périphérique se connecte pour la première fois, son adresse mac est inconnue du serveur et il n'a pas
    de numéro de périphérique (=0). Il envoie une demande "data_read_".
    Le serveur cherche une place libre et lui attribue le num. correspondant.
    Si son adresse mac est déjà connue il lui donne le N° de périphérique correspondant.
    Il renvoie une commande 'set_______' avec les params du périphérique tel qu'ils sont dans la table :
    N° de périphérique et adresse Mac (0 s pas de place), pitch et période pour les mesures de température, 
    nombre de switchs, la durée des pulses éventuels et état des switchs, le nombre de détecteurs, pour chaque switch
    leur mode d'action sur le switch. Plus la date et l'heure.

    Le serveur peut envoyer au périphérique une demande d'état "etat______", la réponse est "data_save_" avec les 
    données du périphérique qui sont stockées dans la table. La réponse du serveur est "ack_______" 
    avec date et heure comme donnée.
    Le périphérique qui a un numéro envoie des "data_save_" selon la période programmée.
    
    Un périphérique qui n'a jamais été identifié a le numéro 0 ; si le serveur donne la valeur 0 lors d'un data_read_
    c'est que l'adresse mac du périph ne correspond pas à celle de la table ou n'existe pas. Le périph remet son numéro
    à 0 et refait une procédure d'initialisation (en principe, data_read_).

    En cas d'anomalie de transmission le périphérique met son numléro à 0.
    Au reset, le numéro de périph est 0.
    
    Le symbole "_" sert de séparateur de champ et n'est pas autorisé dans les data.

    format des messages d'échange avec les périphériques :    nom_fonction=nnnn_...donnée...cc
              nom_fonction 10 caractères
              nnnn longueur sur 4 chiffres complétés avec 0 à gauche de la longueur au crc inclus
              cc crc

    La longueur maxi théorique des datas en provenance des périphériques est limitée par la commande GET et à 99999 avec POST
    Pratiquement voir les paramètres NBVAL LENVAL LVAL LBUFSERVER etc...
    
*/
        periInitVar();        // pas de rémanence des données des périphériques entre 2 accès au serveur

        memset(strSD,0,sizeof(strSD)); memset(buf,0,sizeof(buf));charIp(remote_IP,strSD);          // histo :
        sprintf(buf,"%d",nbreparams+1);strcat(strSD," ");strcat(strSD,buf);strcat(strSD," = ");    // une ligne par transaction
        strcat(strSD,strSdEnd);

/*      
    Un formulaire (<form>....</form>) contient des champs de saisie dont le nom et la valeur sont transmis dans l'ordre d'arrivée 
    quand submit incluse dans le formulaire est déclenchée (<input type="submit" value="MàJ">)

    La première fonction du formulaire doit fixer les paramètres communs à toutes les autres du formulaire :
      pour chaque ligne de periTable ou table de switchs, le traitement à effectuer après que toutes les fonctions 
      soient traitées, le numéro de périphérique, le chargement du périphérique, d'éventuels autres traitements communs préalables.
    premières fonctions de formulaires : pericur (lignes de péritable), peri_t_sw_)
    
    what indique le traitement à effectuer (voir switch(what) pour les détails)
*/
        what=0;                           // pas de traitement subsidiaire

/*
      3 modes de fonctionnement :

      1 - accueil : saisie de mot de passe
      2 - serveur pour périphériques : seules fonctions dataRead et dataSave ; contiennent l'adr mac pour identifier le périphérique.
      3 - serveur pour navigateur : dans les message GET ou POST, 
          pour assurer la validité des données du périphérique concerné, periCur doit être positioné et periLoad() effectué 
          par une fonction qui précède les fonctions modifiant des variables du formulaire concerné (peri_cur__, peri_t_sw_) 

    si la première fonction est fperipass, c'est un périphérique, sinon un browser 
    si c'est un browser : 
      (première fonction==(fpassword ou fmacmaster)      ... page d'accueil ou peritable et saisie macmaster
      ou (macMaster==remote_MAC et pas TO))              ... macmaster et TO ok
      sinon accueil
    
    fonctionnement de la rémanence de password_ :
    
    toPassword=0 annule la rémanence de password_                        - disTrigPwd()
    toPassword=TO_PASSWORD établit la rémanence
    ctlto(passwordTime,toPassword) retourne vrai si le temps est dépassé - chkTrigPad()
    startto(passwordTime,toPassword,TO_PASSWORD) retrig la rémanence     - trigPwd()
     
    lorsque macMaster!=remote_MAC la rémanence n'est pas armée par password_ car password_ ne sert que pour afficher peritable ou valider une fonction à suivre (future use)
    lorsque macMaster==remote_MAC password_ est une action qui retrigger
*/     

      if(numfonct[0]!=fperipass){
        if(numfonct[0]!=fpassword && numfonct[0]!=fmacmaster){
          if(memcmp(macMaster,remote_MAC,6)!=0 || chkTrigPwd()){
            what=-1;nbreparams=-1;i=0;numfonct[i]=faccueil;memset(macMaster,0x00,6);disTrigPwd();
          }
        }
      }

/*
    boucle des fonctions accumulées par getnv
*/   
        for (i=0;i<=nbreparams;i++){

          if(i<NBVAL && i>=0){
          
            valf=valeurs+nvalf[i];    // valf pointe la ième chaine à traiter dans valeurs[] (terminée par '\0')
                                      // nvalf longueur dans valeurs
                                      // si c'est la dernière chaîne, strlen(valf) est sa longueur 
                                      // c'est obligatoirement le cas pour data_read_ et data_save_ qui terminent le message
            
/*            
    controle de dépassement de capacité du buffer strSD ; si ok, ajout de la fonction, sinon ajout de '*'  
*/
            if((strlen(strSD)+strlen(valf)+5+strlen(strSdEnd))<RECCHAR){
              strSD[strlen(strSD)-strlen(strSdEnd)]='\0';sprintf(buf,"%d",numfonct[i]);strcat(strSD,buf);
              strcat(strSD," ");strcat(strSD,valf);strcat(strSD,";");strcat(strSD,strSdEnd);}
            else {strSD[strlen(strSD)-strlen(strSdEnd)]='*';}


            switch (numfonct[i])      
              {
              case 0:  pertemp=0;conv_atobl(valf,&pertemp);break;
              case 1:  if((memcmp(macMaster,remote_MAC,6)==0 && memcmp(remote_IP_Mac,remote_IP,4)==0 && !chkTrigPwd()) || ctlpass(valf,modpass)){
                         trigPwd();memcpy(macMaster,remote_MAC,6);memcpy(remote_IP_Mac,remote_IP,4);}       // macmaster_
                       else {disTrigPwd();memset(macMaster,0x00,6);memset(remote_IP_Mac,0x00,4);what=-1;nbreparams=0;}
                       memDetServ=0;                                    // raz checkbit det serv effectué par la première fonction de l'en-tête
                       break;
              case 2:  if(checkData(valf)==MESSOK){                                                         // peri_pass_
                         periPassOk=ctlpass(valf+5,peripass);                                               // skip len
                         if(periPassOk==FAUX){memset(remote_IP_cur,0x00,4);sdstore_textdh(&fhisto,"pp","ko",strSD);}
                         else {memcpy(remote_IP_cur,remote_IP,4);}
                       }break;
              case 3:  if(!ctlpass(valf,usrpass)){                                                          // password_
                         disTrigPwd;memset(macMaster,0x00,6);memset(remote_IP_Mac,0x00,4);what=-1;nbreparams=0;
                         sdstore_textdh(&fhisto,"pw","ko",strSD);break;}                                    // si faux accueil (what=-1)
                       else if(nbreparams==0){                                                              // si vrai et pas de params...
                         if(memcmp(macMaster,remote_MAC,6)==0){trigPwd();}                                     // ... si macMaster ok -> retrig
                         periTableHtml(&cli_a);what=0;}                                                     // ... peritable
                       break;                                                                               // sinon param suivant (future use)
              case 4:  testHtml(&cli_a);break;                                                       // testhtml__
              case 5:  break;                                                                        // done
              case 6:  what=2;perrefr=0;conv_atob(valf,&perrefr);                                           // periode refresh browser
                       break;                                                                               
              case 7:  *periThOffset=0;*periThOffset=convStrToNum(valf,&j);break;                    // Th Offset
              case 8:  periCur=*(libfonctions+2*i+1)-PMFNCHAR;                                       // switchs___ 
                       periInitVar();periLoad(periCur);
                       SwCtlTableHtml(&cli_a,*periSwNb,4);
                       break;                                                                               
              case 9:  autoreset=VRAI;break;                                                         // reset
              case 10: dumpsd(&cli_a);break;                                                         // dump_sd
              case 11: what=2;sdpos=0;conv_atobl(valf,&sdpos);break;                                        // SD pos
              case 12: if(periPassOk==VRAI){what=1;periDataRead();periPassOk==FAUX;}break;           // data_save
              case 13: if(periPassOk==VRAI){what=3;periDataRead();periPassOk==FAUX;}break;           // data_read
              case 14: what=5;{                                                                             // peri swb (buton)
                       uint8_t sw;sw=*(libfonctions+2*i)-PMFNCHAR;                                          // sw n° switch
                       uint8_t sh;sh=libfonctions[2*i+1]-PMFNCHAR;                                          // sh 0 ou 1 état demandé
                       byte mask=(0x10<<sw*2);
                       *periSwVal&=~mask;*periSwVal|=mask;
                       }break;
              case 15: what=4;periCur=0;conv_atob(valf,&periCur);                                           // peri cur  N° périph courant (1ère fonction ligne table)
                       if(periCur>NBPERIF){periCur=NBPERIF;}                                                // fixe periCur et charge la ligne
                       periInitVar();periLoad(periCur);
                       if(periProg!=0){what=5;}                                                             // si périphérique serveur -> perisend                           
                       break;                                                                               // sinon perisave seul
              case 16: *periPerRefr=0;conv_atobl(valf,periPerRefr);break;                            // per refr periph courant
              case 17: memset(periNamer,0x00,PERINAMLEN-1);                                          // nom periph courant
                       memcpy(periNamer,valf,nvalf[i+1]-nvalf[i]);
                       break;                                                                              
              case 18: for(j=0;j<6;j++){conv_atoh(valf+j*2,(periMacr+j));}break;                     // Mac periph courant
              case 19: accueilHtml(&cli_a);break;                                                    // accueil
              case 20: periTableHtml(&cli_a);break;                                                  // peri table
              case 21: *periProg=*valf-48;break;                                                     // peri prog
              case 22: *periSondeNb=*valf-48;if(*periSondeNb>MAXSDE){*periSondeNb=MAXSDE;}break;     // peri sonde
              case 23: *periPitch=0;*periPitch=convStrToNum(valf,&j);break;                          // peri pitch
              case 24: {                                                                             // peri SwPulseCtl (pmo) bits détecteurs
                       uint8_t sw;sw=*(libfonctions+2*i)-PMFNCHAR;                                   // sw n° switch
                       uint8_t sh;sh=libfonctions[2*i+1]-PMFNCHAR;if(sh>32){sh-=6;}                  // '-6' pour skip ponctuations (valeur maxi utilisable 2*26=52)
                       uint64_t pipm;memcpy(&pipm,(char*)(periSwPulseCtl+sw),DLSWLEN);               // sh=[LENNOM-1]-PMFNCHAR nb de shifts dans le détecteur
                       byte msk;msk=0x01;if((sw&0xf0)!=0){msk=mask[DLNULEN];sw&=0x0f;}               // si !=0 -> n° detecteurs ou code action sinon cb                               
                       bitvSwCtl(periSwPulseCtl,sw,DLSWLEN,sh,msk);            // shift référencé au début du switch
                       }break;
              case 25: *periDetNb=*valf-48;if(*periDetNb>MAXDET){*periDetNb=MAXDET;}break;           // peri det Nb  
              case 26: *periSwNb=*valf-48;if(*periSwNb>MAXSW){*periSwNb=MAXSW;}break;                // peri sw Nb                       
              case 27: *periSwVal&=0xfd;*periSwVal|=(*valf&0x01)<<1;break;                           // peri Sw Val 0
              case 28: *periSwVal&=0xf7;*periSwVal|=(*valf&0x01)<<3;break;                           // peri Sw Val 1
              case 29: *periSwVal&=0xdf;*periSwVal|=(*valf&0x01)<<5;break;                           // peri Sw Val 2
              case 30: *periSwVal&=0x7f;*periSwVal|=(*valf&0x01)<<7;break;                           // peri Sw Val 3
              case 31: periCur=0;conv_atob(valf,&periCur);                                           // peri_t_sw_
                       if(periCur>NBPERIF){periCur=NBPERIF;}
                       periInitVar();periLoad(periCur);cbErase();                                    // effacement checkboxs des switchs du periphérique courant
                       break;  
              case 32: {uint8_t sw=*(libfonctions+2*i)-PMFNCHAR,b=*(libfonctions+2*i+1);             // peri SwPulseCtl (otf) bits généraux (FOT)
                        uint8_t sh=0;
                        switch (b){
                           case 'F':sh=PMFRO_PB;break;
                           case 'O':sh=PMTOE_PB;break;
                           case 'T':sh=PMTTE_PB;break;
                           default:break;
                        }
                        bitvSwCtl(periSwPulseCtl,sw,DLSWLEN,sh,0x01);   // shift référencé au début du switch
                       }break;       
              case 33: {uint8_t v=0,b=0;frecupptr(libfonctions+2*i,&v,&b,MAXTAC);                    // periSwMode N° detec (imn)
                        *(periSwMode+v) &= ~(mask[SWMDLNUMS_PB-SWMDLNULS_PB+1]<<SWMDLNULS_PB);
                        *(periSwMode+v) |= (*valf&mask[SWMDLNUMS_PB-SWMDLNULS_PB+1])<<SWMDLNULS_PB;
                       }break;                                                                      
              case 34: {uint8_t v=0,b=0;frecupptr(libfonctions+2*i,&v,&b,MAXTAC);                    // periSwMode 6 checkbox (imc)
                        *(periSwMode+v) &= maskbit[b*2];if(*valf&0x01!=0){*(periSwMode+v) |= maskbit[b*2+1];} 
                       }break;                                                                      
              case 35: {int sw=*(libfonctions+2*i)-PMFNCHAR;                                         // peri Pulse one (pto)
                        *(periSwPulseOne+sw)=0;*(periSwPulseOne+sw)=(uint32_t)convStrToNum(valf,&j);                              
                       }break;                                                                      
              case 36: {int sw=*(libfonctions+2*i)-PMFNCHAR;
                        *(periSwPulseTwo+sw)=0;*(periSwPulseTwo+sw)=(uint32_t)convStrToNum(valf,&j); 
                       }break;                                                                       // peri Pulse two (ptt)
              case 37: *periThmin=0;*periThmin=convStrToNum(valf,&j);break;                          // Th min
              case 38: *periThmax=0;*periThmax=convStrToNum(valf,&j);break;                          // Th max
              case 39: *periVmin=0;*periVmin=(float)convStrToNum(valf,&j);break;                     // V min
              case 40: *periVmax=0;*periVmax=convStrToNum(valf,&j);break;                            // V max
              case 41: *periDetServEn |= (byte)(*valf-48)<<(*(libfonctions+2*i+1)-PMFNCHAR);break;   // Det Serv enable bits
              case 42: memDetServ |= (byte)(*valf-48)<<(*(libfonctions+2*i+1)-PMFNCHAR);break;       // Det Serv level bits
              case 43: {int nb=*(libfonctions+2*i+1)-PMFNCHAR;                                       // ssid[libf+1]
                       memset(ssid+nb*(LENSSID+1),0x00,LENSSID+1);memcpy(ssid+nb*(LENSSID+1),valf,nvalf[i+1]-nvalf[i]);
                       }break;
              case 44: {int nb=*(libfonctions+2*i+1)-PMFNCHAR;                                       // passssid[libf+1]
                       memset(passssid+nb*(LPWSSID+1),0x00,LENSSID+1);memcpy(passssid+nb*(LPWSSID+1),valf,nvalf[i+1]-nvalf[i]);
                       }break;
              case 45: cfgServerHtml(&cli_a);configPrint();break;break;                              // config
              case 46: memset(usrpass,0x00,LPWD);memcpy(usrpass,valf,nvalf[i+1]-nvalf[i]);configSave();break;     // passwordcfg
              case 47: memset(modpass,0x00,LPWD);memcpy(modpass,valf,nvalf[i+1]-nvalf[i]);configSave();break;     // modpasscfg
              case 48: memset(peripass,0x00,LPWD);memcpy(peripass,valf,nvalf[i+1]-nvalf[i]);configSave();break;   // peripasscfg
              case 49: for(j=0;j<6;j++){conv_atoh(valf+j*2,(mac+j));}configSave();break;                          // Mac config
                              
              default:break;
              }
              
            }     // i<NBVAL   
          }       // fin boucle nbre params
          
          if(nbreparams>=0){
            Serial.print("what=");Serial.print(what);Serial.print(" periCur=");Serial.print(periCur);Serial.print(" strSD=");Serial.print(strSD);
            sdstore_textdh(&fhisto,"ip","CX",strSD);  // utilise getdate() et place date et heure dans l'enregistrement
          }                                           // 1 ligne par commande GET

        //configPrint();
        periCheck(8,"what");

/*
   periParamsHtml (fait perisave) effectue une réponse ack ou set ou envoie une commande get /set si applelé par perisend
*/
       
        switch(what){                                           
          case 0:break;                                         // fonctions ponctuelles du serveur
          case 1:periParamsHtml(&cli_a," ",0);break;            // data_save
          case 2:periTableHtml(&cli_a);break;                   // saisies de l'en-tête de peritable
          case 3:periParamsHtml(&cli_a," ",0);break;            // data_read
          case 4:periSave(periCur);periTableHtml(&cli_a);break; // periphériques non serveurs pas de commande get /set ...
          case 5:periSend();periTableHtml(&cli_a);break;        // périphériques serveurs            commande get /set ... (fait periParamsHtml)
          case 6:break;                                         // dispo        
          case 7:periSend();SwCtlTableHtml(&cli_a,*periSwNb,4);break; // config switchs - smise à jour des périphériques (fait periParamsHtml)
          default:accueilHtml(&cli_a);break;
          }

          valeurs[0]='\0';
          cli_a.stop();
          delay(1);
          Serial.println("---- cli stopped"); 
      }   // cli.connected
    }     // server.available

// *** blink

      ledblink(0);   
          
// *** maj température  

      if((millis()-curtemp)>pertemp*1000){
        curtemp=millis();
        char buf[]={0,0,'.',0,0,0};
        readDS3231temp(&msb,&lsb);
        sprintf(buf,"%02u",msb);sprintf(buf+3,"%02u",lsb);
        sdstore_textdh(&fhisto,"T=",buf,"<br>\n");
      }
        
} // loop


// ========================================= tools ====================================

void checkdate(uint8_t num)
{
  if(periLastDateIn[0]==0x66){Serial.print("===>>> date ");Serial.print(num);Serial.print(" HS ");
    char dateascii[12];
    unpackDate(dateascii,periLastDateIn);for(j=0;j<12;j++){Serial.print(dateascii[j]);if(j==5){Serial.print(" ");}}Serial.println();
  }
}

void periDataRead()             // traitement d'une chaine "dataSave" ou "dataRead" en provenance d'un periphérique 
                                // periInitVar() a été effectué
                                // controle len,CRC, charge periCur (N° périf du message), effectue periLoad()
                                // gère les différentes situations présence/absence/création de l'entrée dans la table des périf
                                // transfère adr mac du message reçu (periMacBuf) vers periRec (periMacr)                                
                                // retour periMess=MESSOK -> periCur valide, periLoad effectué, tfr données dataRead effectué
                                //        periMess=MESSFULL -> plus de place libre
                                //        periMess autres valeurs retour de checkData
{
  int i=0;
  char* k;
  uint8_t c=0;
  int ii=0;
  int perizer=0;

  periCur=0;
                        // check len,crc
  periMess=checkData(valf);if(periMess!=MESSOK){periInitVar();return;}         
  
                        // len,crc OK
  valf+=5;conv_atob(valf,&periCur);packMac(periMacBuf,valf+3);                       
  
  if(periCur!=0){                                                 // si le périph a un numéro, ctle de l'adr mac
    periLoad(periCur);
    if(memcmp(periMacBuf,periMacr,6)!=0){periCur=0;}}
    
  if(periCur==0){                                                 // si periCur=0 recherche si mac connu   
    for(i=1;i<=NBPERIF;i++){                                      // et, au cas où, une place libre
      periLoad(i);
      if(compMac(periMacBuf,periMacr)){periCur=i;i=NBPERIF+1;}    // mac trouvé
      if(memcmp("\0\0\0\0\0\0",periMacr,6)==0 && perizer==0){perizer=i;}        // place libre trouvée
    }
  }
    
  if(periCur==0 && perizer!=0){                                   // si pas connu utilisation N° perif libre "perizer"
      Serial.println(" Mac inconnu");
      periInitVar();periCur=perizer;periLoad(periCur);
      periMess=MESSFULL;
  }

  if(periCur!=0){                                                 // si ni trouvé, ni place libre, periCur=0 
    Serial.println(" place libre ou Mac connu");
       
    memcpy(periMacr,periMacBuf,6);
    k=valf+2+1+17+1;*periLastVal=convStrToNum(k,&i);                                    // température si save
    k+=i;convStrToNum(k,&i);                                                            // age si save
    k+=i;*periAlim=convStrToNum(k,&i);                                                  // alim
    k+=i;strncpy(periVers,k,LENVERSION);                                                // version
    k+=strchr(k,'_')-k+1; uint8_t qsw=(uint8_t)(*k-48);                                 // nbre sw
    k+=1; *periSwVal&=0xAA;for(int i=MAXSW-1;i>=0;i--){*periSwVal |= (*(k+i)-48)<< 2*(MAXSW-1-i);}    // periSwVal états sw
    k+=MAXSW+1; *periDetNb=(uint8_t)(*k-48);                                                          // nbre detec
    k+=1; *periDetVal=0;for(int i=MAXDET-1;i>=0;i--){*periDetVal |= ((*(k+i)-48)&DETBITLH_VB )<< 2*(MAXDET-1-i);}    // détecteurs
    k+=MAXDET+1;for(int i=0;i<MAXSW;i++){periSwPulseSta[i]=*(k+i);}                     // pulse clk status 
    k+=MAXSW+1;for(int i=0;i<LENMODEL;i++){periModel[i]=*(k+i);periNamer[i]=*(k+i);}    // model
    k+=LENMODEL+1;
    
    memcpy(periIpAddr,remote_IP_cur,4);            //for(int i=0;i<4;i++){periIpAddr[i]=remote_IP_cur[i];}      // Ip addr
    char date14[14];alphaNow(date14);checkdate(0);packDate(periLastDateIn,date14+2);    // maj dates
Serial.println("periDataRead");periPrint(periCur);
    periSave(periCur);checkdate(6);
  }
}

void periSend()                 // configure periParamsHtml pour envoyer un set_______ à un périph serveur
{
  checkdate(2);
  char ipaddr[16];memset(ipaddr,'\0',16);
  charIp(periIpAddr,ipaddr);
  periParamsHtml(&cliext,ipaddr,PORTSERVPERI);
}

int periParamsHtml(EthernetClient* cli,char* host,int port)   // fonction set ou ack vers périphérique
                    // si port=0 envoie une page html (bufServer encapsulé dans <body>...</body>) (fonction ack suite à réception de datasave - set si dataread)
                    // sinon envoie cde GET dans bufServer via messToServer + getHttpResp         (fonction set suite à modif dans periTable)
                    //                            status retour de messToServer ou getHttpResponse ou fonct invalide (doit être done___)
                    // periCur est à jour (0 ou n) et periMess contient le diag du dataread/save reçu
                    // si periCur = 0 message set réduit
                    // format nomfonction_=nnnndatas...cc                nnnn len mess ; cc crc
                    // datas NN_mm.mm.mm.mm.mm.mm_AAMMJJHHMMSS_nn..._    NN numpériph ; mm.mm... mac
{                   
  char message[LENMESS]={'\0'};
  char nomfonct[LENNOM+1]={'\0'};
  int8_t zz=MESSOK;
  char date14[15];alphaNow(date14);
  
  if((periCur!=0) && (what==1) && (port==0)){strcpy(nomfonct,"ack_______");}    // ack pour datasave (what=1)
  else strcpy(nomfonct,"set_______");
  checkdate(3);
  assySet(message,periCur,periDiag(periMess),date14);
  checkdate(4);
  *bufServer='\0';

          if(port!=0){memcpy(bufServer,"GET /\0",6);}   // message pour commande directe de périphérique en mode serveur
          buildMess(nomfonct,message,"");               // bufServer complété   

          if(port==0){                                  // réponse à dataRead/Save
            htmlIntro0(cli);
            cli->print("<body>");
            cli->print(bufServer);
            cli->println("</body></html>");
          }
          else {                                        // envoi vers périphérique en mode serveur
            zz=messToServer(cli,host,port,bufServer);
            if(zz==MESSOK){
              uint8_t fonct;
              zz=getHttpResponse(&cliext,bufServer,LBUFSERVER,&fonct);
              if(zz==MESSOK && fonct!=fdone){zz=MESSFON;}
            }
            purgeServer(&cliext);
            cli->stop();
            delay(1);
          }
          checkdate(5);
          if(zz==MESSOK){packDate(periLastDateOut,date14+2);}
          *periErr=zz;
          periSave(periCur);                              // modifs de periTable et date effacèe par prochain periLoad si pas save

          return zz;
}

int getcde(EthernetClient* cli) // décodage commande reçue selon tables 'cdes' longueur maxi LENCDEHTTP
{
  char c='\0',cde[LENCDEHTTP];
  int k=0,ncde=0,ko=0;
  
  while (cli->available() && c!='/' && k<LENCDEHTTP) {
    c=cli->read();Serial.print(c);     // décode la commande 
    if(c!='/'){cde[k]=c;k++;}
    else {cde[k]=0;break;}
    }
  if (c!='/'){ko=1;}                                                                                            // pas de commande, message 400 Bad Request
  else if (strstr(cdes,cde)==0){ko=2;}                                                                          // commande inconnue 501 Not Implemented
  else {ncde=1+(strstr(cdes,cde)-cdes)/LENCDEHTTP ;}                                                            // numéro de commande (ok si >0 && < nbre cdes)
  if ((ncde<=0) || (ncde>strlen(cdes)/LENCDEHTTP)){ko=1;ncde=0;while (cli->available()){cli->read();}}          // pas de cde valide -> vidage + message
  switch(ko){
    case 1:cli->print("<body><br><br> err. 400 Bad Request <br><br></body></html>");break;
    case 2:cli->print("<body><br><br> err. 501 Not Implemented <br><br></body></html>");break;
    default:break;
  }
  return ncde;                                // ncde=0 si KO ; 1 à n numéro de commande
}

int analyse(EthernetClient* cli)              // decode la chaine et remplit les tableaux noms/valeurs 
{                                             // prochain car = premier du premier nom
                                              // les caractères de ctle du flux sont encodés %HH par le navigateur
                                              // '%' encodé '%25' ; '@' '%40' etc... 

  boolean nom=VRAI,val=FAUX,termine=FAUX;
  int i=0,j=0,k=0;
  char c,cpc='\0';                         // cpc pour convertir les séquences %hh 
  char noms[LENNOM+1]={0},nomsc[LENNOM-1];noms[LENNOM]='\0';nomsc[LENNOM-2]='\0';
  memset(libfonctions,0x00,sizeof(libfonctions));

      nvalf[0]=0;
      memset(valeurs,0,LENVALEURS+1);                   // effacement critique (password etc...)
      memset(noms,' ',LENNOM);
      numfonct[0]=-1;                                   // aucune fonction trouvée
     
      while (cli->available()){
        c=cli->read();

        if(c=='%'){cpc=c;}                              // %
        else {
        
          if(cpc=='%'){cpc=(c&0x0f)<<4;}                // % reçu ; traitement 1er car
          else {
        
            if(cpc!='\0'){c=cpc|(c&0x0f);cpc='\0';}     // traitement second                 
            Serial.print(c);
            if (!termine){
          
              if (nom==FAUX && (c=='?' || c=='&')){nom=VRAI;val=FAUX;j=0;memset(noms,' ',LENNOM);if(i<NBVAL){i++;};Serial.println(libfonctions+2*(i-1));}  // fonction suivante ; i indice fonction courante ; numfonct[i] N° fonction trouvée
              if (nom==VRAI && (c==':' || c=='=')){                                    
                nom=FAUX;val=VRAI;
                nvalf[i+1]=nvalf[i]+1;
                if(i==0){nvalf[1]=0;}                                       // permet de stocker le tout premier car dans valeurs[0]
                else {nvalf[i]++;}                                          // skip l'intervalle entre 2 valeurs           
                numfonct[i]=(strstr(fonctions,noms)-fonctions)/LENNOM;      // acquisition nom terminée récup N° fonction
                memcpy(libfonctions+2*i,noms+LENNOM-2,2);                   // les 2 derniers car du nom de fonction si nom court
                if(numfonct[i]<0 || numfonct[i]>=nbfonct){
                  memcpy(nomsc,noms,LENNOM-2);
                  numfonct[i]=(strstr(fonctions,nomsc)-fonctions)/LENNOM;   // si nom long pas trouvé, recherche nom court (complété par nn)
                  if(numfonct[i]<0 || numfonct[i]>=nbfonct){numfonct[i]=faccueil;}
                }
              }
              if (nom==VRAI && c!='?' && c!=':' && c!='&' && c>' '){noms[j]=c;if(j<LENNOM-1){j++;}}              // acquisition nom
              if (val==VRAI && c!='&' && c!=':' && c!='=' && c>' '){
                valeurs[nvalf[i+1]]=c;if(nvalf[i+1]<=LENVALEURS-1){nvalf[i+1]++;}}                               // contôle decap !
              if (val==VRAI && (c=='&' || c<=' ')){
                nom=VRAI;val=FAUX;j=0;Serial.println();
                if(c==' ' || c<=' '){termine=VRAI;}}                                                             // ' ' interdit dans valeur : indique fin données                                 
            }
          }                                                                                             // acquisition valeur terminée (données aussi si c=' ')
        }
      }
      Serial.print("\n---- fin getnv i=");Serial.println(i);
      if(numfonct[0]<0){return -1;} else return i;
}

int getnv(EthernetClient* cli)        // décode commande, chaine et remplit les tableaux noms/valeurs
{                                     // sortie -1 pas de commande ; 0 pas de nom/valeur ; >0 nbre de noms/valeurs                                
  numfonct[0]=-1;
  int cr=0,pbli=0;
#define LBUFLI 12
  char bufli[LBUFLI];
  
  Serial.println("--- getnv");
  
  int ncde=getcde(cli); 
      Serial.print("ncde=");Serial.print(ncde);Serial.println(" ");
      if(ncde==0){return -1;}  
      
      c=' ';
      while (cli->available() && c!='?'){      // attente '?'
        c=cli->read();Serial.print(c);
        bufli[pbli]=c;if(pbli<LBUFLI-1){pbli++;bufli[pbli]='\0';}
      }Serial.println();          

        switch(ncde){
          case 1:           // GET
            if(bufli[0]=='?' || strstr(bufli,"page.html?")>0 || strstr(bufli,"cx?")>0){return analyse(cli);}
            break;    
          case 2:           // POST
            if(c=='\n' && cr>=3){return analyse(cli);}
            else if(c=='\n' || c=='\r'){cr++;}
            else {cr=0;}
            break;
          default:break;
        }
      if(numfonct[0]<0){return -1;} else return 0; 
}

void packVal2(byte* value,byte* val)      // insertion dans *value des bits 0 et 1 de *val 
                                          // dans la position de int(*val/4) 
{                                         // *val retourné forme 0x03
  byte a=(byte)*val&0xfe;
  if(*val&0x01==0){*val=0;}
  else *val&=0xfe;
  a=0xff^a;*value&=a;*value|=*val;
}

void unpackVal2(byte* value,char* dest)   // remplit 4 car de dest avec les valeurs des 4 paires de bits de *value
{
  for(int q=0;q<4;q++){byte x=*value;dest[3-q]=((x >> 2*q) & 0x03) + 48;}
}


#ifdef _AVEC_AES
void xcrypt()
{
    AES_init_ctx_iv(&ctx, key, iv);
    AES_CTR_xcrypt_buffer(&ctx, chaine, 16);
}
#endif // _AVEC_AES


void conv_atob(char* ascii,uint16_t* bin)
{
  int j=0;
  *bin=0;
  for(j=0;j<LENVAL;j++){c=ascii[j];if(c>='0' && c<='9'){*bin=*bin*10+c-48;}else{j=LENVAL;}}
}

void conv_atobl(char* ascii,uint32_t* bin)
{
  int j=0;
  *bin=0;
  for(j=0;j<LENVAL;j++){c=ascii[j];if(c>='0' && c<='9'){*bin=*bin*10+c-48;}else{j=LENVAL;}}
}

void frecupptr(char* libfonct,uint8_t* v,uint8_t* b,uint8_t lenpersw)
{  
   *v=((*libfonct-48)*lenpersw+(*(libfonct+1)-MAXSW*16)/16)&0x0f;   // num octet de type d'action
   *b=(*(libfonct+1)-MAXSW*16)%16;                                  // N° de bit dans l'octet de type d'action
}

void bitvSwCtl(byte* data,uint8_t sw,uint8_t datalen,uint8_t shift,byte msk)   
// place le(s) bit(s) de valf position shift, masque msk, dans data de rang sw longeur datalen 
{
  //Serial.print("sw=");Serial.print(sw);Serial.print(" datalen=");Serial.print(datalen);Serial.print(" shift=");Serial.print(shift);Serial.print(" msk=");Serial.print(msk,HEX);Serial.print(" *valf=");Serial.println(*valf-PMFNCVAL);
  //dumpstr(data,datalen);memset(data,0x00,datalen);
  uint64_t pipm=0;memcpy(&pipm,(char*)(data+sw*datalen),datalen);
  uint64_t c=((uint64_t)msk)<<shift;
  pipm &= ~c;
  c=0;c=((uint64_t)(*valf-PMFNCVAL))<<shift;
  pipm +=c;
//  Serial.print(" shift=");Serial.print(shift,HEX);Serial.print(" msk=");Serial.print(msk,HEX);Serial.print(" c=");dumpstr((char*)&c,8);
  memcpy((char*)(data+sw*datalen),(char*)&pipm,datalen);
}

void test2Switchs()
{

  char ipAddr[16];memset(ipAddr,'\0',16);
  charIp(lastIpAddr,ipAddr);
  for(int x=0;x<4;x++){
//Serial.print(x);Serial.print(" test2sw ");Serial.println(ipAddr);
    testSwitch("GET /testb_on__=0006AB8B",ipAddr,PORTSERVPERI);
    delay(2000);
    testSwitch("GET /testa_on__=0006AB8B",ipAddr,PORTSERVPERI);
    delay(2000);
    testSwitch("GET /testboff__=0006AB8B",ipAddr,PORTSERVPERI);
    delay(2000);
    testSwitch("GET /testaoff__=0006AB8B",ipAddr,PORTSERVPERI);
    delay(2000);
  }
}

void testSwitch(char* command,char* perihost,int periport)
{
            uint8_t fonct;
            
            memset(bufServer,'\0',32);
            memcpy(bufServer,command,24);

            int z=messToServer(&cliext,perihost,periport,bufServer);
            Serial.println(z);
            if(z==MESSOK){
              periMess=getHttpResponse(&cliext,bufServer,LBUFSERVER,&fonct);
              Serial.println(periMess);
            }
            purgeServer(&cliext);
            cliext.stop();
            delay(1);
}


