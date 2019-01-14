
#define _MODE_DEVT    // change l'adresse Mac de la carte IP, l'adresse IP et le port

#include <SPI.h>      //bibliothèqe SPI pour W5100
#include <Ethernet.h> //bibliothèque W5100 Ethernet
#include <Wire.h>     //biblio I2C pour RTC 3231
#include <shconst.h>
#include <shutil.h>
#include <shmess.h>
#include "const.h"
#include "utilether.h"
#include "periph.h"
#include "httphtml.h"
//#include "htmlwk.h"


#ifdef WEMOS
  #define Serial SerialUSB
#endif def WEMOS
#ifndef WEMOS
  #include <avr/wdt.h>  //biblio watchdog
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
//  EthernetClient cli_b;             // client du serveur browser local  
#define TO_SERV  600                // sec
  EthernetClient cliext;            // client de serveur externe  


#ifdef _MODE_DEVT
  byte mac[] = {0x90, 0xA2, 0xDA, 0x0F, 0xDF, 0xAC}; //adresse mac carte ethernet AB service ; AC devt
  byte localIp[] = {192, 168, 0, 35}; //adresse IP    ---- 34 service, 35 devt
  EthernetServer periserv(1790);      // port 1789 service, 1790 devt
#endif _MODE_DEVT

#ifndef _MODE_DEVT
  byte* mac = {0x90, 0xA2, 0xDA, 0x0F, 0xDF, 0xAB}; //adresse mac carte ethernet AB service ; AC devt
  byte localIp[] = {192, 168, 0, 34}; //adresse IP    ---- 34 service, 35 devt
  EthernetServer periserv(1789);      // port 1789 service, 1790 devt
#endif _MODE_DEVT

  uint8_t remote_IP[4]={0,0,0,0},remote_IP_cur[4]={0,0,0,0};
  byte    remote_MAC[6]={0,0,0,0,0,0};
  long    to_serv=0;                      // to du serveur pour reset password
  char    pass[]=SRVPASS;           
  bool    passok=FAUX;

/*  EthernetServer monserveur(1792);        // port pour browser
  uint8_t remote_IP_b[4]={0,0,0,0},remote_IP_b_cur[4]={0,0,0,0};
  long    to_serv_b=0;                    // to du serveur pour reset password
  char    pass_b[]=SRVPASS;           
  bool    passok_b=FAUX;
*/
  byte    macMaster[6]={0,0,0,0,0,0};

  int8_t  numfonct[NBVAL];             // les fonctions trouvées 
  char*   fonctions="per_temp__dispomacmaster_peri_pass_dump_sd___test2sw___done______per_refr__peri_tofs_per_date__reset_____password__sd_pos____data_save_data_read_peri_dispoperi_cur__peri_refr_peri_nom__peri_mac__accueil___peri_tableperi_prog_peri_sondeperi_pitchperi_pmo__peri_detnbperi_intnbperi_intv0peri_intv1peri_intv2peri_intv3peri_dispoperi_otf__peri_imn__peri_imc__peri_pto__peri_ptt__last_fonc_";  //};
                                       // nombre fonctions, valeur pour accueil, data_save_ fonctions multiples etc
  int     nbfonct=0,faccueil=0,fdatasave=0,fperiSwVal=0,fperiDetSs=0,fdone=0,fpericur=0,fperipass=0,fpassword=0;
  char    valeurs[LENVALEURS];         // les valeurs associées à chaque fonction trouvée
  uint16_t nvalf[NBVAL];               // offset dans valeurs[] des valeurs trouvées (séparées par '\0')
  char*   valf;                        // pointeur dans valeurs en cours de décodage
  char    libfonctions[NBVAL*2];        // les 2 chiffres des noms de fonctions courts
  int     nbreparams=0;                // 
  int     what=0;                      // ce qui doit être fait après traitement des fonctions (0=rien)

#define LENCDEHTTP 6
  char*   cdes="GET   POST  \0";       // commandes traitées par le serveur
  char    strSD[RECCHAR]={0};          // buffer enregistrements sur SD
  char*   strSdEnd="<br>\r\n\0";
  char    buf[6];

  char bufServer[LBUFSERVER];          // buffer entrée/sortie dataread/save

  byte  msb=0,lsb=0;                   // pour temp DS3231
  float tempf;
  long  curtemp=0;                     // mémo last millis() pour temp
#define PTEMP 3600                     // sec
  long  pertemp=PTEMP;                 // période ech temp       
  int   perrefr=0;                     // periode rafraichissement de l'affichage
#define PDATE 60*12                    // min
  long  perdate=PDATE;                 // période rafr de la date

  
  int   stime=0;int mtime=0;int htime=0;
  long  curdate=0;

/*  enregistrement de table des périphériques ; un fichier par entrée
    (voir periInit() pour l'ordre physique des champs + periSave et periLoad=
*/
  char      periRec[PERIRECLEN];          // 1er buffer de l'enregistrement de périphérique
  
  int16_t   periCur=0;                    // Numéro du périphérique courant
  int16_t*  periNum;                      // ptr ds buffer : Numéro du périphérique courant
  int32_t*  periPerRefr;                  // ptr ds buffer : période datasave minimale
  float*    periPitch;                    // ptr ds buffer : variation minimale de température pour datasave
  float*    periLastVal;                  // ptr ds buffer : dernière valeur de température  
  float*    periAlim;                     // ptr ds buffer : dernière tension d'alimentation
  char*     periLastDateIn;               // ptr ds buffer : date/heure de dernière réception
  char*     periLastDateOut;              // ptr ds buffer : date/heure de dernier envoi  
  char*     periLastDateErr;              // ptr ds buffer : date/heure de derniere anomalie com
  int8_t*   periErr;                      // ptr ds buffer : code diag anomalie com (voir MESSxxx shconst.h)
  char*     periNamer;                    // ptr ds buffer : description périphérique
  char*     periVers;                     // ptr ds buffer : version logiciel du périphérique
  char*     periModel;                    // ptr ds buffer : model du périphérique
  byte*     periMacr;                     // ptr ds buffer : mac address 
  byte*     periIpAddr;                   // ptr ds buffer : Ip address
  byte*     periSwNb;                     // ptr ds buffer : Nbre d'interrupteurs (0 aucun ; maxi 4(MAXSW)            
  byte*     periSwVal;                    // ptr ds buffer : état/cde des inter  
  byte*     periSwMode;                   // ptr ds buffer : Mode fonctionnement inters (4 par switch)           
  uint32_t* periSwPulseOne;               // ptr ds buffer : durée pulses sec ON (0 pas de pulse)
  uint32_t* periSwPulseTwo;               // ptr ds buffer : durée pulses sec OFF(mode astable)
  uint32_t* periSwPulseCurrOne;           // ptr ds buffer : temps courant pulses ON
  uint32_t* periSwPulseCurrTwo;           // ptr ds buffer : temps courant pulses OFF
  byte*     periSwPulseCtl;               // ptr ds buffer : mode pulses
  byte*     periSwPulseSta;               // ptr ds buffer : état clock pulses
  uint8_t*  periSondeNb;                  // ptr ds buffer : nbre sonde
  boolean*  periProg;                     // ptr ds buffer : flag "programmable"
  byte*     periDetNb;                    // ptr ds buffer : Nbre de détecteurs maxi 4 (MAXDET)
  byte*     periDetVal;                   // ptr ds buffer : flag "ON/OFF" si détecteur (2 bits par détec))
  float*    periThOffset;                 // ptr ds buffer : offset correctif sur mesure température
  
  int8_t    periMess;                     // code diag réception message (voir MESSxxx shconst.h)
  byte      periMacBuf[6]; 

  byte*     periBegOfRecord;
  byte*     periEndOfRecord;

  byte      lastIpAddr[4]; 

  // alimentation DS3231
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
 
File fhisto;      // fichier histo sd card
File fhtml;       // fichiers pages html

char cxdx[]="RE";       // param pour l'enregistrement dans SD (CX connexion, 
long sdsiz=0;           // SD current size
long sdpos=0;           // SD current pos. pour dump

boolean autoreset=FAUX;    // VRAI déclenche l'autoreset

int   i=0,j=0;
char  c=' ';
char  b[2]={0,0};
char* chexa="0123456789ABCDEFabcdef\0";
byte  maskbit[]={0xfe,0x01,0xfd,0x02,0xfb,0x04,0xf7,0x08,0xef,0x10,0xdf,0x20,0xbf,0x40,0x7f,0x80};
byte  mask[]={0x00,0x01,0x03,0x07,0x0F};

/* prototypes */

int  getnv(EthernetClient* cli);

void init_params();
int  sdOpen(char mode,File* fileSlot,char* fname);
void conv_atob(char* ascii,int16_t* bin);
void conv_atobl(char* ascii,int32_t* bin);

byte decToBcd(byte val);
byte bcdToDec(byte val);
void xcrypt();
//void serialPrintSave();
void periSend();
int  periParamsHtml(EthernetClient* cli,char* host,int port);
void periDataRead();
void packVal2(byte* value,byte* val);
void frecupptr(char* nomfonct,uint8_t* v,uint8_t* b,uint8_t lenpersw);
void bitvSwCtl(char* data,uint8_t sw,uint8_t datalen,uint8_t shift,byte msk);
void test2Switchs();


void setup() {                              // ====================================


  Serial.begin (115200);delay(1000);
  Serial.print("+");

  pinMode(PINLED,OUTPUT);
  extern uint8_t cntBlink;
  cntBlink=0;

  digitalWrite(PINGNDDS,LOW);
  pinMode(PINGNDDS,OUTPUT);  
  digitalWrite(PINVCCDS,HIGH);
  pinMode(PINVCCDS,OUTPUT);  

  nbfonct=(strstr(fonctions,"last_fonc_")-fonctions)/LENNOM;
  faccueil=(strstr(fonctions,"accueil___")-fonctions)/LENNOM;
  fdatasave=(strstr(fonctions,"data_save_")-fonctions)/LENNOM;
  fperiSwVal=(strstr(fonctions,"peri_intv0")-fonctions)/LENNOM;
  fdone=(strstr(fonctions,"done______")-fonctions)/LENNOM;
  fpericur=(strstr(fonctions,"peri_cur__")-fonctions)/LENNOM;
  fperipass=(strstr(fonctions,"peri_pass_")-fonctions)/LENNOM;
  fpassword=(strstr(fonctions,"password__")-fonctions)/LENNOM;
  
  periInit();
  passok=FAUX;

  long periRecLength=(long)periEndOfRecord-(long)periBegOfRecord+1;
  
  delay(1000);
  Serial.println();Serial.print(VERSION);
  #ifdef _MODE_DEVT
  Serial.print(" MODE_DEVT");
  #endif _MODE_DEVT
  Serial.print("\nPERIRECLEN=");Serial.print(PERIRECLEN);Serial.print("/");Serial.print(periRecLength);Serial.print(" RECCHAR=");Serial.print(RECCHAR);Serial.print(" LBUFSERVER=");Serial.print(LBUFSERVER);Serial.print(" nbfonct=");Serial.print(nbfonct);
  delay(100);if(periRecLength!=PERIRECLEN){ledblink(BCODEPERIRECLEN);}
  
  Serial.print("\nSD card ");
  if(!SD.begin(4)){Serial.println("KO");
    ledblink(BCODESDCARDKO);}
  Serial.println("OK");

/*
for(i=0;i<100;i++){
  Serial.print(i);if(i<10){Serial.print(" ");}Serial.print(" ");
  for(j=0;j<10;j++){Serial.print((char)fonctions[i*10+j]);}
  Serial.println();if((strstr(fonctions+i*10,"last")-(fonctions+i*10))==0){i=100;}}
while(1){} 
*/
/* 
    if(SD.exists("fdhisto.txt")){SD.remove("fdhisto.txt");}
*/
/*//Modification des fichiers de périphériques   
    Serial.println("conversion en cours...");
    periConvert();
    Serial.println("terminé");
    while(1){};
//*/  
/*  création des fichiers de périphériques   
    periInit();
    for(i=1;i<=NBPERIF;i++){
      periRemove(i);
      periCur=i;periSave(i);}
    Serial.print(i-1);Serial.println(" fichiers créés");
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

  Wire.begin();


  if(Ethernet.begin(mac) == 0)
    {//Serial.println("Failed with DHCP");
    // **** la connexion mac seul ne fonctionne pas ****    mais le test doit être fait pour que l'UDP marche ???!!!
    Ethernet.begin (mac, localIp); //initialisation de la communication Ethernet
    }
    
  Serial.print(" MacAddr=");serialPrintMac(mac);
  Serial.print("serveur ");Serial.print(NOMSERV);Serial.print(" sur adresse : "); Serial.print(Ethernet.localIP());Serial.print("/");Serial.println(PORTSERVER);
  
  periserv.begin();   // serveur périphériques
//  monserveur.begin();  // serveur browser
  
  delay(1000);

#ifndef WEMOS  
 // wdt_enable(WDTO_4S);                // watch dog init (4 sec)
#endif ndef WEMOS
  // check date/heure RTC et maj éventuelle par NTP  
  
  i=getUDPdate(&hms,&amj,&js);
  if(!i){Serial.println("pb NTP");ledblink(BCODEPBNTP);} // pas de service date externe 
  else
    {
    Serial.print(js);Serial.print(" ");Serial.print(amj);Serial.print(" ");Serial.println(hms);
    getdate(&hms2,&amj2,&js2);               // read DS3231
    if(amj!=amj2 || hms!=hms2 || js!=js2)
      {
      Serial.print(js2);Serial.print(" ");Serial.print(amj2);Serial.print(" ");Serial.print(hms2);Serial.println(" setup DS3231 ");
     //hms=231700;js=4;amj=20181205;
      setDS3231time((byte)(hms%100),(byte)((hms%10000)/100),(byte)(hms/10000),js,(byte)(amj%100),(byte)((amj%10000)/100),(byte)((amj/10000)-2000)); // SET GMT TIME      
      getdate(&hms2,&amj2,&js2);sdstore_textdh0(&fhisto,"ST","RE"," ");
      }
    }
  
  sdstore_textdh(&fhisto,".3","RE","<br>\n ");

  Serial.println("fin setup");
/*
  lastIpAddr[0]=192;lastIpAddr[1]=168;lastIpAddr[2]=0;lastIpAddr[3]=7;
  while(1){test2Switchs();}
 */
}

/*=================== fin setup ============================ */


void getremote_IP(EthernetClient *client,uint8_t* ptremote_IP,byte* ptremote_MAC)
{
    W5100.readSnDHAR(client->getSocketNumber(), ptremote_MAC);
    W5100.readSnDIPR(client->getSocketNumber(), ptremote_IP);
}



// =========================================================================================================

void loop()                             // =========================================
{

#ifndef WEMOS
    if(!autoreset){wdt_reset();}        // watch dog reset
#endif ndef WEMOS


  
    if(cli_a = periserv.available())    // attente d'un client
      {
        // dévalidation mot de passe si time out ou IP change
        if((to_serv+TO_SERV)<(millis()/1000)){passok=FAUX;}to_serv=millis()/1000;
          getremote_IP(&cli_a,remote_IP,remote_MAC);
          for(i=0;i<4;i++){if(remote_IP[i]!=remote_IP_cur[i]){passok=FAUX;}remote_IP_cur[i]=remote_IP[i];} 
        
      Serial.print("\n**** loop ");serialPrintIp(remote_IP_cur);Serial.print(" new=");serialPrintIp(remote_IP);
      Serial.print("  ");serialPrintMac(macMaster);
      
      if (cli_a.connected()) 
        {nbreparams=getnv(&cli_a);Serial.print("\nnbreparams ");Serial.println(nbreparams);
        
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
      3 modes de fonctionnement :

      1 - accueil : saisie de mot de passe
      2 - serveur pour périphériques : (ip et mot de passe ok) seules fonctions dataRead et dataSave ;
      contiennent l'adr mac pour identifier le périphérique.
      3 - serveur pour navigateur : (ip et mot de passe ok) la première fonction d'un message GET ou POST doit être pericur 
      si des données de périphérique sont modifiées par les fonctions suivantes.
*/
        what=0;
/*      
    what indique le traitement à effectuer lorsque toutes les fonctions POSTées ou GETées ont été traitées
    what=0 rien à faire : commandes ponctuelles du serveur (refresh, reset, password etc)
    what=4 provoque l'enregistrement de la ligne de table via periParamsHtml
    what=5 effectue perisend et l'enregistrement de la ligne de table via periParamsHtml

*/     
   
        for (i=0;i<=nbreparams;i++){

          if(!(passok==VRAI 
            || macMaster[0]!=0
            || numfonct[0]==fperipass 
            || numfonct[0]==faccueil 
            || numfonct[0]==fpassword)){what=-1;nbreparams=-1;i=0;numfonct[i]=faccueil;}

     //      Serial.print("what=");Serial.print(what);Serial.print(" nbreparams=");Serial.print(nbreparams);
    //      Serial.print(" i=");Serial.print(i);Serial.print(" numfonct[i]=");Serial.println(numfonct[i]);
          
          if(i<NBVAL && i>=0){
          
            valf=valeurs+nvalf[i];    // valf pointe la ième chaine à traiter dans valeurs[] (terminée par '\0')
                                      // nvalf longueur dans valeurs
                                      // si c'est la dernière chaîne, strlen(valf) est sa longueur 
                                      // c'est obligatoirement le cas pour data_read_ et data_save_ qui terminent le message

            
//          strSD = en-tete + n fonctions + pied(strSdEnd) ; le pied est écrasé à chaque ajout de fonction
            
            if(strlen(strSD)+strlen(valf)+5+strlen(strSdEnd)<RECCHAR){
              strSD[strlen(strSD)-strlen(strSdEnd)]='\0';sprintf(buf,"%d",numfonct[i]);strcat(strSD,buf);
              strcat(strSD," ");strcat(strSD,valf);strcat(strSD,";");strcat(strSD,strSdEnd);}
            else {strSD[strlen(strSD)-strlen(strSdEnd)]="*";}
            //Serial.print("strSD=");Serial.print(strSD);dumpstr(valf,128);

            switch (numfonct[i])      
              {
              case 0: pertemp=0;conv_atobl(valf,&pertemp);break;
              case 1: if(ctlpass(valf,pass)){ memcpy(macMaster,remote_MAC,6);}
                      break;                                                                                // MacMaster
              case 2: if(checkData(valf)==MESSOK){                                                          // peri_pass_
                        passok=ctlpass(&valeurs[i*LENVAL+5],pass);
                        if(passok==FAUX){sdstore_textdh(&fhisto,"pp","ko",strSD);}
                      }break;
              case 3: dumpsd(&cli_a);break;
              case 4: test2Switchs();break;                                                                 // test2sw
              case 5: break;                                                                                // done
              case 6: perrefr=0;conv_atob(valf,(int16_t*)&perrefr);break; 
              case 7: what=4;*periThOffset=0;*periThOffset=convStrToNum(valf,&j);break;                     // Th Offset
              case 8: perdate=0;conv_atobl(valf,&perdate);break;
              case 9: autoreset=VRAI;break;
              case 10: Serial.print("password=");what=-1;nbreparams=0;                                      // password
                       passok=ctlpass(&valeurs[i*LENVAL],pass);                                             // si saisie fausse -> accueil                          }
                       Serial.println(passok);   
                       if(passok==FAUX){sdstore_textdh(&fhisto,"pw","ko",strSD);}
                       else {periTableHtml(&cli_a);what=0;}
                       break;
              case 11: sdpos=0;conv_atobl(valf,&sdpos);break;                                               // SD pos
              case 12: what=1;periDataRead();break;                                                         // data_save
              case 13: what=3;periDataRead();break;                                                         // data_read
              case 14: break;                                                                               // dispo
              case 15: what=4;periCur=0;conv_atob(valf,&periCur);                                           // N° périph courant  
                       if(periCur>NBPERIF){periCur=NBPERIF;}                                                // géré automatiquement (maj manuelle interdite)
                       periInitVar();periLoad(periCur);cbErase();
                       break;                                                                               // déclenche la mise à jour des variables du périphérique
              case 16: what=4;*periPerRefr=0;conv_atobl(valf,periPerRefr);break;                            // per refr periph courant
              case 17: what=4;{
                       memset(periNamer,' ',PERINAMLEN-1);
                       memcpy(periNamer,valf,PERINAMLEN-1);periNamer[PERINAMLEN-1]='\0';                    // nom periph courant
                       }break;                                                                              // periSwPulseCtl
              case 18: what=4;for(j=0;j<6;j++){conv_atoh(valf+j*2,(periMacr+j));}break;                     // Mac periph courant
              case 19: Serial.println("accueil");what=-1;break;                                                                               // accueil
              case 20: periTableHtml(&cli_a);break;                                                           // peri Table
              case 21: what=4;*periProg=*valf-48;break;                                                     // peri prog
              case 22: what=4;*periSondeNb=*valf-48;if(*periSondeNb>MAXSDE){*periSondeNb=MAXSDE;}break;     // peri sonde
              case 23: what=5;*periPitch=0;*periPitch=convStrToNum(valf,&j);break;                          // peri pitch
              case 24: what=5;{                                                                             // peri SwPulseCtl (pmo) bits détecteurs
                       uint8_t sw;sw=*(libfonctions+2*i)-PMFNCHAR;                                          // sw n° switch
                       uint8_t sh;sh=libfonctions[2*i+1]-PMFNCHAR;if(sh>32){sh-=6;}                         // '-6' pour skip ponctuations (valeur maxi utilisable 2*26=52)
                       uint64_t pipm;memcpy(&pipm,(char*)(periSwPulseCtl+sw),DLSWLEN);                      // sh=[LENNOM-1]-PMFNCHAR nb de shifts dans le détecteur
                       byte msk;msk=0x01;if((sw&0xf0)!=0){msk=mask[DLNULEN];sw&=0x0f;}                      // si !=0 -> n° detecteurs ou code action sinon cb                               
                       bitvSwCtl(periSwPulseCtl,sw,DLSWLEN,sh,msk);                                         // shift référencé au début du switch
                       }break;
              case 25: what=4;*periDetNb=*valf-48;if(*periDetNb>MAXDET){*periDetNb=MAXDET;}break;           // peri det Nb  
              case 26: what=4;*periSwNb=*valf-48;if(*periSwNb>MAXSW){*periSwNb=MAXSW;}break;                // peri sw Nb                       
              case 27: what=5;*periSwVal&=0xfd;*periSwVal|=(*valf&0x01)<<1;break;                           // peri Sw Val
              case 28: what=5;*periSwVal&=0xf7;*periSwVal|=(*valf&0x01)<<3;break;
              case 29: what=5;*periSwVal&=0xdf;*periSwVal|=(*valf&0x01)<<5;break;
              case 30: what=5;*periSwVal&=0x7f;*periSwVal|=(*valf&0x01)<<7;break;
              case 31: what=4;break;                                                                        // *** dispo
              case 32: what=5;{                                                                             // peri SwPulseCtl (otf) bits généraux (FOT)
                                uint8_t sw=*(libfonctions+2*i)-PMFNCHAR,b=*(libfonctions+2*i+1);
                                uint8_t sh=0;
                                switch (b){
                                  case 'F':sh=PMFRO_PB;break;
                                  case 'O':sh=PMTOE_PB;break;
                                  case 'T':sh=PMTTE_PB;break;
                                  default:break;
                                }
                                bitvSwCtl(periSwPulseCtl,sw,DLSWLEN,sh,0x01);          // shift référencé au début du switch
                              }break;       
              case 33: what=5;{uint8_t v=0,b=0;frecupptr(libfonctions+2*i,&v,&b,MAXTAC);             // numéro détecteur
                              *(periSwMode+v) &= ~(mask[SWMDLNUMS_PB-SWMDLNULS_PB+1]<<SWMDLNULS_PB);
                              *(periSwMode+v) |= (*valf&mask[SWMDLNUMS_PB-SWMDLNULS_PB+1])<<SWMDLNULS_PB;
                              };break;         // periSwMode N° detec
              case 34: what=5;{uint8_t v=0,b=0;frecupptr(libfonctions+2*i,&v,&b,MAXTAC);
                              *(periSwMode+v) &= maskbit[b*2];if(*valf&0x01!=0){*(periSwMode+v) |= maskbit[b*2+1];} 
                              };break;         // periSwMode 6 checkbox
              case 35: what=5;{int sw=*(libfonctions+2*i)-PMFNCHAR;
                              *(periSwPulseOne+sw)=0;*(periSwPulseOne+sw)=(uint32_t)convStrToNum(valf,&j);                              
                              };break;         // peri Pulse On
              case 36: what=5;{int sw=*(libfonctions+2*i)-PMFNCHAR;
                              *(periSwPulseTwo+sw)=0;*(periSwPulseTwo+sw)=(uint32_t)convStrToNum(valf,&j);
                              };break;         // peri Pulse Off
              default:break;
              }
              
            }     // i<NBVAL   
          }       // fin boucle nbre params
          
          if(nbreparams>=0){
            Serial.print("what=");Serial.print(what);Serial.print(" periCur=");Serial.print(periCur);Serial.print(" strSD=");Serial.print(strSD);
            sdstore_textdh(&fhisto,"ip","CX",strSD);  // utilise getdate() et place date et heure dans l'enregistrement
          }                                           // 1 ligne par commande GET

        switch(what){                                           // periParamsHtml fait periSave
          case 0:break;                                         // fonctions ponctuelles du serveur
          case 1:periParamsHtml(&cli_a," ",0);break;            // data_save
          case 2:break;                                         // dispo
          case 3:periParamsHtml(&cli_a," ",0);break;            // data_read
          case 4:periSave(periCur);periTableHtml(&cli_a);break; // fonctions sans mise à jour des périphériques
          case 5:periSend();periTableHtml(&cli_a);break;        // fonctions avec mise à jour des périphériques (fait periParamsHtml)
          default:accueilHtml(&cli_a,passok);break;
          }
          valeurs[0]='\0';
          cli_a.stop();
          delay(1);
          Serial.println("cli stopped"); 
      }   // cli.connected
    }     // server.available

// *** blink

      ledblink(0);   
          
// *** save date
      
      if(curdate+perdate<(long)(millis()/60000))
        {curdate=(long)(millis()/60000);sdstore_textdh(&fhisto,"--","DH"," ");}
      
// *** maj température  

      if((curtemp+pertemp)<(long)(millis()/1000) || curtemp==0)
        {
        char buf[]={0,0,'.',0,0,0};
        readDS3231temp(&msb,&lsb);
        sprintf(buf,"%02u",msb);sprintf(buf+3,"%02u",lsb);
        sdstore_textdh(&fhisto,"T=",buf,"<br>\n");
        }
        
} // loop


// ========================================= tools ====================================


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
    periLoad(periCur);if(memcmp(periMacBuf,periMacr,6)!=0){periCur=0;}}
    
  if(periCur==0){                                                 // si periCur=0 recherche si mac connu   
    for(i=1;i<=NBPERIF;i++){                                      // et, au cas où, une place libre
      periLoad(i);
      if(compMac(periMacBuf,periMacr)){periCur=i;i=NBPERIF+1;}    // mac trouvé
      if(memcmp("\0\0\0\0\0\0",periMacr,6)==0 && perizer==0){perizer=i;}        // place libre trouvée
    }
  }
    
  if(periCur==0 && perizer!=0){                                   // si pas connu utilisation N° perif libre "perizer"
      Serial.println(" Mac inconnu");
      periInitVar();periCur=perizer;periLoad(periCur);periMess=MESSFULL;
  }

  if(periCur!=0){                                                 // si ni trouvé, ni place libre, periCur=0 
    Serial.println(" place libre ou Mac connu");
       
    strncpy((char*)periMacr,(char*)periMacBuf,6);
    k=valf+2+1+17+1;*periLastVal=convStrToNum(k,&i);                                    // température si save
    k+=i;convStrToNum(k,&i);                                                            // age si save
    k+=i;*periAlim=convStrToNum(k,&i);                                                  // alim
    k+=i;strncpy(periVers,k,LENVERSION);                                                // version
    k+=strchr(k,'_')-k+1; uint8_t qsw=(uint8_t)(*k-48);                                               // nbre sw
    k+=1; *periSwVal&=0xAA;for(int i=MAXSW-1;i>=0;i--){*periSwVal |= (*(k+i)-48)<< 2*(MAXSW-1-i);}    // periSwVal états sw
    k+=MAXSW+1; *periDetNb=(uint8_t)(*k-48);                                                          // nbre detec
    k+=1; *periDetVal=0;for(int i=MAXDET-1;i>=0;i--){*periDetVal |= (*(k+i)-48)<< 2*(MAXDET-1-i);}    // détecteurs
    k+=MAXDET+1;for(int i=0;i<MAXSW;i++){periSwPulseSta[i]=*(k+i);}                     // pulse clk status 
    k+=MAXSW+1;for(int i=0;i<LENMODEL;i++){periModel[i]=*(k+i);periNamer[i]=*(k+i);}    // model
    k+=LENMODEL+1;
    
    for(int i=0;i<4;i++){periIpAddr[i]=remote_IP_cur[i];}                               // Ip addr
    char date14[14];alphaNow(date14);packDate(periLastDateIn,date14+2);                 // maj dates

    periSave(periCur);
  }
}

void periSend()                 // configure periParamsHtml pour envoyer un set_______ à un périph serveur
{
  char ipaddr[16];memset(ipaddr,'\0',16);
  charIp(periIpAddr,ipaddr);
  if(periParamsHtml(&cliext,ipaddr,PORTSERVPERI)==MESSOK){
    char date14[14];alphaNow(date14);packDate(periLastDateIn,date14+2);
  }
}

int periParamsHtml(EthernetClient* cli,char* host,int port)        
                   // status retour de messToServer ou getHttpResponse ou fonct invalide
                   // (doit être done___)
                   // periCur est à jour (0 ou n) et periMess contient le diag du dataread/save reçu
                   // un message de paramètres pour périph ; 
                   // soit encapsulé dans <body>...</body></html>\n\r en réponse à dataread/save etc
                   // soit envoyé à un périph-serveur via GET (dans ce cas port != 0)
                   // format nomfonction_=nnnndatas...cc                nnnn len mess ; cc crc
                   // datas NN_mm.mm.mm.mm.mm.mm_AAMMJJHHMMSS_nn..._    NN numpériph ; mm.mm... mac
{                   
  char message[LENMESS]={'\0'};
  char nomfonct[LENNOM+1]={'\0'};
  int zz=1;
  char date14[15];alphaNow(date14);
  
  if((periCur!=0) && (what==1) && (port==0)){strcpy(nomfonct,"ack_______");}    // ack pour datasave (what=1)
  else strcpy(nomfonct,"set_______");

  assySet(message,periCur,periDiag(periMess),date14);

  *bufServer='\0';

          if(port!=0){memcpy(bufServer,"GET /\0",6);}  
          buildMess(nomfonct,message,"");            // bufServer complété   

          if(port==0){                               // réponse à dataRead/Save
            htmlIntro0(cli);
            cli->print("<body>");
            cli->print(bufServer);
            cli->println("</body></html>");
          }
          else {                                     // envoi vers peri-serveur
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
          periErr=zz;packDate(periLastDateOut,date14+2);periSave(periCur);
          Serial.print("pHtml=");Serial.println(zz);
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
      memset(valeurs,0,LENVALEURS);                     // effacement critique (password etc...)
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
              if (nom==VRAI && c!='?' && c!=':' && c!='&'){noms[j]=c;if(j<LENNOM-1){j++;}}              // acquisition nom
              if (val==VRAI && c!='&' && c!=':' && c!='=' && c!=' '){
                valeurs[nvalf[i+1]]=c;if(nvalf[i+1]<=LENVALEURS-1){nvalf[i+1]++;}}          
              if (val==VRAI && (c=='&' || c==' ')){
                nom=VRAI;val=FAUX;j=0;Serial.println();
                if(c==' '){termine=VRAI;}}                                                              // ' ' interdit dans valeur : indique fin données                                 
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
  
  Serial.println("\n--- getnv");
  
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

void init_params()
{
  autoreset=FAUX;
  sdpos    =0;
  perdate  =PDATE;
  pertemp  =PTEMP;
  perrefr  =0;
}

void conv_atob(char* ascii,int16_t* bin)
{
  int j=0;
  *bin=0;
  for(j=0;j<LENVAL;j++){c=ascii[j];if(c>='0' && c<='9'){*bin=*bin*10+c-48;}else{j=LENVAL;}}
}

void conv_atobl(char* ascii,int32_t* bin)
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

void bitvSwCtl(char* data,uint8_t sw,uint8_t datalen,uint8_t shift,byte msk)   
// place le(s) bit(s) de valf position shift, masque msk, dans data de rang sw longeur datalen 
{
  //Serial.print("sw=");Serial.print(sw);Serial.print(" datalen=");Serial.print(datalen);Serial.print(" shift=");Serial.print(shift);Serial.print(" msk=");Serial.print(msk,HEX);Serial.print(" *valf=");Serial.println(*valf-PMFNCVAL);
  //dumpstr(data,datalen);memset(data,0x00,datalen);
  uint64_t pipm=0;memcpy(&pipm,(char*)(data+sw*datalen),datalen);
  uint64_t c=((uint64_t)msk)<<shift;
  pipm &= ~c;
  c=0;c=((uint64_t)(*valf-PMFNCVAL))<<shift;
  pipm +=c;
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


