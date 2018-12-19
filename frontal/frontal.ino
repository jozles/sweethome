
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
#include "htmlwk.h"


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

  EthernetClient cli;               // client du serveur local
  #define TO_SERV 600 // sec
  EthernetClient cliext;            // client de serveur externe  


#ifdef _MODE_DEVT
  byte mac[] = {0x90, 0xA2, 0xDA, 0x0F, 0xDF, 0xAC}; //adresse mac carte ethernet AB service ; AC devt
  byte localIp[] = {192, 168, 0, 35}; //adresse IP    ---- 34 service, 35 devt
  EthernetServer monserveur(1790);  // port 1789 service, 1790 devt
//  #define NOMSERV "sweet hdev"
#endif _MODE_DEVT

#ifndef _MODE_DEVT
  byte* mac = {0x90, 0xA2, 0xDA, 0x0F, 0xDF, 0xAB}; //adresse mac carte ethernet AB service ; AC devt
  byte localIp[] = {192, 168, 0, 34}; //adresse IP    ---- 34 service, 35 devt
  EthernetServer monserveur(1789);  // port 1789 service, 1790 devt
//  #define NOMSERV "sweet home"
#endif _MODE_DEVT


  uint8_t remote_IP[4]={0,0,0,0},remote_IP_cur[4]={0,0,0,0};
  long    to_serv=0;                   // to du serveur pour reset password
  int     chge_pwd=FAUX;               // interdit le chargement du mot de passe si pas d'affichage de la page de saisie         

  int8_t  numfonct[NBVAL];             // les fonctions trouvées 
  char*   fonctions="per_temp__start_wr__peri_pass_dump_sd___test2sw___done______per_refr__stop_refr_per_date__reset_____password__sd_pos____data_save_data_read_peri_parm_peri_cur__peri_refr_peri_nom__peri_mac__acceuil___peri_tableperi_prog_peri_sondeperi_pitchperi_pmo__peri_detnbperi_intnbperi_intv0peri_intv1peri_intv2peri_intv3peri_dispoperi_dispoperi_imn__peri_imc__peri_pon__peri_pof__peri_imd__last_fonc_";  //};
  int     nbfonct=0,facceuil=0,fdatasave=0,fperiIntVal=0,fperiDetSs=0,fdone=0;         // nombre fonctions, valeur pour acceuil, data_save_ et fonctions quadruples
  char    valeurs[LENVALEURS];         // les valeurs associées à chaque fonction trouvée
  uint16_t nvalf[NBVAL];               // offset dans valeurs[] des valeurs trouvées (séparées par '\0')
  char*   valf;                        // pointeur dans valeurs en cours de décodage
  char    libfonctions[NBVAL*2];        // les 2 chiffres des noms de fonctions courts
  int     nbreparams=0;                // 
  int     what=0;                      // ce qui doit être fait après traitement des fonctions (0=rien)

#define LENCDEHTTP 6
  char*   cdes="GET   POST  \0";       // commandes traitées par le serveur
  char    strSD[RECCHAR]={0};          // buffer enregistrements sur SD
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
  char  password[LENPSW]={0};
  char  pass[]=SRVPASS;
  bool  passok=FAUX;
  
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
  byte*     periMacr;                     // ptr ds buffer : mac address 
  byte*     periIpAddr;                   // ptr ds buffer : Ip address
  byte*     periIntNb;                    // ptr ds buffer : Nbre d'interrupteurs (0 aucun ; maxi 4(MAXSW)            
  byte*     periIntVal;                   // ptr ds buffer : état/cde des inter  
  byte*     periIntMode;                  // ptr ds buffer : Mode fonctionnement inters (4 par switch)           
  uint32_t* periIntPulseOn;               // ptr ds buffer : durée pulses sec ON (0 pas de pulse)
  uint32_t* periIntPulseOff;              // ptr ds buffer : durée pulses sec OFF(mode astable)
  uint32_t* periIntPulseCurrOn;           // ptr ds buffer : temps courant pulses ON
  uint32_t* periIntPulseCurrOff;          // ptr ds buffer : temps courant pulses OFF
  uint16_t* periIntPulseMode;             // ptr ds buffer : mode pulses
  uint8_t*  periSondeNb;                  // ptr ds buffer : nbre sonde
  boolean*  periProg;                     // ptr ds buffer : flag "programmable"
  byte*     periDetNb;                    // ptr ds buffer : Nbre de détecteurs maxi 4 (MAXDET)
  byte*     periDetVal;                   // ptr ds buffer : flag "ON/OFF" si détecteur (2 bits par détec))
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

int  periParamsHtml(EthernetClient* cli,char* host,int port);
void periDataRead();
void packVal2(byte* value,byte* val);
void frecupptr(char* nomfonct,uint8_t* v,uint8_t* b,uint8_t lenpersw);
void test2Switchs();


void setup() {                              // ====================================


  pinMode(PINLED,OUTPUT);
  extern uint8_t cntBlink;
  cntBlink=0;

  digitalWrite(PINGNDDS,LOW);
  pinMode(PINGNDDS,OUTPUT);  
  digitalWrite(PINVCCDS,HIGH);
  pinMode(PINVCCDS,OUTPUT);  

  nbfonct=(strstr(fonctions,"last_fonc_")-fonctions)/LENNOM;
  facceuil=(strstr(fonctions,"accueil___")-fonctions)/LENNOM;
  fdatasave=(strstr(fonctions,"data_save_")-fonctions)/LENNOM;
  fperiIntVal=(strstr(fonctions,"peri_intv0")-fonctions)/LENNOM;
  fdone=(strstr(fonctions,"done______")-fonctions)/LENNOM;
    
  periInit();
  passok=FAUX;
 
  Serial.begin (115200);delay(1000); 

  long periRecLength=(long)periEndOfRecord-(long)periBegOfRecord+1;
  
  Serial.println();Serial.print(VERSION);
  #ifdef _MODE_DEVT
  Serial.print(" MODE_DEVT");
  #endif _MODE_DEVT
  Serial.print(" PERIRECLEN=");Serial.print(PERIRECLEN);Serial.print("/");Serial.print(periRecLength);Serial.print(" RECCHAR=");Serial.print(RECCHAR);Serial.print(" nbfonct=");Serial.print(nbfonct);
  delay(100);if(periRecLength>PERIRECLEN){ledblink(BCODEPERIRECLEN);}
  
  Serial.print("\nSD card ");
  if(!SD.begin(4)){Serial.println("KO");
    ledblink(BCODESDCARDKO);}
  Serial.println("OK");

  
/* 
    if(SD.exists("fdhisto.txt")){SD.remove("fdhisto.txt");}
*/
/*//Modification des fichiers de périphériques   
    Serial.println("conversion en cours...");
    periModif();
    Serial.println("terminé");
    while(1){}
*/  
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
  
  monserveur.begin();   // le serveur est en route et attend une connexion
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


void getremote_IP(EthernetClient *client,uint8_t* ptremote_IP)
  {W5100.readSnDIPR(client->getSocketNumber(), ptremote_IP);}



// =========================================================================================================

void loop()                             // =========================================
{

#ifndef WEMOS
    if(!autoreset){wdt_reset();}        // watch dog reset
#endif ndef WEMOS



    if(cli = monserveur.available())    // attente d'un client (VRAI, il y en a un)
      {

      // dévalidation mot de passe si time out || IP change
      if((to_serv+TO_SERV)<(millis()/1000)){passok=FAUX;chge_pwd=FAUX;}to_serv=millis()/1000;
      getremote_IP(&cli,remote_IP);
      Serial.print("\n**** loop ");serialPrintIp(remote_IP_cur);Serial.print(" new=");serialPrintIp(remote_IP);
      for(i=0;i<4;i++){if(remote_IP[i]!=remote_IP_cur[i]){passok=FAUX;chge_pwd=FAUX;}remote_IP_cur[i]=remote_IP[i];}     //!!!!!! bug probable sur chge_pwd
      //Serial.print(" passok/chge_pwd ");Serial.print(passok);Serial.print("/");Serial.println(chge_pwd);
      
      // ******** strSD (chaine alpha de l'enregistrement SD généré pour chaque fonction à chaque commande GET reçue)
      memset(strSD,0,sizeof(strSD)); memset(buf,0,sizeof(buf));charIp(remote_IP,strSD);
      //for(i=0;i<4;i++){sprintf(buf,"%d",remote_IP[i]);strcat(strSD,buf);if(i<3){strcat(strSD,".");}}
      
      if (cli.connected()) 

        {nbreparams=getnv(&cli);Serial.print("\nnbreparams ");Serial.println(nbreparams);//Serial.print(" passok/chge_pwd ");Serial.print(passok);Serial.print("/");Serial.println(chge_pwd);
/*  getnv() décode la chaine GET ou POST ; le reste est ignoré
    forme ?nom1:valeur1&nom2:valeur2&... etc (NBVAL max)
    le séparateur nom/valeur est ':' ou '=' 
    la liste des noms de fonctions existantes est dans fonctions* ; ils sont de longueur fixe (LENNOM) ; 
    les 2 derniers caractères peuvent être utilisés pour passer des paramètres, 
    la recherche du nom dans la table se fait sur une longueur raccourcie si rien n'est trouvé sur la longueur LENNOM.
    (C'est utilisé pour réduire le nombre de noms de fonctions ; par exemple pour periIntMode - 1 car 4 switchs ; 1 car 4 types d'action et 6 bits(sur 16 possibles))
    la liste des fonctions trouvées est dans numfonct[]
    les valeurs associées sont dans valeurs[] (LENVAL car max)
    les 2 derniers car de chaque fonction trouvée sont dans libfonctions[]
    i pointe sur tout ça !!!
    nbreparams le nbre de fonctions trouvées
    il existe nbfonct fonctions 
    la fonction qui renvoie à l'accueil est facceuil (par ex si le numéro de fonction trouvé est > que nbfonct)
    Il y a une fonction par champ de l'enregistrement de la table des périphériques 
    periCur est censé être valide à tout moment si != 0  
    Lorsque le bouton "maj" d'une ligne de la table html des produits est cliqué, les fonctions/valeurs des variables sont listées
    par POST et getnv() les décode. La première est periCur (qui doit etre en premiere colonne).
    Les autres sont transférées dans periRec puis enregistrées avec periSave(periCur).
*/
    // ******** strSD
    sprintf(buf,"%d",nbreparams+1);strcat(strSD," ");strcat(strSD,buf);strcat(strSD," = ");    
/*
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
    
//        if(!passok){nbreparams=-1;}  // force saisie pswd ; à rebrancher quand test terminé
        what=-1; // si nbre params=-1 (pas de params)
/*
    what indique le traitement à effectuer lorsque toutes les fonctions POSTées ou GETées ont été traitées
    what=4 provoque la sauvegarde de l'enregistrement de table (seuls les champs de l'enr de table ont what=4)
*/
        for (i=0;i<=nbreparams;i++){
          
          if(i<NBVAL && i>=0){
          
            valf=valeurs+nvalf[i];    // valf pointe la ième chaine à traiter dans valeurs[] (terminée par '\0')
            //Serial.print("*** i=");Serial.print(i);Serial.print(" ");for(int z=0;z<LENNOM;z++){Serial.print(fonctions[numfonct[i]*LENNOM+z]);}Serial.print(" nvalf[i]=");Serial.print(nvalf[i]);Serial.print(" *valf=");Serial.println(valf);
                                      // si c'est la dernière chaîne, strlen(valf) est sa longueur 
                                      // c'est obligatoirement le cas pour data_read_ et data_save_ qui terminent le message
            // ******** strSD
            sprintf(buf,"%d",numfonct[i]);strcat(strSD,buf);strcat(strSD," \0");strcat(strSD,valf);strcat(strSD,";");
            
            
            switch (numfonct[i])        // traitement des fonctions
              {
              case 0: pertemp=0;conv_atobl(valf,&pertemp);break;
              case 1: break;
              case 2: if(checkData(valf)==MESSOK){for(j=0;j<LENVAL;j++){                                    // peri_pass_
                        password[j]=valeurs[i*LENVAL+j+5];if(pass[j]==0){j=LENVAL;}else if(password[j]!=pass[j]){passok=FAUX;}}
                        if(passok==FAUX){sdstore_textdh(&fhisto,"pw","ko",strSD);}
                        }break;
              case 3: dumpsd();break;
              case 4: what=0;test2Switchs();break;                                                          // test2sw
              case 5: break;                                                                                // done
              case 6: perrefr=0;conv_atob(valf,(int16_t*)&perrefr);break;
              case 7: break;
              case 8: perdate=0;conv_atobl(valf,&perdate);break;
              case 9: autoreset=VRAI;break;
              case 10: if(chge_pwd){
                          passok=VRAI;
                        for(j=0;j<LENVAL;j++){
                          password[j]=valeurs[i*LENVAL+j];
                          if(pass[j]==0){j=LENVAL;}
                          else if(password[j]!=pass[j]){passok=FAUX;}
                          }
                        if(passok==FAUX){sdstore_textdh(&fhisto,"pw","ko",strSD);}
                        }break;
              case 11: what=0;sdpos=0;conv_atobl(valf,&sdpos);break;                                               // SD pos
              case 12: what=1;periDataRead();break;                                                         // data_save
              case 13: what=3;periDataRead();break;                                                         // data_read
              case 14: what=-1;break;                                                                       // peri_parm
              case 15: what=4;periCur=0;conv_atob(valf,&periCur);                                           
                       if(periCur>NBPERIF){periCur=NBPERIF;}periLoad(periCur);                              // N° périph courant
                       *periSondeNb=0;*periProg=FAUX;*periIntNb=0;*periDetNb=0;break;                       // les checkbox non cochées ne sont pas renvoyées par le navigateur
              case 16: what=4;*periPerRefr=0;conv_atobl(valf,periPerRefr);break;                            // per refr periph courant
              case 17: what=4;memcpy(periNamer,valf,PERINAMLEN);periNamer[PERINAMLEN-1]='\0';               // nom periph courant
                                                            // *** effacement des bits checkbox
                       *periProg=0;                                                                               // periProg                                      
                       for(int k=0;k<MAXSW;k++){*(periIntPulseMode+k) &= 0xe0ff;} break;                          // periIntPulseMode
              case 18: what=4;for(j=0;j<6;j++){conv_atoh(valf+j*2,(periMacr+j));}break;                     // Mac periph courant
              case 19: what=-1;break;                                                                       // accueil
              case 20: what=2;break;                                                                        // peri Table (pas de save)
              case 21: what=4;*periProg=*valf;break;                                                        // peri prog
              case 22: what=4;*periSondeNb=*valf-48;if(*periSondeNb>MAXSDE){*periSondeNb=MAXSDE;}break;     // peri sonde
              case 23: what=5;*periPitch=0;*periPitch=convStrToNum(valf,&j);break;                          // peri pitch
              case 24: what=5;{uint8_t sw, num, locv;if(*(libfonctions+2*i)=='_'){                          // peri pulse mode peri_pmoxy : voir structure nom et donnée dans subMPn
                                  // checkbox
                                  sw=*(libfonctions+2*i+1)&0x3f;num=sw&0x07;sw=sw>>4;locv=*valf&0x01;
                                  *((char*)periIntPulseMode+sw*sizeof(uint16_t)+1) |= maskbit[(num-1)*2+1];
                                  }
                                else {
                                  // num source
                                  sw=*(libfonctions+2*i)&0x3f;num=sw&0x07;sw=sw>>4;locv=(uint8_t)convStrToNum(valf,&j);
                                  char* pipm=((char*)periIntPulseMode)+(sw*sizeof(uint16_t));
                                  byte msk[3]={0x3c,0x33,0x0f}, msk2[3]={0xdf,0xbf,0x7F};           // bits dispo à 0
                                  *(pipm) &= msk[num-1];*(pipm) |= (locv&0x03)<<((num-1)*2);
                                  *(pipm+1) &= msk2[num-1];*(pipm+1) |= (locv&0x04)<<(3+num-1);
                                  }
                              };break;                                                           
              case 25: what=4;*periDetNb=*valf-48;if(*periDetNb>MAXDET){*periDetNb=MAXDET;}break;           // peri det Nb  
              case 26: what=4;*periIntNb=*valf-48;if(*periIntNb>MAXSW){*periIntNb=MAXSW;}                   // peri Int Nb
                       for(int k=0;k<(*periIntNb)*MAXTAC;k++){*(periIntMode+k)&=0xc0;}break;                // *** effacement des bits checkbox
              case 27: what=5;*periIntVal&=0xfd;*periIntVal|=(*valf&0x01)<<1;break;                // peri Int Val Serial.print("02=");Serial.println(*periIntVal,HEX);
              case 28: what=5;*periIntVal&=0xf7;*periIntVal|=(*valf&0x01)<<3;break;                //              Serial.print("08=");Serial.println(*periIntVal,HEX);
              case 29: what=5;*periIntVal&=0xdf;*periIntVal|=(*valf&0x01)<<5;break;                //              Serial.print("20=");Serial.println(*periIntVal,HEX);
              case 30: what=5;*periIntVal&=0x7f;*periIntVal|=(*valf&0x01)<<7;break;                //              Serial.print("80=");Serial.println(*periIntVal,HEX);
              case 31: what=4;break;                                                                 // *** dispo
              case 32: what=4;break;                                                                 // *** dispo
              case 33: what=5;{uint8_t v=0,b=0;frecupptr(libfonctions+2*i,&v,&b,MAXTAC);             // numéro détecteur
                              *(periIntMode+v) &= 0x3f;*(periIntMode+v) |= (*valf&0x03)<<6;
                              };break;          // periIntMode N° detec
              case 34: what=5;{uint8_t v=0,b=0;frecupptr(libfonctions+2*i,&v,&b,MAXTAC);
                              *(periIntMode+v) &= maskbit[b*2];if(*valf&0x01!=0){*(periIntMode+v) |= maskbit[b*2+1];} 
                              };break;         // periIntMode 6 checkbox
              case 35: what=5;{uint8_t v=0,b=0;frecupptr(libfonctions+2*i,&v,&b,1);
                              *(periIntPulseOn+v)=0;*(periIntPulseOn+v)=(uint32_t)convStrToNum(valf,&j);
                              };break;         // peri Pulse On
              case 36: what=5;{uint8_t v=0,b=0;frecupptr(libfonctions+2*i,&v,&b,1);
                              *(periIntPulseOff+v)=0;*(periIntPulseOff+v)=(uint32_t)convStrToNum(valf,&j);
                              };break;        // peri Pulse Off
              default:break;
              }
              
            }     // i<NBVAL   
          }       // nbre params
          // ******** strSD
          if(nbreparams>=0){
            strcat(strSD,"<br>\r\n\0");
            Serial.print("what=");Serial.print(what);Serial.print(" strSD=");Serial.println(strSD);
            sdstore_textdh(&fhisto,"ip","CX",strSD);  // utilise getdate() et place date et heure dans l'enregistrement
          }                                           // 1 ligne par commande GET

        switch(what){
          case 0:break;
          case 1:periParamsHtml(&cli," ",0);break;        // data_save
          case 2:periTableHtml(&cli);break;               // peri_table
          case 3:periParamsHtml(&cli," ",0);break;        // data_read
          case 4:periSave(periCur);periTableHtml(&cli);break;               // fonctions de modifs des params
          case 5:periSend();periSave(periCur);periTableHtml(&cli);break;    // fonctions de modif des périph          
          default:acceuilHtml(&cli,passok);break;
          }
          valeurs[0]='\0';
          cli.stop(); 
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
        char* buf2=&buf[3];
        curtemp=(long)millis()/1000;
        readDS3231temp(&msb,&lsb);
        sprintf(buf,"%02u",msb);sprintf(buf2,"%02u",lsb);
        sdstore_textdh(&fhisto,"T=",buf,"<br>\n");
        }
        
} // loop


// ========================================= tools ====================================



void periDataRead()             // traitement d'une chaine "dataSave" ou "dataRead" en provenance d'un periphérique 
                                // controle len,CRC, charge periCur (N° périf du message)
                                // gère les différentes situations présence/absence/création de l'entrée dans la table des périf
                                // transfère adr mac du message reçu (periMacBuf) vers periRec (periMacr) 
                                // charge la valeur reçue (valf+2+1+17+1) dans periRec (periLastVal) 
                                // charge la valeur reçue (valf+2+1+32+1) dans periRec (periAlim)
                                // charge la valeur reçue (valf+2+1+37+1) dans periRec (periVers)
                                // charge la valeur reçue (valf+2+1+41+1) dans periRec (periIntVal)
                                // charge la valeur reçue (valf+2+1+43+1) dans periRec (periDetVal)
                                // charge l'addr IP du périphérique dans periRec (periIpAddr)
                               
{
  int i=0;
  char* k;
  uint8_t c=0;
  int ii=0;
  
 
                        // check len,crc
  periMess=checkData(valf);if(periMess!=MESSOK){periInitVar();return;}         
  
                        // LEN,CRC OK
  valf+=5;periCur=0;conv_atob(valf,&periCur);packMac(periMacBuf,valf+3);                       
  
  if(periCur!=0){       // le périph a un numéro, ctle de l'adr mac
    periLoad(periCur);
    Serial.print("periCur=");Serial.print(periCur);Serial.print(" periIntVal=");Serial.println(*periIntVal,HEX); 
    if(!compMac(periMacBuf,periMacr)){periCur=0;}}
  else {                // periCur=0 d'abord recherche si mac connu   
    for(i=1;i<=NBPERIF;i++){periLoad(i);if(compMac(periMacBuf,periMacr)){periCur=i;i=NBPERIF+1;}}
    
    if(periCur==0){     // si pas connu recherche mac=0 (N° perif libre)
      Serial.println("Mac inconnu");
      for(i=NBPERIF;i>0;i--){periLoad(i);
        if(compMac((byte*)"\0\0\0\0\0\0",periMacr)){periInitVar();periCur=i;periMess=MESSFULL;}
      }
    }     
  }                     // si pas trouvé place libre periCur=i=0 
  if(periCur!=0){
    Serial.println("place libre ou Mac connu");
   
    strncpy((char*)periMacr,(char*)periMacBuf,6);//Serial.print("periMacr avt save=");serialPrintMac(periMacr);
    k=valf+2+1+17+1;*periLastVal=convStrToNum(k,&i);        // température si save
    k+=i;convStrToNum(k,&i);                                // age si save
    k+=i;*periAlim=convStrToNum(k,&i);                      // alim
    k+=i;strncpy(periVers,k,3);                             // version
    k+=4; uint8_t qsw=(uint8_t)(*k-48);                     // nbre sw
    k+=1; *periIntVal&=0xAA;for(int i=qsw-1;i>=0;i--){*periIntVal |= (*(k+i)-48)<< 2*i;}   // periIntVal états sw
    k+=5; *periDetNb=(uint8_t)(*k-48);                      // nbre detec
    k+=1; *periDetVal=0;for(int i=3;i>=0;i--){*periDetVal |= (*(k+i)-48)<< 2*i;}
    k+=5;
    for(int i=0;i<4;i++){periIpAddr[i]=remote_IP_cur[i];}       
    char date14[14];alphaNow(date14);packDate(periLastDateIn,date14+2);

    periSave(periCur);
  }
}

void periSend()                 // configure periParamsHtml pour envoyer un set_______ à un périph serveur
{
  char ipaddr[16];memset(ipaddr,'\0',16);
  charIp(periIpAddr,ipaddr);
  if(periParamsHtml(&cliext,ipaddr,80)==MESSOK){
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
  int zz;
  
  if((periCur!=0) && (what==1) && (port==0)){strcpy(nomfonct,"ack_______");}    // ack pour datasave (what=1)
  else strcpy(nomfonct,"set_______");
//Serial.print("periCur=");Serial.print(periCur);Serial.print(" what=");Serial.print(what);Serial.print(" port=");Serial.print(port);Serial.print(" nomfonct=");Serial.println(nomfonct);
  char date14[15];alphaNow(date14);

  assySet(message,periCur,periDiag(periMess),date14);

          *bufServer='\0';
          buildMess(nomfonct,message,"");            // bufServer complété   

          packDate(periLastDateOut,date14+2);periSave(periCur);

          if(port==0){                               // réponse à dataRead/Save
            htmlIntro0(cli);
            cli->print("<body>");
            cli->print(bufServer);
            cli->println("</body></html>");
          }
          else {                                     // envoi vers peri-serveur
            memcpy(bufServer,"GET /\0",6);
            zz=messToServer(cli,host,port,bufServer);
            Serial.println(zz);
            if(zz==MESSOK){
              uint8_t fonct;
              zz=getHttpResponse(&cliext,bufServer,LBUFSERVER,&fonct);
              if(zz==MESSOK && fonct!=fdone){zz=MESSFON;}
              Serial.println(zz);
            }
            purgeServer(&cliext);
          }
          return zz;
}

int getcde(EthernetClient* cli) // décodage commande reçue selon tables 'cdes' longueur maxi LENCDEHTTP
{
  char c='\0',code[4],cde[LENCDEHTTP],pageErr[11]={0};
  int k=0,ncde=0;
  
  while (cli->available() && c!='/' && k<LENCDEHTTP) {
    c=cli->read();Serial.print(c);     // décode la commande 
    if(c!='/'){cde[k]=c;k++;}
    else {cde[k]=0;break;}
    }
  if (c!='/'){strcpy(code,"400");}                    // pas de commande, message 400 Bad Request
  else if (strstr(cdes,cde)==0){strcpy(code,"501");}  // commande inconnue 501 Not Implemented
  else {ncde=1+(strstr(cdes,cde)-cdes)/LENCDEHTTP ;}  // numéro de commande OK (>0)
  if (ncde<0){while (cli->available()){cli->read();}strcpy(pageErr,"err");strcat(pageErr,code);strcat(pageErr,"txt");htmlPrint(cli,&fhtml,pageErr);}
  return ncde;                                        // ncd=0 si KO ; 1 à n numéro de commande
}

int analyse(EthernetClient* cli)              // decode la chaine et remplit les tableaux noms/valeurs 
{                                             // prochain car = premier du premier nom
                                              // les caractères de ctle du flux sont encodés %HH par le navigateur
                                              // '%' encodé '%25' ; '@' '%40' etc... 

  boolean nom=VRAI,val=FAUX,termine=FAUX;
  int i=0,j=0,k=0;
  char c,cpc='\0';                         // cpc pour convertir les séquences %hh 
  char noms[LENNOM+1]={0},nomsc[LENNOM-1];noms[LENNOM]='\0';nomsc[LENNOM-2]='\0';

      nvalf[0]=0;
      memset(valeurs,0,LENVALEURS);
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
          
              if (nom==FAUX && (c=='?' || c=='&')){nom=VRAI;val=FAUX;j=0;memset(noms,' ',LENNOM);if(i<NBVAL){i++;};Serial.println();}  // fonction suivante ; i indice fonction courante ; numfonct[i] N° fonction trouvée
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
                  if(numfonct[i]<0 || numfonct[i]>=nbfonct){numfonct[i]=facceuil;}
                }
              }
              if (nom==VRAI && c!='?' && c!=':' && c!='&'){noms[j]=c;if(j<LENNOM-1){j++;}}              // acquisition nom
              if (val==VRAI && c!='&' && c!=':' && c!='=' && c!=' '){
                valeurs[nvalf[i+1]]=c;if(nvalf[i+1]<=LENVALEURS-1){nvalf[i+1]++;}}          
              if (val==VRAI && (c=='&' || c==' ')){nom=VRAI;val=FAUX;j=0;Serial.println();
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
      
      while (cli->available()){      // skip en-têtes
        c=cli->read();Serial.print(c);
        bufli[pbli]=c;if(pbli<LBUFLI-1){pbli++;bufli[pbli]='\0';}

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
      }
      if(numfonct[0]<0){return -1;} else return 0; 
}

/*
int extServerSend(byte* ip,int port,char* message,byte len,char* rep)             // renvoie 0 si pas de client extérieur dispo à ip,port
                                                                                  //        -1 si la réponse attendue n'est pas là
{                                                                                 //         1 si la bonne réponse est là
  conv_htoa(message+5,&len);message[len+2]='\0';
  uint8_t c=calcCrc(message,len);conv_htoa(message+len,&c);message[len+2]='\0';
  Serial.println();serialPrintIp(ip);Serial.print(" ");Serial.print(port);
  Serial.print(" ");Serial.print(message);Serial.print(" ");Serial.print(len);
  Serial.print(" ");Serial.println(rep);
  
  EthernetClient cliext0;
  if(!cliext0.connect(ip, port)){return 0;}                               // >>>>>>>>>>> pas de client exterieur 
  Serial.println(" connected");
  cliext0.print(message);cliext0.println(" HTTP/1.1");
  cliext0.println("Connection: close");
  cliext0.println();
  String input="";
  int l=0;
  delay(100);
  while(cliext0.available()){
    char cc=cliext0.read();Serial.print(cc);
    input+=cc;l++;
    if(input.indexOf("</html>")>=0 || l>150){break;}  
  }
  Serial.println();Serial.print(l);
  cliext0.stop();
  if(input.indexOf(rep)>=0){return 1;}
  return -1;
}
*/

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

void acceuilHtml(EthernetClient* cli,bool passok)
{
          if(!passok) 
            {
            Serial.println(" saisie pwd");
            init_params();chge_pwd=VRAI;
            htmlIntro(NOMSERV,cli);

            //htmlTest();                             // ********************* pour recharger les fichiers html
            cli->println(VERSION);
            htmlPrint(cli,&fhtml,"passw.txt");
            //htmlPrint(cli,&fhtml,"err400.txt");
      
            cli->stop();
            }
          else periTableHtml(cli);
}          

void frecupptr(char* libfonct,uint8_t* v,uint8_t* b,uint8_t lenpersw)
{
   //uint8_t switch=*libfonct-48;               // num source
   //uint8_t typend=(*(libfonct+1)-64)/16;      // num type dans la source
   //uint8_t typenb=(*(libfonct+1)-64)%16;      // num bit
   
   *v=((*libfonct-48)*lenpersw+(*(libfonct+1)-MAXSW*16)/16)&0x0f;   // num octet de type
   *b=(*(libfonct+1)-MAXSW*16)%16;                                  // N° de bit dans l'octet de type d'action
}


void test2Switchs()
{

  char ipAddr[16];memset(ipAddr,'\0',16);
  charIp(lastIpAddr,ipAddr);
  for(int x=0;x<4;x++){
Serial.print(x);Serial.print(" test2sw ");Serial.println(ipAddr);
    testSwitch("GET /testb_on__=0006AB8B",ipAddr,80);
    delay(2000);
    testSwitch("GET /testa_on__=0006AB8B",ipAddr,80);
    delay(2000);
    testSwitch("GET /testboff__=0006AB8B",ipAddr,80);
    delay(2000);
    testSwitch("GET /testaoff__=0006AB8B",ipAddr,80);
    delay(2000);
  }
}

void testSwitch(char* command,char* perihost,int periport)
{
            uint8_t fonct;
            
            memset(bufServer,'\0',32);
            memcpy(bufServer,command,24);
   Serial.print(perihost);Serial.print(" ");Serial.println(bufServer);
            int z=messToServer(&cliext,perihost,periport,bufServer);
            Serial.println(z);
            if(z==MESSOK){
              periMess=getHttpResponse(&cliext,bufServer,LBUFSERVER,&fonct);
              Serial.println(periMess);
            }
            purgeServer(&cliext);
}

