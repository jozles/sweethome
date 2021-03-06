
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

  EthernetClient cli_a;             // client du serveur periphériques et browser configuration
  //EthernetClient cli_b;             // client du serveur pilotage
  EthernetClient cliext(4);            // client de serveur externe  
char ab;
    
    IPAddress test(82,64,32,56);
    IPAddress espdev(192,168,0,38);
    IPAddress esptv(192,168,0,208);
    IPAddress gggg(74,125,232,128);    

    long mill=millis();
    
/* >>>> config server <<<<<< */

char configRec[CONFIGRECLEN];

  byte* mac;              // adresse server
  byte* localIp;          // adresse server
  uint16_t* portserver;   //
  char* nomserver;        //
  char* userpass;         // mot de passe browser
  char* modpass;          // mot de passe modif
  char* peripass;         // mot de passe périphériques
  char* ssid;             // MAXSSID ssid
  char* passssid;         // MAXSSID password SSID
  int*  nbssid;           // inutilisé
  char* usrnames;         // usernames
  char* usrpass;          // userpass
  long* usrtime;          // user cx time
  char* thermonames;      // noms sondes thermos
  int16_t* thermoperis;   // num périphériques thermo
  uint16_t* toPassword;   // Délai validité password en sec !

  byte* configBegOfRecord;
  byte* configEndOfRecord;

  bool    periPassOk=FAUX;  // contrôle du mot de passe des périphériques
  int     usernum=-1;       // numéro(0-n) de l'utilisateur connecté (valide durant commonserver)   


EthernetServer periserv(PORTSERVER);  // serveur perif et table port 1789 service, 1790 devt, 1786 devt2
EthernetServer pilotserv(PORTPILOT);  // serveur pilotage 1792 devt, 1788 devt2

  uint8_t remote_IP[4]={0,0,0,0};           // periserver
  uint8_t remote_IP_cur[4]={0,0,0,0};       // périphériques periserver
  uint8_t remote_IP_Mac[4]={0,0,0,0};       // maintenance periserver
  uint8_t remote_IPb[4]={0,0,0,0};          // pilotserver
  byte    remote_MAC[6]={0,0,0,0,0,0};      // periserver
  byte    remote_MACb[6]={0,0,0,0,0,0};     // pilotserver


  int8_t  numfonct[NBVAL];             // les fonctions trouvées  (au max version 1.1k 23+4*57=251)
  
  char*   fonctions="per_temp__peri_pass_username__password__user_ref__to_passwd_per_refr__peri_tofs_switchs___reset_____dump_sd___sd_pos____data_save_data_read_peri_swb__peri_cur__peri_refr_peri_nom__peri_mac__accueil___peri_tableperi_prog_peri_sondeperi_pitchperi_pmo__peri_detnbperi_intnbperi_rtempremote____testhtml__peri_vsw__peri_t_sw_peri_otf__peri_imn__peri_imc__peri_pto__peri_ptt__peri_thminperi_thmaxperi_vmin_peri_vmax_peri_dsv__mem_dsrv__ssid______passssid__usrname___usrpass____cfgserv___pwdcfg____modpcfg___peripcfg__ethcfg____remotecfg_remote_ctlremotehtmlthername__therperi__thermohtmlperi_port_tim_name__tim_det___tim_hdf___tim_chkb__timershtmldone______last_fonc_";
  
  /*  nombre fonctions, valeur pour accueil, data_save_ fonctions multiples etc */
  int     nbfonct=0,faccueil=0,fdatasave=0,fperiSwVal=0,fperiDetSs=0,fdone=0,fpericur=0,fperipass=0,fpassword=0,fusername=0,fuserref=0;
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

  float     oldth=0;                       // pour prev temp DS3231
  long      temptime=0;                    // last millis() pour temp
#define PTEMP 120                          // secondes
  uint32_t  pertemp=PTEMP;                 // période ech temp sur le serveur
  uint16_t  perrefr=0;                     // periode rafraichissement de l'affichage

  long      timerstime=0;                  // last millis pour timers
#define PTIMERS 10;                        // secondes
  uint32_t  pertimers=PTIMERS;             // période ctle timers
  
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
  
  uint16_t  periCur=0;                          // Numéro du périphérique courant
  
  uint16_t* periNum;                        // ptr ds buffer : Numéro du périphérique courant
  uint32_t* periPerRefr;                    // ptr ds buffer : période maximale d'accès au serveur
  uint16_t* periPerTemp;                    // ptr ds buffer : période de lecture tempèrature
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
  uint16_t* periPort;                       // ptr ds buffer : port periph server
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

byte   periRemoteStatus[NBPERIF];            // etat de la mise à jour des périphériques depuis le remote ctl (si 0 faire periload/perisend - voir remoteUpdate)

struct SwRemote remoteT[MAXREMLI];
char*  remoteTA=(char*)&remoteT;
long   remoteTlen=(sizeof(SwRemote))*MAXREMLI;

struct Remote remoteN[NBREMOTE];
char*  remoteNA=(char*)&remoteN;
long   remoteNlen=(sizeof(Remote))*NBREMOTE;

struct Timers timersN[NBTIMERS];
char*  timersNA=(char*)&timersN;
long   timersNlen=(sizeof(Timers))*NBTIMERS;

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
 *  l'objet client est utilisé pour l'accès aux serveurs externes ET l'accès aux clients du serveur interne
 *  EthernetClient nom_client crée les objets clients pour serveur externe ou interne (capacité 4 clients)
 *                                                                                                          
 *  pour creer/utiliser un serveur : EthernetServer nom_serveur(port) crée l'objet ; 4 serveurs possibles sur 4 ports différents
 *                                   nom_serveur.begin()  activation
 *                                   connexions des appels entrants : nom_client_in=nom_serveur.available()
 *                                   nom_serveur.write(byte) ou (buf,len) envoie la donnée à tous les clients 
 *                                   nom_serveur.print(variable) envoie la variable en ascii
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
void periSend(uint16_t np);
int  periParamsHtml(EthernetClient* cli,char* host,int port);
void periDataRead();
void packVal2(byte* value,byte* val);
void frecupptr(char* nomfonct,uint8_t* v,uint8_t* b,uint8_t lenpersw);
void bitvSwCtl(byte* data,uint8_t sw,uint8_t datalen,uint8_t shift,byte msk);
void test2Switchs();
void periserver();
void pilotserver();



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
  fusername=(strstr(fonctions,"username__")-fonctions)/LENNOM;
  fuserref=(strstr(fonctions,"user_ref__")-fonctions)/LENNOM;
  
/* >>>>>>     config     <<<<<< */  
  
  delay(1000);
  Serial.println();Serial.print(VERSION);
  #ifdef _MODE_DEVT || MODE_DEVT2
  Serial.print(" MODE_DEVT");Serial.print(" free=");Serial.println(freeMemory(), DEC);
  #endif _MODE_DEVT/2

  Serial.print("\nSD card ");
  if(!SD.begin(4)){Serial.println("KO");ledblink(BCODESDCARDKO);}
  Serial.println("OK");

  configInit();long configRecLength=(long)configEndOfRecord-(long)configBegOfRecord+1;
  
  Serial.print("CONFIGRECLEN=");Serial.print(CONFIGRECLEN);Serial.print("/");Serial.print(configRecLength);
  delay(100);if(configRecLength!=CONFIGRECLEN){ledblink(BCODECONFIGRECLEN);}
  Serial.print(" nbfonct=");Serial.println(nbfonct);
  //configSave();
  configLoad();configPrint();
  
  periInit();long periRecLength=(long)periEndOfRecord-(long)periBegOfRecord+1;
  
  Serial.print("PERIRECLEN=");Serial.print(PERIRECLEN);Serial.print("/");Serial.print(periRecLength);
  delay(100);if(periRecLength!=PERIRECLEN){ledblink(BCODEPERIRECLEN);}

  Serial.print(" RECCHAR=");Serial.print(RECCHAR);Serial.print(" LBUFSERVER=");Serial.print(LBUFSERVER);
  Serial.print(" free=");Serial.println(freeMemory(), DEC); 

/* >>>>>>  maintenance fichiers peri  <<<<<< */
/*
for(i=0;i<50;i++){
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
*/  
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
    for(i=1;i<=NBPERIF;i++){periSave(i,PERISAVESD);}
    Serial.println("terminé");
    while(1){}
*/
/*  init timers
 
    timersInit();
    timersSave();
    while(1){}; 
*/
  
  sdOpen(FILE_WRITE,&fhisto,"fdhisto.txt");
  //sdstore_textdh0(&fhisto,".1","RE"," ");

/* >>>>>> ethernet start <<<<<< */

  Wire.begin();

  if(Ethernet.begin(mac) == 0)
    {Serial.print("Failed with DHCP... forcing Ip ");serialPrintIp(localIp);Serial.println();
    Ethernet.begin (mac, localIp); //initialisation de la communication Ethernet
    }
  Serial.println(Ethernet.localIP());


  periserv.begin();Serial.println("periserv.begin ");   // serveur périphériques

  pilotserv.begin();Serial.println("pilotserv.begin ");  //  remote serveur
  

  if(cliext.connect(esptv,1791)){Serial.println("esptv connected");}
  cliext.stop();

  if(cliext.connect(gggg,80)){Serial.println("gggg connected");}
  cliext.stop();
  
  delay(1000);

#ifndef WEMOS  
 // wdt_enable(WDTO_4S);                // watch dog init (4 sec)
#endif ndef WEMOS

/* >>>>>> RTC ON, check date/heure et maj éventuelle par NTP  <<<<<< */

  digitalWrite(PINGNDDS,LOW);pinMode(PINGNDDS,OUTPUT);  
  digitalWrite(PINVCCDS,HIGH);pinMode(PINVCCDS,OUTPUT);  

#ifdef UDPUSAGE
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
#endif UDPUSAGE
#ifndef UDPUSAGE
  getdate(&hms2,&amj2,&js2);sdstore_textdh0(&fhisto,"ST","RE"," ");
  Serial.print(" DS3231 time ");Serial.print(js2);Serial.print(" ");Serial.print(amj2);Serial.print(" ");Serial.println(hms2);
#endif UDPUSAGE
  
  sdstore_textdh(&fhisto,".3","RE","<br>\n\0");

  remoteLoad();
  timersLoad();

  Serial.println("fin setup");
}

/*=================== fin setup ============================ */


void getremote_IP(EthernetClient *client,uint8_t* ptremote_IP,byte* ptremote_MAC)
{
    W5100.readSnDHAR(client->getSocketNumber(), ptremote_MAC);
    W5100.readSnDIPR(client->getSocketNumber(), ptremote_IP);
}



/* ======================================= loop ===================================== */

void loop()                         
{

            periserver();     // *** périphérique ou maintenance

            pilotserver();    // *** pilotage

            ledblink(0);  

            scantemp();

            scantimers();
}

 


/* ========================================= tools =================================== */

void scantemp()
{
    if((millis()-temptime)>pertemp*1000){   // *** maj température  
      temptime=millis();
      char buf[]={0,0,'.',0,0,0};
      float th;
      readDS3231temp(&th);
      if(fabs(th-oldth)>MINTHCHGE){
        oldth=th;sprintf(buf,"%02.02f",th);
        sdstore_textdh(&fhisto,"T=",buf,"<br>\n\0");
      }
    }       
}


void poolperif(uint8_t nt,int* tablePerToSend,uint8_t detec,char* onoff)
{
  Serial.print("poolperif(timer=");Serial.print(nt);Serial.print(" ");Serial.print(onoff);Serial.println(")");
  for(uint16_t np=1;np<=NBPERIF;np++){                                                 // boucle périf
    periLoad(np);
    if(*periSwNb!=0){
      for(int sw=0;sw<*periSwNb;sw++){                                                // boucle switchs
        for(int nd=0;nd<DLNB;nd++){                                                   // boucle détecteurs
          uint64_t pipm=0;memcpy(&pipm,((char*)(periSwPulseCtl+sw*DLSWLEN)),DLSWLEN); // mot switch 
          uint8_t deten =(pipm>>(nd*DLBITLEN+DLENA_PB)&0x01);                         // enable
          uint8_t locext=(pipm>>(nd*DLBITLEN+DLEL_PB)&0x01);                          // local / externe
          uint8_t numdet=(pipm>>(nd*DLBITLEN+DLNLS_PB)&mask[DLNULEN]);                // num détecteur
          if(deten!=0 && locext==0 && detec==numdet){
            /* trouvé usage du détecteur dans periSwPulseCtl */
            Serial.print(" p=");Serial.print(np);Serial.print(" sw=");Serial.print(sw);Serial.print("/");Serial.print(nd);
            Serial.print(" l/e=");Serial.print(locext);Serial.print(" d=");Serial.print(numdet);Serial.print(" ");
            tablePerToSend[np-1]++;                                                     // periSend à faire sur ce périf            
            for(int nnp=0;nnp<NBPERIF;nnp++){Serial.print(tablePerToSend[nnp]);Serial.print(" ");}Serial.println();
          }
        }
      }
    }
  }
}

void scantimers()
{
    if((millis()-timerstime)>pertimers*1000){

      int tablePerToSend[NBPERIF];memset(tablePerToSend,0x00,NBPERIF*sizeof(int));      // !=0 si periSend à faire sur le perif
      timerstime=millis();
      char now[LNOW];
      alphaNow(now);
      for(int nt=0;nt<NBTIMERS;nt++){
        if(
          timersN[nt].enable==1
          && (timersN[nt].perm==1 || (memcmp(timersN[nt].dhdebcycle,now,14)<0 && memcmp(timersN[nt].dhfincycle,now,14)>0)) 
          && memcmp(timersN[nt].hdeb,(now+8),6)<0 
          && memcmp(timersN[nt].hfin,(now+8),6)>0
          && (timersN[nt].dw & maskbit[1+now[14]*2])!=0 )
          {
          if(timersN[nt].curstate!=1){                    // changement 0->1
            timersN[nt].curstate=1;memDetServ |= maskbit[1+(timersN[nt].detec)*2];
            poolperif(nt,tablePerToSend,timersN[nt].detec,"on");}
        }
        else {
          if(timersN[nt].curstate!=0){                    // changement 1->0
            timersN[nt].curstate=0;memDetServ &= maskbit[(timersN[nt].detec)*2];
            poolperif(nt,tablePerToSend,timersN[nt].detec,"off");}
        }
      }     
      if((millis()-timerstime)>1){Serial.print("durée scan timers =");Serial.println(millis()-timerstime);}
      for(uint16_t np=1;np<=NBPERIF;np++){if(tablePerToSend[np-1]!=0){periSend(np);}}
      if((millis()-timerstime)>1){Serial.print("durée scan+send timers =");Serial.print(millis()-timerstime);
        Serial.print(" ");
        for(int nnp=0;nnp<NBPERIF;nnp++){Serial.print(tablePerToSend[nnp]);}Serial.println();}
    }
}


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
      if(compMac(periMacBuf,periMacr)){
        periCur=i;i=NBPERIF+1;
        Serial.println(" DataRead/Save Mac connu");
      }                                                                         // mac trouvé
      if(memcmp("\0\0\0\0\0\0",periMacr,6)==0 && perizer==0){
        perizer=i;
        Serial.println(" DataRead/Save place libre");
      }        // place libre trouvée
    }
  }
    
  if(periCur==0 && perizer!=0){                                   // si pas connu utilisation N° perif libre "perizer"
      Serial.println(" DataRead/Save Mac inconnu");
      periInitVar();periCur=perizer;periLoad(periCur);
      periMess=MESSFULL;
  }

  if(periCur!=0){                                                 // si ni trouvé, ni place libre, periCur=0 
    memcpy(periMacr,periMacBuf,6);
#define PNP 2+1+17+1
    k=valf+PNP;*periLastVal=convStrToNum(k,&i);                                    // température si save
#if PNP != SDPOSTEMP-SDPOSNUMPER
    periCur/=0;
#endif     
    k+=i;convStrToNum(k,&i);                                                            // age si save
    k+=i;*periAlim=convStrToNum(k,&i);                                                  // alim
    k+=i;strncpy(periVers,k,LENVERSION);                                                // version
    k+=strchr(k,'_')-k+1; uint8_t qsw=(uint8_t)(*k-48);                                 // nbre sw
    k+=1; *periSwVal&=0xAA;for(int i=MAXSW-1;i>=0;i--){*periSwVal |= (*(k+i)-48)<< 2*(MAXSW-1-i);}    // periSwVal états sw
    k+=MAXSW+1; *periDetNb=(uint8_t)(*k-48);                                                          // nbre detec
    k+=1; *periDetVal=0;for(int i=MAXDET-1;i>=0;i--){*periDetVal |= ((*(k+i)-48)&DETBITLH_VB )<< 2*(MAXDET-1-i);}    // détecteurs
    k+=MAXDET+1;for(int i=0;i<MAXSW;i++){periSwPulseSta[i]=*(k+i);}                     // pulse clk status 
    k+=MAXSW+1;for(int i=0;i<LENMODEL;i++){periModel[i]=*(k+i);periNamer[i]=*(k+i);}    // model
  if(memcmp(periVers,"1.g",3)>=0){
    k+=LENMODEL+1;for(int i=MAXSW-1;i>=0;i--){if(*(k+i)=='1'){*periSwVal^(0x02<<(i*2));}} // toggle bits
    k+=MAXSW+1; 
  }  
    memcpy(periIpAddr,remote_IP_cur,4);            //for(int i=0;i<4;i++){periIpAddr[i]=remote_IP_cur[i];}      // Ip addr
    char date14[LNOW];alphaNow(date14);checkdate(0);packDate(periLastDateIn,date14+2);    // maj dates
    Serial.print("periDataRead peri=");periPrint(periCur);
    periSave(periCur,PERISAVESD);checkdate(6);
  }
}

void periSend(uint16_t np)                 // configure periParamsHtml pour envoyer un set_______ au périph serveur de np (avec cb prog cochée)
{                                          // periCur, periLoad rechargés avec np 
  periCur=np;periLoad(periCur);
  Serial.print("periSend(peri=");Serial.print(periCur);Serial.print("-port=");Serial.print(*periPort);Serial.println(")");
  if(*periProg!=0 && *periPort!=0){
    checkdate(2);
    char ipaddr[16];memset(ipaddr,'\0',16);
    charIp(periIpAddr,ipaddr);
    periParamsHtml(&cliext,ipaddr,(int)*periPort);
  }
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
  char date14[LNOW];alphaNow(date14);
  
  if((periCur!=0) && (what==1) && (port==0)){strcpy(nomfonct,"ack_______");}    // ack pour datasave (what=1)
  else strcpy(nomfonct,"set_______");
  Serial.print("periParamsHtml(peri=");Serial.print(periCur);Serial.print("-port=");Serial.print(port);Serial.println(")");
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
            uint8_t fonct;
            if(zz==MESSOK){
              zz=getHttpResponse(cli,bufServer,LBUFSERVER,&fonct);
              if(zz==MESSOK && fonct!=fdone){zz=MESSFON;}
              delay(1);
              purgeServer(cli);
            }
          }
          checkdate(5);
          if(zz==MESSOK){packDate(periLastDateOut,date14+2);}
          *periErr=zz;
          periSave(periCur,PERISAVESD);                  // modifs de periTable et date effacèe par prochain periLoad si pas save
          cli->stop();
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

                long numfonc=(strstr(fonctions,noms)-fonctions)/LENNOM;     // acquisition nom terminée récup N° fonction
                memcpy(libfonctions+2*i,noms+LENNOM-2,2);                   // les 2 derniers car du nom de fonction si nom court

                if(numfonc<0 || numfonc>=nbfonct){
                  memcpy(nomsc,noms,LENNOM-2);
                  numfonc=(strstr(fonctions,nomsc)-fonctions)/LENNOM;   // si nom long pas trouvé, recherche nom court (complété par nn)

                  if(numfonc<0 || numfonc>=nbfonct){numfonc=faccueil;}
                  else {numfonct[i]=numfonc;}
                }
                else {numfonct[i]=numfonc;}
              }
              if (nom==VRAI && c!='?' && c!=':' && c!='&' && c>' '){noms[j]=c;if(j<LENNOM-1){j++;}}              // acquisition nom
              if (val==VRAI && c!='&' && c!=':' && c!='=' && c>' '){
                valeurs[nvalf[i+1]]=c;if(nvalf[i+1]<=LENVALEURS-1){nvalf[i+1]++;}}                               // contrôle decap !
              if (val==VRAI && (c=='&' || c<=' ')){
                nom=VRAI;val=FAUX;j=0;Serial.println();
                if(c==' ' || c<=' '){termine=VRAI;}}                                                             // ' ' interdit dans valeur : indique fin données                                 
            }//Serial.println(libfonctions+2*(i-1));
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
      while (cli->available() && c!='?' && c!='.'){      // attente '?' ou '.'
        c=cli->read();Serial.print(c);
        bufli[pbli]=c;if(pbli<LBUFLI-1){pbli++;bufli[pbli]='\0';}
      }Serial.println();          

        switch(ncde){
          case 1:           // GET
            if(strstr(bufli,"favicon")>0){htmlFavicon(cli);}
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

void switchCtl(uint8_t sw,byte val)
{
  switch(sw){
    case 0:*periSwVal&=0xfd;*periSwVal|=(val&0x01)<<1;break;                           // peri Sw Val 0
    case 1:*periSwVal&=0xf7;*periSwVal|=(val&0x01)<<3;break;                           // peri Sw Val 1
    case 2:*periSwVal&=0xdf;*periSwVal|=(val&0x01)<<5;break;                           // peri Sw Val 2
    case 3:*periSwVal&=0x7f;*periSwVal|=(val&0x01)<<7;break;                           // peri Sw Val 3
    default:break;
  }
}

void periRemoteUpdate()
{
  Serial.print("periRemoteUpdate() ");
  for(uint16_t pp=1;pp<=NBPERIF;pp++){
    if(periRemoteStatus[pp-1]==0){periSend(pp);periRemoteStatus[pp-1]=1;Serial.print(pp);Serial.print(" ");}
  }
  Serial.println();
}

void periRecRemoteUpdate()
/* traitement et mise à jour lorsque toutes les fonctions ont été vues */
{
  Serial.println("periRecRemoteUpdate() ");
  memset(periRemoteStatus,0x01,NBPERIF);   // effacement
  for(uint8_t nbr=0;nbr<NBREMOTE;nbr++){
    if(remoteN[nbr].onoff + remoteN[nbr].newonoff==1){
      Serial.print(" chges ");Serial.print(nbr);
      remoteN[nbr].onoff=remoteN[nbr].newonoff;
      for(uint8_t nbs=0;nbs<MAXREMLI;nbs++){
        if(remoteT[nbs].num==nbr+1){
          periCur=remoteT[nbs].pernum;periRemoteStatus[periCur-1]=0;periLoad(periCur);switchCtl(remoteT[nbs].persw,remoteN[nbr].newonoff);
          periSave(periCur,PERISAVESD);
          Serial.print(" ");Serial.print(periCur);Serial.print("/");Serial.print(remoteT[nbs].persw);
          //Serial.print(remoteT[nbs].num);Serial.print(" ");Serial.print(remoteT[nbs].pernum);Serial.print(" ");Serial.print(remoteT[nbs].persw);Serial.print(" ");
          //Serial.println(*periSwVal,HEX);
        }
      }
    }
  }
  remoteSave();
}

void textfonc(char* nf,int len)
{
  memset(nf,0x00,len);
  memcpy(nf,valf,nvalf[i+1]-nvalf[i]);
}

void test2Switchs()
{

  char ipAddr[16];memset(ipAddr,'\0',16);
  charIp(lastIpAddr,ipAddr);
  for(int x=0;x<4;x++){
//Serial.print(x);Serial.print(" test2sw ");Serial.println(ipAddr);
    testSwitch("GET /testb_on__=0006AB8B",ipAddr,*periPort);
    delay(2000);
    testSwitch("GET /testa_on__=0006AB8B",ipAddr,*periPort);
    delay(2000);
    testSwitch("GET /testboff__=0006AB8B",ipAddr,*periPort);
    delay(2000);
    testSwitch("GET /testaoff__=0006AB8B",ipAddr,*periPort);
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


void commonserver(EthernetClient cli)
{
/*
    
    Une connexion est valide sur le serveur courant (clixx=xxxserv.available() a fonctionné)

    Si le client est un périphérique, la fonction peripass doit précéder dataread/datasave et le mot de passe est systématiquement contrôlé
    Si le client est un browser (la première fonction n'est pas peripass) les 2 premières fonctions doivent être : 
      soit username__ et password__ en cas de login.
      soit usr_ref___ la référence fournie en première zone (hidden) de la page (num user en libfonction+2xi+1) et millis() comme valeur;
*/
        
      getremote_IP(&cli,remote_IP,remote_MAC);

      Serial.print("\n *** serveur actif  IP=");serialPrintIp(remote_IP);Serial.print(" MAC=");serialPrintMac(remote_MAC,1);
      
      if (cli.connected()) 
        {nbreparams=getnv(&cli);Serial.print("\n---- nbreparams ");Serial.println(nbreparams);
        
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

    En cas d'anomalie de transmission le périphérique met son numéro à 0.
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

        memset(strSD,0x00,sizeof(strSD)); memset(buf,0,sizeof(buf));charIp(remote_IP,strSD);       // histo :
        sprintf(buf,"%d",nbreparams+1);strcat(strSD," ");strcat(strSD,buf);strcat(strSD," = ");    // une ligne par transaction
        strcat(strSD,strSdEnd);

/*      
    Un formulaire (<form>....</form>) contient des champs de saisie dont le nom et la valeur sont transmis dans l'ordre d'arrivée 
    quand la fonction submit incluse dans le formulaire est déclenchée (<input type="submit" value="MàJ">)

    La première fonction du formulaire (après le mot de passe) doit fixer les paramètres communs à toutes les autres du formulaire :
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
    si c'est un browser : username__ puis password__ en cas de login, sinon user_ref_n=ttt... et contrôle du TO (ttt... millis() du dernier accés)
                          user_ref__ seule génère peritable
    
    fonctionnement de la rémanence de password_ :
    chaque entrée de la table des utilisateurs incorpore un champ usrtime qui stocke millis() de password puis de la dernière fonction usr_ref__
    les boutons et autres commandes recoivent usrtime et le renvoient lorsque déclenchés. 
    Si usrtime a changé ou si le délai de validité est dépassé ---> accueil.
    Le délai est modifiable dans la config (commun à tous les utilisateurs)
                                       

    Sécurité à développer : pour assurer que le mot de passe n'est pas dérobable et que sans mot de passe on ne peut avoir de réponse du serveur,
    le mot de passe doit être crypté dans une chaine qui change de valeur à chaque transmission ; donc crypter mdp+heure. 
    Le serveur accepte une durée de validité (10sec?) au message et refuse le ré-emploi de la même heure.
    Ajouter du java pour crypter ce qui sort du navigateur ? (les 64 premiers caractères de GET / : username,password,heure/user_ref,heure)
    
*/     

      if(numfonct[0]!=fperipass){
        if((numfonct[0]!=fusername || numfonct[1]!=fpassword) && numfonct[0]!=fuserref){
          what=-1;nbreparams=-1;i=0;numfonct[i]=faccueil;}    // -->> accueil en cas d'anomalie
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

//Serial.print(i);Serial.print(" numfonct[i]=");Serial.print(numfonct[i]);Serial.print(" valf=");Serial.println(valf);

            switch (numfonct[i])      
              {
              case 0:  pertemp=0;conv_atobl(valf,&pertemp);break;                                           // pertemp serveur
              case 1:  if(checkData(valf)==MESSOK){                                                         // peri_pass_
                         periPassOk=ctlpass(valf+5,peripass);                                               // skip len
                         if(periPassOk==FAUX){memset(remote_IP_cur,0x00,4);sdstore_textdh(&fhisto,"pp","ko",strSD);}
                         else {memcpy(remote_IP_cur,remote_IP,4);}
                       }break;
              case 2:  usernum=searchusr(valf);if(usernum<0){                                        // username__
                          what=-1;nbreparams=-1;i=0;numfonct[i]=faccueil;}
                       Serial.print("username:");Serial.print(valf);Serial.print(" usernum=");
                       Serial.print(usernum);Serial.print("/");Serial.print(usrnames+usernum*LENUSRNAME);
                       Serial.print(" usrtime=");Serial.println(usrtime[usernum]);
                       break;
              case 3:  if(!ctlpass(valf,usrpass+usernum*LENUSRPASS)){                                // password__
                         what=-1;nbreparams=-1;i=0;numfonct[i]=faccueil;usrtime[usernum]=0;          // si faux accueil (what=-1)
                         sdstore_textdh(&fhisto,"pw","ko",strSD);}                                   
                       else {Serial.println("password ok");usrtime[usernum]=millis();if(nbreparams==1){what=2;}}
                       break;                                                                        
              case 4:  {usernum=*(libfonctions+2*i+1)-PMFNCHAR;                                      // user_ref__ (argument : millis() envoyées avec la fonction au chargement de la page)
                        long cxtime=0;conv_atobl(valf,(uint32_t*)&cxtime);
                        Serial.print("usr_ref__ : usrnum=");Serial.print(usernum);Serial.print(" millis()/1000=");Serial.print(millis()/1000);Serial.print(" cxtime=");Serial.print(cxtime);Serial.print(" usrtime[nb]=");Serial.println(usrtime[usernum]);
                        if(usrtime[usernum]!=cxtime || (millis()-usrtime[usernum])>(*toPassword*1000)){
                          what=-1;nbreparams=-1;i=0;numfonct[i]=faccueil;usrtime[usernum]=0;}
                        else {Serial.print("user ");Serial.print(usrnames+usernum*LENUSRNAME);Serial.println(" ok");usrtime[usernum]=millis();if(nbreparams==0){what=2;}}
                        }break;                                                                        
              case 5:  *toPassword=TO_PASSWORD;conv_atob(valf,toPassword);Serial.print(" topass=");Serial.println(valf);break;                     // to_passwd_
              case 6:  what=2;perrefr=0;conv_atob(valf,&perrefr);                                    // (en tête peritable) periode refresh browser
                       memDetServ=0;                                                                 // effacement cb det server
                       break;                                                                               
              case 7:  *periThOffset=0;*periThOffset=convStrToNum(valf,&j);break;                       // (ligne peritable) Th Offset
              case 8:  periCur=*(libfonctions+2*i+1)-PMFNCHAR;                                       // bouton switchs___ 
                       periInitVar();periLoad(periCur);
                       SwCtlTableHtml(&cli,*periSwNb,4);
                       break;                                                                               
              case 9:  autoreset=VRAI;break;                                                         // bouton reset
              case 10: dumpsd(&cli);break;                                                           // bouton dump_sd
              case 11: what=2;sdpos=0;conv_atobl(valf,&sdpos);break;                                    // (en-tete peritable) SD pos
              case 12: if(periPassOk==VRAI){what=1;periDataRead();periPassOk==FAUX;}break;           // data_save
              case 13: if(periPassOk==VRAI){what=3;periDataRead();periPassOk==FAUX;}break;           // data_read
              case 14: what=5;{                                                                      // (bouton testsw ?) peri swb 
                       uint8_t sw;sw=*(libfonctions+2*i)-PMFNCHAR;                                      // sw n° switch
                       uint8_t sh;sh=libfonctions[2*i+1]-PMFNCHAR;                                      // sh 0 ou 1 état demandé
                       byte mask=(0x10<<sw*2);
                       *periSwVal&=~mask;*periSwVal|=mask;
                       }break;
              case 15: what=5;periCur=0;conv_atob(valf,&periCur);                                    // submit modifs dans ligne de peritable (peri cur)
                       if(periCur>NBPERIF){periCur=NBPERIF;}                                         
                       periInitVar();periLoad(periCur);
                       *periProg=0;
                       break;                                                                        
              case 16: *periPerRefr=0;conv_atobl(valf,periPerRefr);break;                               // (ligne peritable) periode maxi accès serveur periph courant
              case 17: memset(periNamer,0x00,PERINAMLEN-1);                                             // (ligne peritable) nom periph courant
                       memcpy(periNamer,valf,nvalf[i+1]-nvalf[i]);
                       break;                                                                              
              case 18: for(j=0;j<6;j++){conv_atoh(valf+j*2,(periMacr+j));}break;                        // (ligne peritable) Mac periph courant
              case 19: accueilHtml(&cli);break;                                                      // accueil
              case 20: periTableHtml(&cli);break;                                                    // peri table
              case 21: *periProg=*valf-48;break;                                                        // (ligne peritable) peri prog
              case 22: *periSondeNb=*valf-48;if(*periSondeNb>MAXSDE){*periSondeNb=MAXSDE;}break;        // (ligne peritable) peri sonde
              case 23: *periPitch=0;*periPitch=convStrToNum(valf,&j);break;                             // (ligne peritable) peri pitch
              case 24: {                                                                                // (switwhs) peri SwPulseCtl (pmo) bits détecteurs
                       uint8_t sw;sw=*(libfonctions+2*i)-PMFNCHAR;                                        // sw n° switch
                       uint8_t sh;sh=libfonctions[2*i+1]-PMFNCHAR;if(sh>32){sh-=6;}                       // '-6' pour skip ponctuations (valeur maxi utilisable 2*26=52)
                       uint64_t pipm;memcpy(&pipm,(char*)(periSwPulseCtl+sw),DLSWLEN);                    // sh=[LENNOM-1]-PMFNCHAR nb de shifts dans le détecteur
                       byte msk;msk=0x01;if((sw&0xf0)!=0){msk=mask[DLNULEN];sw&=0x0f;}                    // si !=0 -> n° detecteurs ou code action sinon cb                               
                       bitvSwCtl(periSwPulseCtl,sw,DLSWLEN,sh,msk);            // shift référencé au début du switch
                       }break;
              case 25: *periDetNb=*valf-48;if(*periDetNb>MAXDET){*periDetNb=MAXDET;}break;              // (ligne peritable) peri det Nb  
              case 26: *periSwNb=*valf-48;if(*periSwNb>MAXSW){*periSwNb=MAXSW;}break;                   // (ligne peritable) peri sw Nb                       
              case 27: *periPerTemp=0;conv_atob(valf,periPerTemp);break;                             // periode check température
              case 28: cfgRemoteHtml(&cli);remotePrint();break;                                      // bouton remotecfg_
              case 29: testHtml(&cli);break;                                                         // bouton testhtml
              case 30: {uint8_t sw=*(libfonctions+2*i+1)-48;                                            // (ligne peritable) peri Sw Val 
                        switchCtl(sw,(byte)*valf);
//                       Serial.print(" sw=");Serial.print(sw);Serial.print(" periSwVal=");Serial.println(*periSwVal,HEX);
                       }break;
              case 31: what=5;periCur=0;conv_atob(valf,&periCur);                                    // submit modifs dans tables sw (peri_t_sw_)
                       if(periCur>NBPERIF){periCur=NBPERIF;}
                       periInitVar();periLoad(periCur);cbErase();                                           // effacement checkboxs des switchs du periphérique courant
                       break;  
              case 32: {uint8_t sw=*(libfonctions+2*i)-PMFNCHAR,b=*(libfonctions+2*i+1);                // (switchs) peri SwPulseCtl (otf) bits généraux (FOT)
                        uint8_t sh=0;
                        switch (b){
                           case 'F':sh=PMFRO_PB;break;
                           case 'O':sh=PMTOE_PB;break;
                           case 'T':sh=PMTTE_PB;break;
                           default:break;
                        }
                        bitvSwCtl(periSwPulseCtl,sw,DLSWLEN,sh,0x01);   // shift référencé au début du switch
                       }break;       
              case 33: {uint8_t v=0,b=0;frecupptr(libfonctions+2*i,&v,&b,MAXTAC);                       // (switchs) periSwMode N° detec (imn)
                        *(periSwMode+v) &= ~(mask[SWMDLNUMS_PB-SWMDLNULS_PB+1]<<SWMDLNULS_PB);
                        *(periSwMode+v) |= (*valf&mask[SWMDLNUMS_PB-SWMDLNULS_PB+1])<<SWMDLNULS_PB;
                       }break;                                                                      
              case 34: {uint8_t v=0,b=0;frecupptr(libfonctions+2*i,&v,&b,MAXTAC);                       // (switchs) periSwMode 6 checkbox (imc)
                        *(periSwMode+v) &= maskbit[b*2];if(*valf&0x01!=0){*(periSwMode+v) |= maskbit[b*2+1];} 
                       }break;                                                                      
              case 35: {int sw=*(libfonctions+2*i)-PMFNCHAR;                                            // (switchs) peri Pulse one (pto)
                        *(periSwPulseOne+sw)=0;*(periSwPulseOne+sw)=(uint32_t)convStrToNum(valf,&j);                              
                       }break;                                                                      
              case 36: {int sw=*(libfonctions+2*i)-PMFNCHAR;
                        *(periSwPulseTwo+sw)=0;*(periSwPulseTwo+sw)=(uint32_t)convStrToNum(valf,&j); 
                       }break;                                                                          // (switchs) peri Pulse two (ptt)
              case 37: *periThmin=0;*periThmin=convStrToNum(valf,&j);break;                             // (ligne peritable) Th min
              case 38: *periThmax=0;*periThmax=convStrToNum(valf,&j);break;                             // (ligne peritable) Th max
              case 39: *periVmin=0;*periVmin=(float)convStrToNum(valf,&j);break;                        // (ligne peritable) V min
              case 40: *periVmax=0;*periVmax=convStrToNum(valf,&j);break;                               // (ligne peritable) V max
              case 41: *periDetServEn |= (byte)(*valf-48)<<(*(libfonctions+2*i+1)-PMFNCHAR);break;   // Det Serv enable bits
              case 42: memDetServ |= (byte)(*valf-48)<<(*(libfonctions+2*i+1)-PMFNCHAR);break;       // Det Serv level bits
              case 43: {int nb=*(libfonctions+2*i+1)-PMFNCHAR;                                          // (config) ssid[libf+1]
                       memset(ssid+nb*(LENSSID+1),0x00,LENSSID+1);memcpy(ssid+nb*(LENSSID+1),valf,nvalf[i+1]-nvalf[i]);
                       }break;
              case 44: {int nb=*(libfonctions+2*i+1)-PMFNCHAR;                                          // (config) passssid[libf+1]
                       memset(passssid+nb*(LPWSSID+1),0x00,LENSSID+1);memcpy(passssid+nb*(LPWSSID+1),valf,nvalf[i+1]-nvalf[i]);
                       }break;
              case 45: {int nb=*(libfonctions+2*i+1)-PMFNCHAR;                                          // (config) usrname[libf+1]
                       memset(usrnames+nb*(LENUSRNAME+1),0x00,LENUSRNAME+1);memcpy(usrnames+nb*(LENUSRNAME+1),valf,nvalf[i+1]-nvalf[i]);
                       }break;
              case 46: {int nb=*(libfonctions+2*i+1)-PMFNCHAR;                                          // (config) usrpass[libf+1]
                       memset(usrpass+nb*(LENUSRPASS+1),0x00,LENUSRPASS+1);memcpy(usrpass+nb*(LENUSRPASS+1),valf,nvalf[i+1]-nvalf[i]);
                       }break;                       
              case 47: cfgServerHtml(&cli);configPrint();break;                                      // bouton config
              case 48: what=6;                                                                       // submit depuis cfgServervHtml 1ère fonction 
                       memset(userpass,0x00,LPWD);memcpy(userpass,valf,nvalf[i+1]-nvalf[i]);break;      // (config) pwdcfg____
              case 49: memset(modpass,0x00,LPWD);memcpy(modpass,valf,nvalf[i+1]-nvalf[i]);break;        // (config) modpcfg___
              case 50: memset(peripass,0x00,LPWD);memcpy(peripass,valf,nvalf[i+1]-nvalf[i]);break;      // (config) peripcfg__
              case 51: char eth;eth=*(libfonctions+2*i+1);                                              // (config) eth config
                       switch(eth){
                          case 'i': break;memset(localIp,0x00,4);                                     // (config) localIp
//                                    for(j=0;j<4;j++){conv_atob(valf,localIp+j);}break;   // **** à faire ****
                          case 'p': *portserver=0;conv_atob(valf,portserver);break;                   // (config) portserver
                          case 'm': for(j=0;j<6;j++){conv_atoh(valf+j*2,(mac+j));}break;              // (config) mac
                          default: break;
                       }
                       break;
              case 52: what=8;                                                                       // submit depuis cfgRemotehtml
                       {int nb=*(libfonctions+2*i+1)-PMFNCHAR;
                        switch(*(libfonctions+2*i)){                                               
                            case 'n': memset(remoteN[nb].nam,0x00,LENREMNAM);                           // (remotecfg) no nom remote courante              
                                      memcpy(remoteN[nb].nam,valf,nvalf[i+1]-nvalf[i]);
                                      remoteN[nb].enable=0;remoteN[nb].onoff=0;break;                   // (remotecfg) effacement cb 
                            case 'e': remoteN[nb].enable=*valf-48;break;                                // (remotecfg) en enable remote courante
                            case 'o': remoteN[nb].onoff=*valf-48;break;                                 // (remotecfg) on on/off remote courante
                            case 'u': remoteT[nb].num=*valf-48;                                         // (remotecfg) un N° remote table sw
                                      remoteT[nb].enable=0;break;                                       // (remotecfg) effacement cb
                            case 'p': remoteT[nb].pernum=0;                                             // (remotecfg) pn N° periph table sw
                                      conv_atob(valf,&remoteT[nb].pernum);
                                      if(remoteT[nb].pernum>NBPERIF){remoteT[nb].pernum=NBPERIF;}break;
                            case 's': remoteT[nb].persw=*valf-48;                                       // (remotecfg) n° sw dans le peri
                                      if(remoteT[nb].persw>MAXSW){remoteT[nb].persw=MAXSW;}break;       
                            case 'x': remoteT[nb].enable=*valf-48;break;                                // (remotecfg) xe enable table sw
                            default:break;
                          }
                       }break;
              case 53: what=9;{int nb=*(libfonctions+2*i+1)-PMFNCHAR;                                // submit depuis remoteHtml (cdes on/off)
                       //Serial.print(nb);Serial.print(" ");Serial.println((char)*(libfonctions+2*i));
                       if((char)*(libfonctions+2*i)=='n'){remoteN[nb].newonoff=0;}                      // effacement cb si cn
                       else {remoteN[nb].newonoff=1;}                                                   // check cb si ct
                       }break;                                                                       
              case 54: Serial.println("remoteHtml()");remoteHtml(&cli);break;                        // remotehtml
              case 55: {int nb=*(libfonctions+2*i+1)-PMFNCHAR;                                       // thername_
                       memset(thermonames+nb*(LENTHNAME+1),0x00,LENTHNAME+1);memcpy(thermonames+nb*(LENTHNAME+1),valf,nvalf[i+1]-nvalf[i]);
                       }break;
              case 56: {int nb=*(libfonctions+2*i+1)-PMFNCHAR;                                       // therperi_
                       *(thermoperis+nb)=0;conv_atob(valf,(uint16_t*)(thermoperis+nb));
                       if(*(thermoperis+nb)>NBPERIF){*(thermoperis+nb)=NBPERIF;}
                       }break;
              case 57: Serial.println("thermoHtml()");thermoHtml(&cli);break;                        // thermoHtml
              case 58: *periPort=0;conv_atob(valf,periPort);break;                                   // (ligne peritable) peri_port_
              case 59: what=7;{int nb=*(libfonctions+2*i+1)-PMFNCHAR;                                // (timers) tim_name__
                       textfonc(timersN[nb].nom,LENTIMNAM);
                       //Serial.print("efface cb timers ");Serial.print(nb);Serial.print(" ");Serial.print(timersN[nb].nom);
                       timersN[nb].enable=0;                                                         // (timers) effacement cb     
                       timersN[nb].perm=0;
                       timersN[nb].cyclic=0;
                       timersN[nb].curstate=0;
                       timersN[nb].forceonoff=0;
                       timersN[nb].dw=0;
                       }break;
              case 60: {int nb=*(libfonctions+2*i+1)-PMFNCHAR;                                       // (timers) tim_det___
                       timersN[nb].detec=*valf-48;
                       if(timersN[nb].detec>NBDSRV){timersN[nb].detec=NBDSRV;}
                       //Serial.print(" ");Serial.println(timersN[nb].detec);
                       }break;
              case 61: {int nb=*(libfonctions+2*i+1)-PMFNCHAR;                                       // (timers) tim_hdf___
                        switch (*(libfonctions+2*i)){         
                         case 'd':textfonc(timersN[nb].hdeb,6);
                         //Serial.print(" ");Serial.print(timersN[nb].hdeb);
                         break;
                         case 'f':textfonc(timersN[nb].hfin,6);
                         //Serial.print(" ");Serial.print(timersN[nb].hfin);
                         break;
                         case 'b':textfonc(timersN[nb].dhdebcycle,14);
                         //Serial.print(" ");Serial.print(timersN[nb].dhdebcycle);
                         break;
                         case 'e':textfonc(timersN[nb].dhfincycle,14);
                         //Serial.print(" ");Serial.println(timersN[nb].dhfincycle);
                         break;
                         default:break;
                        } 
                       }break;
              case 62: {int nb=*(libfonctions+2*i+1)-PMFNCHAR;                                       // (timers) tim_chkb__
                        int nv=*(libfonctions+2*i)-PMFNCHAR;
//                        Serial.print(nb);Serial.print(" ");Serial.print(nv);Serial.print(" ");
                        /* e_p_c_f */
                        switch (nv){         
                         case 0:timersN[nb].enable=*valf-48;
                         //Serial.print(timersN[nb].enable);
                         break;
                         case 1:timersN[nb].perm=*valf-48;
                         //Serial.print(timersN[nb].perm);
                         break;
                         case 2:timersN[nb].cyclic=*valf-48;
                         //Serial.print(timersN[nb].cyclic);
                         break;
                         case 3:timersN[nb].forceonoff=*valf-48;//Serial.print(timersN[nb].forceonoff);
                         break;                        
                         default:break;
                        }
                        /* dw */
                        if(nv=NBCBTIM){timersN[nb].dw=0xFF;}
                        if(nv>NBCBTIM){
                          timersN[nb].dw|=maskbit[1+2*(7-nv+NBCBTIM)];                          
                        }
                        //Serial.println();
                         
                       }break;
              case 63: Serial.println("timersHtml()");timersHtml(&cli);break;                        // timershtml
              case 64: break;                                                                        // done                        
                              
              default:break;
              }
              
            }     // i<NBVAL 
          }       // fin boucle nbre params
          
          if(nbreparams>=0){
            Serial.print("what=");Serial.print(what);Serial.print(" periCur=");Serial.print(periCur);Serial.print(" strSD=");Serial.print(strSD);
            sdstore_textdh(&fhisto,"ip","CX",strSD);  // utilise getdate() et place date et heure dans l'enregistrement
          }                                           // 1 ligne par commande GET

/*
   periParamsHtml (fait perisave) effectue une réponse ack ou set ou envoie une commande get /set si applelé par perisend
*/                          
       
        switch(what){                                           
          case 0:break;                                       // fonctions ponctuelles du serveur
          case 1:periParamsHtml(&cli," ",0);break;            // data_save
          case 2:if(ab=='a'){periTableHtml(&cli);}            // peritable ou remote suite à login
                 if(ab=='b'){remoteHtml(&cli);} break;     
          case 3:periParamsHtml(&cli," ",0);break;            // data_read
          case 4:break;                                       // unused                                      
          case 5:periSave(periCur,PERISAVESD);periTableHtml(&cli); // browser modif ligne de peritable ou switchs
                  cli.stop();periSend(periCur);break;                
          case 6:configSave();cfgServerHtml(&cli);break;      // config serveur
          case 7:timersSave();timersHtml(&cli);break;         // timers
          case 8:remoteSave();cfgRemoteHtml(&cli);break;      // bouton remotecfg puis submit
          case 9:periRecRemoteUpdate();remoteHtml(&cli);      // bouton remotehtml ou remote ctl puis submit
                  cli.stop();
                  periRemoteUpdate();break;                   // remoteHtml nécessite la mise à jour de perirec et la connexion cli ; 
                                                              // perisend nécessite qu'elle soit arrêtée ; donc mise à jour en 2 étape via periRemoteStatus
            

          default:accueilHtml(&cli);break;
        }

        valeurs[0]='\0';
        purgeServer(&cli);
        cli.stop();

        delay(1);
        Serial.println("---- cli stopped"); 

    } // cli.connected
}

void periserver()
{
  ab='a';
      if(cli_a = periserv.available())      // attente d'un client
      {
        commonserver(cli_a);
      }
}

void pilotserver()
{
  ab='b';
     // if(cli_b = pilotserv.available())      // attente d'un client
      if(cli_a = pilotserv.available())      // attente d'un client
        {
          commonserver(cli_a);
        }     
}

