
#define _MODE_DEVT    
/* Mode développement */

#include <ESP8266WiFi.h>
#include "const.h"
#include <shconst.h>
#include <shutil.h>
#include <shmess.h>
#include "ds1820.h"
#include "util.h"
#include "dynam.h"

extern "C" {                  
#include <user_interface.h>     // pour struct rst_info, system_deep_sleep_set_option(), rtc_mem
}

#if CONSTANT==EEPROMSAVED
#include <EEPROM.h>
#endif

Ds1820 ds1820;

  char model[LENMODEL];

  long dateon=millis();           // awake start time
  long boucleTime=millis();

  const char* ssid;
  const char* password;
  const char* ssid1= "pinks";
  const char* password1 = "cain ne dormant pas songeait au pied des monts";
  const char* ssid2= "PINKS2";
  const char* password2= "il vit un oeil";
#ifndef _MODE_DEVT
  const char* host = "192.168.0.34";
  const int   port = 1789;
#endif ndef_MODE_DEVT
#ifdef _MODE_DEVT
  const char* host = "192.168.0.35";
  const int   port = 1790;
#endif def_MODE_DEVT

WiFiClient cli;                 // client local du serveur externe (utilisé pour dataread/save)
WiFiServer server(PORTSERVPERI);
WiFiClient cliext;              // client externe du serveur local

  String headerHttp;

  char* srvpswd=SRVPASS;
  byte  lsrvpswd=LENSRVPASS;

// enregistrement pour serveur externe

  char  bufServer[LBUFSERVER];   // buffer des envois/réceptions de messages
  int   periMess;           // diag de réception de message

  char* fonctions={"set_______ack_______etat______reset_____sleep_____testaoff__testa_on__testboff__testb_on__last_fonc_"};
  uint8_t fset_______,fack_______,fetat______,freset_____,fsleep_____,ftestaoff__,ftesta_on__,ftestboff__,ftestb_on__;;
  int     nbfonct;
  uint8_t fonction;      // la dernière fonction reçue

  float temp;
  long  tempTime=-PERTEMP*1000;        // timer température pour mode loop
  char  ageSeconds[8];     // secondes 9999999s=115 jours
  long  tempAge=1;         // secondes
  bool  tempchg=FAUX;

  long ctlTime=millis();    // timer stepper switchs, pulses, détecteurs
  uint8_t ctlStep=0;        // stepper 

  extern uint8_t nbreBlink;
  long  blkTime=millis();
  int   blkPer=2000;
  long  debTime=millis();   // pour mesurer la durée power on
  long  debConv=millis();   // pour attendre la fin du délai de conversion
  int   debPer=TDEBOUNCE;
  int   cntIntA=0;
  int   cntIntB=0;
  long  detTime[MAXDET]={millis(),millis(),millis(),millis()};    // temps pour debounce
  long  detFlTime[MAXDET]={millis(),millis(),millis(),millis()};  // temps pour effacement flancs

  uint8_t pinSw[MAXSW]={PINSWA,PINSWB,PINSWC,PINSWD};    // les switchs
  byte    staPulse[MAXSW];                               // état clock pulses
  uint8_t pinDet[MAXDET]={PINDTA,PINDTB,PINDTC,PINDTD};  // les détecteurs
  byte    pinDir[MAXDET]={LOW,LOW,LOW,LOW};              // flanc pour interruption des détecteurs (0 falling ; 1 rising
  
  void (*isrD[4])(void);                                 // tableau de pointeurs de fonctions

  int   i=0,j=0,k=0;
  uint8_t oldswa[]={0,0,0,0};         // 1 par switch

constantValues cstRec;

char* cstRecA=(char*)&cstRec;

  byte  mac[6];
  char  buf[3]; //={0,0,0};

  int   cntreq=0;
  int   cntdebug[]={0,0,0,0};


  ADC_MODE(ADC_VCC);

  char* chexa="0123456789ABCDEFabcdef\0";

   /* prototypes */

int  talkServer();
void talkClient(char* etat);

int act2sw(int sw1,int sw2);
uint8_t runPulse(uint8_t sw);

void isrDet(uint8_t det);
void isrD0();
void isrD1();
void isrD2();
void isrD3();
void initIntPin(uint8_t det);

bool  wifiConnexion(const char* ssid,const char* password);
int   dataSave();
int   dataRead();
void  dataTransfer(char* data);  
void  readTemp();
void  ordreExt();
void  swAction();

void debug(int cas){
  if(cntdebug[cas]==0 || cas==1){
    Serial.print("debug");Serial.print(cas);Serial.print(" ");
    Serial.print("cstRec.talkStep=");Serial.println(cstRec.talkStep);
  }
   cntdebug[cas]++;
}


void setup() 
{ 

      /* Hardware Init */

#if POWER_MODE==PO_MODE
  digitalWrite(PINPOFF,LOW);
  pinMode(PINPOFF,OUTPUT);
#endif PM==PO_MODE

  pinMode(PINLED,OUTPUT);
#if POWER_MODE==NO_MODE
  digitalWrite(PINSWA,LOW);
  digitalWrite(PINSWB,LOW);
  pinMode(PINSWA,OUTPUT);
  pinMode(PINSWB,OUTPUT);

  pinMode(PINDTA,INPUT_PULLUP);
  pinMode(PININTA,INPUT_PULLUP);
  pinMode(PININTB,INPUT_PULLUP);
  if(digitalRead(PINDTA==0) || digitalRead(PININTA)==0 || (digitalRead(PININTA)!=0 && digitalRead(PININTB)!=0)){cntIntA=1;}

 /* init tableau des fonctions d'interruption */
 
 isrD[0]=isrD0;
 isrD[1]=isrD1;
 isrD[2]=isrD2;
 isrD[3]=isrD3;
          
#endif PM==NO_MODE

/* inits variables */

  model[0]=CARTE;
  model[1]=POWER_MODE;
  model[2]=CONSTANT;
  model[3]='1';
  model[4]=(char)(NBSW+48);
  model[5]=(char)(NBDET+48);  
  
  memset(staPulse,0x00,MAXSW);

  nbfonct=(strstr(fonctions,"last_fonc_")-fonctions)/LENNOM;
  fset_______=(strstr(fonctions,"set_______")-fonctions)/LENNOM;
  fack_______=(strstr(fonctions,"ack_______")-fonctions)/LENNOM;
  fetat______=(strstr(fonctions,"etat______")-fonctions)/LENNOM;
  freset_____=(strstr(fonctions,"reset_____")-fonctions)/LENNOM;
  fsleep_____=(strstr(fonctions,"sleep_____")-fonctions)/LENNOM;  
  ftestaoff__=(strstr(fonctions,"testaoff__")-fonctions)/LENNOM;
  ftesta_on__=(strstr(fonctions,"testa_on__")-fonctions)/LENNOM;
  ftestboff__=(strstr(fonctions,"testboff__")-fonctions)/LENNOM;  
  ftestb_on__=(strstr(fonctions,"testb_on__")-fonctions)/LENNOM;

  //delay(2000);
  Serial.begin(115200);
  delay(100);
  
#ifdef _MODE_DEVT
  Serial.print("\nSlave 8266 _MODE_DEVT");
#endif _MODE_DEVT

  Serial.print("\n\n");Serial.print(VERSION);Serial.print(" power_mode=");Serial.print(POWER_MODE);
  Serial.print(" carte=");Serial.print(CARTE);

#if CONSTANT==EEPROMSAVED
  EEPROM.begin(512);
#endif

cstRec.cstlen=sizeof(cstRec);

 byte setds[4]={0,0x7f,0x80,0x3f},readds[8];   // 187mS 10 bits accu 0,25°
 int v=ds1820.setDs(WPIN,setds,readds);if(v==1){Serial.print(" DS1820 version=0x");Serial.println(readds[0],HEX);}
 else {Serial.print(" DS1820 error ");Serial.println(v);}
#if POWER_MODE==NO_MODE
  ds1820.convertDs(WPIN);
  delay(TCONVERSION);
#endif PM==NO_MODE
#if POWER_MODE==PO_MODE
  ds1820.convertDs(WPIN);
  debConv=millis();
#endif PM==PO_MODE


#if POWER_MODE==DS_MODE
/* si pas sortie de deep sleep faire une conversion 
   et initialiser les variables permanentes */
  rst_info *resetInfo;
  resetInfo = ESP.getResetInfoPtr();
  Serial.print("resetinfo ");Serial.println(resetInfo->reason);
  if((resetInfo->reason)!=5){           // 5 deepsleep awake ; 6 external reset
    ds1820.convertDs(WPIN);
    delay(TCONVERSION);
    initConstant();
    writeConstant();
    }
#endif PM==DS_MODE

#if POWER_MODE!=DS_MODE
/* si le crc des variables permanentes est faux, initialiser
   et lancer une conversion */
  if(!readConstant()){
    Serial.println("initialisation constantes");
    initConstant();
    dumpstr(cstRecA,256);
    writeConstant();
  }
  //cstRec.memDetec!=MEMDINIT;
#endif PM!=DS_MOD

Serial.print("CONSTANT=");Serial.print(CONSTANT);Serial.print(" time=");Serial.print(millis()-debTime);Serial.println(" ready !");
  yield();
//  for(int g=0;g<MAXSW;g++){cstRec.begPulseOne[g]=0;cstRec.begPulseTwo[g]=0;}
  printConstant();
delay(2000);
#if POWER_MODE==NO_MODE
    wifiConnexion(ssid1,password1); 

#ifdef  _SERVER_MODE
    server.begin();
    Serial.println("server-begin");
#endif  def_SERVER_MODE
    
}    // fin setup NO_MODE

void loop(){        //=============================================      

  // la réception de commande est prioritaire sur tout et (si une commande valide est reçue) toute communication
  // en cours est avortée pour traiter la commande reçue.
  // si une communication avec le serveur est en cours (cstRec.talkStep != 0), l'automate talkServer est actif
  // sinon la boucle d'attente tourne : 
  //                                      - contrôle d'état des switchs 
  //                                      - test de l'heure de mesure de température (ou autre)
  //                                      - test de l'heure de blink
  //                                      - debounce switchs
  //                                      - test de l'heure d'appel du serveur  
  // (changer cstRec.talkStep déclenche un transfert au serveur (cx wifi/dataRead/Save)
  // Si l'appel n'aboutit pas (pas de cx wifi, erreurs de com, rejet par le serveur-plus de place)
  // le délai d'appel au serveur est doublé à concurence de 7200sec entre les appels.
  //  
#ifdef  _SERVER_MODE
  ordreExt(); // réception ordre ext ?
#endif def_SERVER_MODE  
  //debug(0);
   
  if(cstRec.talkStep!=0){
    //debug(1);
    talkServer();
    writeConstant();
    }
  else {
#ifdef  _SERVER_MODE
    //debug(2);  
      if(millis()>(ctlTime+PERCTL)){
        switch(ctlStep++){
          case 1:break;
          case 2:swAction();break;
          case 4:
          /*
            Serial.print(0);Serial.print(" ");
            Serial.print(cstRec.pulseCtl[0],HEX);Serial.print(" ");
            Serial.print(staPulse[0],HEX);Serial.print(" ");
            Serial.print(cstRec.begPulseOne[0]);Serial.print("-");Serial.print(cstRec.begPulseTwo[0]);
            Serial.print("| detect=");Serial.print(cstRec.memDetec,HEX);
            Serial.print("/h=");Serial.print((cstRec.pulseCtl[0]>>PMDIH_PB)&0x0001);
            Serial.print("/e=");Serial.print(cstRec.pulseCtl[0]&PMDIE_VB,HEX);
            Serial.print(" num=");Serial.print(pinDet[(cstRec.pulseCtl[0]>>PMDINLS_PB)&0x0003]);
            Serial.print(" ");Serial.println(millis()/1000);
            */
            break;
            
          case 5:break;
          case 6:break;
          case 7:break;
          case 9:ctlStep=0;
          /*
            for(int det=0;det<MAXDET;det++){                          // effacement flancs détecteurs après (au moins) un tour complet 
              if(millis()>detFlTime[det]+PERCTL*10){
                cstRec.memDetec &= ~(DETPRE_VB<<(LENDET*det));        // effacement précédent
                cstRec.memDetec |= (((DETCUR_VB>>(DETCUR_PB+LENDET*det))&0x0001)<<(DETPRE_PB+LENDET*det));  // remplacement
              }
            }
            */
            break;
          default:break;
        }
        ctlTime=millis();
      }
#endif def_SERVER_MODE  
    //debug(3);

    ledblink(0);

/* debounce switchs sur interrupt */
  for(int det=0;det<NBDET;det++){
    if(detTime[det]!=0 && (millis()>(detTime[det]+TDEBOUNCE))){
      detTime[det]=0; 
      initIntPin(det);
    }
  }
#endif PM==NO_MODE

/* tous modes */

    readTemp();
    
#if  POWER_MODE!=NO_MODE
  
  while(cstRec.talkStep!=0){
    Serial.print("   talkStep=");Serial.println(cstRec.talkStep);
    yield();talkServer();}

/* sauvegarde variables permanentes avant sleep ou power off */
  writeConstant();

  Serial.print("durée ");Serial.print(millis());Serial.print(" - ");
  Serial.print(dateon);Serial.print(" = ");Serial.print(millis()-dateon);
  Serial.print("   talkStep=");Serial.println(cstRec.talkStep);
  delay(10); // purge serial


#if POWER_MODE==DS_MODE
  /* deep sleep */
  system_deep_sleep_set_option(4);
  ESP.deepSleep(cstRec.tempPer*1e6); // microseconds
#endif PM==DS_MODE
#if POWER_MODE==PO_MODE
  /* power off */
  digitalWrite(PINPOFF,HIGH);        // power down
  pinMode(PINPOFF,OUTPUT);
#endif PM==PO_MODE

while(1){};

} //  fin setup si != NO_MODE

void loop() {
#endif PM!=NO_MODE

#if POWER_MODE==NO_MODE
  }       // fin de l'automate talkServer
#endif PM==NO_MODE

}  // fin loop pour toutes les options

// ===============================================================================

void fServer(uint8_t fwaited)          // réception du message réponse du serveur pour DataRead/Save;
                                       // contrôles et transfert 
                                       // retour periMess  
{      
        periMess=getHttpResponse(&cli,bufServer,LBUFSERVER,&fonction);
        Serial.print("gHResp() periMess=");Serial.println(periMess);
        if(periMess==MESSOK){
          if(fonction==fwaited){dataTransfer(bufServer);}
          else {periMess=MESSFON;}
        }
}

void dataTransfer(char* data)           // data sur fonction
                                        // transfert params (ou pas) selon contrôles
                                        //    contrôle mac addr et numPeriph ;
                                        //    si pb -> numPeriph="00" et ipAddr=0
                                        //    si ok -> tfr params
                                        // retour periMess
{
  int  ddata=16;                                  // position du numéro de périphérique  
  byte fromServerMac[6];
  byte hh,ll;
  
        periMess=MESSOK;
        packMac(fromServerMac,(char*)(data+ddata+3));
        if(memcmp(data+ddata,"00",2)==0){periMess=MESSNUMP;}
        else if(!compMac(mac,fromServerMac)){periMess=MESSMAC;}
        else {
                             // si ok transfert des données                                    
            strncpy(cstRec.numPeriph,data+MPOSNUMPER,2);                     // num périph
            int sizeRead;
            cstRec.serverPer=(long)convStrToNum(data+MPOSPERREFR,&sizeRead); // per refresh server
            cstRec.tempPitch=(long)convStrToNum(data+MPOSPITCH,&sizeRead);   // pitch mesure
            cstRec.swCde='\0';
            for(int i=0;i<MAXSW;i++){                                             // 1 byte état/cdes serveur + 4 bytes par switch (voir const.h du frontal)
              
              cstRec.swCde |= (*(data+MPOSSWCDE+MAXSW-1-i)-48)<<(2*(i+1)-1);    // bit cde (bits 8,6,4,2)  
              conv_atoh((data+MPOSINTPAR0+i*9),&cstRec.actCde[i]);
              conv_atoh((data+MPOSINTPAR0+i*9+2),&cstRec.desCde[i]);
              conv_atoh((data+MPOSINTPAR0+i*9+4),&cstRec.onCde[i]);
              conv_atoh((data+MPOSINTPAR0+i*9+6),&cstRec.offCde[i]);

              for(int ctl=0;ctl<DLSWLEN;ctl++){
                conv_atoh((data+MPOSPULSCTL+i*(DLSWLEN*2+1)+ctl*2),&cstRec.pulseCtl[i*DLSWLEN+ctl]);}
              
              cstRec.durPulseOne[i]=(long)convStrToNum(data+MPOSPULSON0+i*(MAXSW*2+1),&sizeRead);
              cstRec.durPulseTwo[i]=(long)convStrToNum(data+MPOSPULSOF0+i*(MAXSW*2+1),&sizeRead);
            }
            printConstant();
        }
        if(periMess!=MESSOK){
          memcpy(cstRec.numPeriph,"00",2);cstRec.IpLocal=IPAddress(0,0,0,0);
        }
        Serial.print("dataTransfer() periMess=");Serial.println(periMess);
}


int talkServer()    // si numPeriph est à 0, dataRead pour se faire reconnaitre ; 
                    // si ça fonctionne réponse numPeriph!=0 ; dataSave 
                    // renvoie 0 et periMess valorisé si la com ne s'est pas bien passée.
{
int v=0,retry=0;
//Serial.print(" talkServer talkStep=");Serial.println(cstRec.talkStep);
switch(cstRec.talkStep){
  case 1:
      ssid=ssid1;password=password1;
      cstRec.talkStep=3;
      break;
      
  case 2:
      ssid=ssid2;password=password2; // tentative sur ssid bis
      cstRec.talkStep=3;
      break;

  case 3:    
      while((retry<NBRETRY)&&!wifiConnexion(ssid,password)){delay(1000);retry++;}
      if(retry>=NBRETRY && cstRec.talkStep==1){cstRec.talkStep=2;}
      else if(retry>=NBRETRY && cstRec.talkStep==2){cstRec.talkStep=98;}
      else {
        cstRec.talkStep=4;
        WiFi.macAddress(mac);
        serialPrintMac(mac);}
      break;
      

  case 4:         // connecté au wifi
                  // si le numéro de périphérique est 00 ---> récup (dataread), ctle réponse et maj params
      
      if(memcmp(cstRec.numPeriph,"00",2)==0){
        v=dataRead();Serial.print("dread v=");Serial.println(v);
        if(v==MESSOK){cstRec.talkStep=5;}
        else {cstRec.talkStep=9;}            // pb com -> recommencer au prochain timing
      }  
      else {cstRec.talkStep=6;}              // numPeriph !=0 -> data_save
      break;
        
  case 5:         // gestion réponse au dataRead
  
        fServer(fset_______);  // récupération adr mac, numPériph, tempPer et tempPitch dans bufServer (ctle CRC & adr mac)
                               // le num de périph est mis à 0 si la com ne s'est pas bien passée
        cstRec.talkStep=6;     // si le numéro de périphérique n'est pas 00 ---> ok (datasave), ctle réponse et maj params
        writeConstant();
        break;
      
  case 6:         // si numPeriph !=0 ou réponse au dataread ok -> datasave
                  // sinon recommencer au prochain timing
      
      if(memcmp(cstRec.numPeriph,"00",2)==0){cstRec.talkStep=9;}
      else {  
        v=dataSave();Serial.print("dsave v=");Serial.println(v);
        if(v==MESSOK){cstRec.talkStep=7;}
        else {cstRec.talkStep=99;}
      }
      break;

  case 7:         // gestion réponse au dataSave
                  // si la réponse est ok -> terminer
                  // sinon recommencer au prochain timing
                  
       fServer(fack_______);
       // le num de périph a été mis à 0 si la com ne s'est pas bien passée
       cstRec.talkStep=9;
       break;  
                   // terminé ; si tout s'est bien passé les 2 côtés sont à jour 
                   // sinon numpériph est à 00 et l'adresse IP aussi

  case 9:
       Serial.print(cstRec.numPeriph);purgeServer(&cli);
       cstRec.talkStep=0;
       break;


  case 98:      // pas réussi à connecter au WiFi ; tempo 2h
        cstRec.serverPer=PERTEMPKO;

  case 99:      // mauvaise réponse du serveur ou wifi ko ; raz numPeriph
        memcpy(cstRec.numPeriph,"00",2);
        cstRec.talkStep=0;
        writeConstant();
        break;
        
  default: break;
  }
  
  
}

#ifdef _SERVER_MODE

void ordreExt()
{
  cliext = server.available();

  if (cliext) {
    Serial.println("\nCliext");
    String input = "";                    // buffer ligne
    headerHttp = "";                      // buffer en-tête
    while (cliext.connected()) {
      if (cliext.available()) {
        char c = cliext.read();
        headerHttp+=c;                    //remplissage en-tête
        Serial.write(c);
        if (c == '\n') {                  // LF fin de ligne
            if (input.length() == 0) {    // si ligne vide fin de requête
              //Serial.println("\n 2 fois LF fin de la requête HTTP");
              break;                      // sortie boucle while
            }
            else {input = "";}            // si 1 LF vidage buffer ligne pour recevoir la ligne suivante
        }
        else if(c != '\r'){input+=c;}   // remplissage ligne
      }
    }
    // headerHttp contient la totalité de l'en-tête 
    //
    // format message "GET /FFFFFFFFFF=nnnn....CC" FFFFFFFFFF instruction (ETAT___, SET____ etc)
    //                                             nnnn nombre de car décimal zéros à gauche 
    //                                             .... éventuels arguments de la fonction
    //                                             CC crc
    int v0=headerHttp.indexOf("GET /");
    if(v0>=0){                            // si commande GET trouvée contrôles et décodage nom fonction 
      int jj=4,ii=convStrToNum(&headerHttp[0]+v0+5+10+1,&jj);   // recup eventuelle longueur
      headerHttp[v0+5+10+1+ii+2]=0x00;    // place une fin ; si long invalide check sera invalide
      Serial.print("len=");Serial.print(ii);Serial.print(" ");Serial.println(headerHttp+v0);
    
      if(checkHttpData(&headerHttp[v0+5],&fonction)==MESSOK){
        Serial.print("reçu message fonction=");Serial.println(fonction);
        switch(fonction){
            case 0:dataTransfer(&headerHttp[v0+5]);break;  // set
            case 1:break;                             // ack ne devrait pas se produire (page html seulement)
            case 2: cstRec.talkStep=1;break;          // etat -> dataread/save   http://192.168.0.6:80/etat______=0006AB8B
            case 3:break;                             // sleep (future use)
            case 4:break;                             // reset (future use)
            case 5:digitalWrite(PINSWA,CLOSA);break;  // test off A        http://192.168.0.6:80/testaoff__=0006AB8B
            case 6:digitalWrite(PINSWA,OPENA);break;  // test on  A        http://192.168.0.6:80/testa_on__=0006AB8B
            case 7:digitalWrite(PINSWB,CLOSB);break;  // test off B        http://192.168.0.6:80/testboff__=0006AB8B
            case 8:digitalWrite(PINSWB,OPENB);break;  // test on  B        http://192.168.0.6:80/testb_on__=0006AB8B
            
            default:break;
        }
        char etat[]="done______=0006AB8B\0";
        talkClient(etat);
        Serial.println();
      }
      cntreq++;
      cliext.stop();
      headerHttp="";
    }                     // une éventuelle connexion a été traitée
                          // si controles ko elle est ignorée
    purgeServer(&cliext);
  }
}

void talkClient(char* etat) // réponse à une requête
{
            /* en-tête réponse HTTP */
            cliext.println("HTTP/1.1 200 OK");
            cliext.println("Content-type:text/html");
            cliext.println("Connection: close\n");
            /* page Web */
            cliext.println("<!DOCTYPE html><html>");
            //cliext.println("<head></head>");
            cliext.print("<body>");
            cliext.print(etat);Serial.print(etat);
            cliext.println("</body></html>");
}
#endif def_SERVER_MODE

//***************** utilitaires

int buildReadSave(char* nomfonction,char* data)          //   connecte et transfère (sortie MESSCX connexion échouée)
                                                  //   password__=nnnnpppppp..cc?
                                                  //   data_rs.._=nnnnppmm.mm.mm.mm.mm.mm_[-xx.xx_aaaaaaa_v.vv]_r.r_siiii_diiii_cc
{
  strcpy(bufServer,"GET /cx?\0");
  if(!buildMess("peri_pass_",srvpswd,"?")==MESSOK){
    Serial.print("decap bufServer ");Serial.print(bufServer);Serial.print(" ");Serial.println(srvpswd);return MESSDEC;};

  char message[LENVAL];
  int sb=0,i=0;
  char x[2]={'\0','\0'};
  
      strcpy(message,cstRec.numPeriph);                               // N° périf                    - 3
      memcpy(message+2,"_\0",2);
      sb=3;
      unpackMac((char*)(message+sb),mac);                             // macaddr                    - 18
      strncpy((char*)(message+sb+17),"_\0",2);
      strcat(message,data);strcat(message,"_");                       // temp, âge (dans data_save seul) - 15

      sb=strlen(message);
      sprintf(message+sb,"%1.2f",(float)ESP.getVcc()/1024.00f);       // alim                        - 5
      strncpy((char*)(message+sb+4),"_\0",2);
      strncpy(message+sb+5,VERSION,LENVERSION);                       // VERSION contient le "_"...  - 4
      
      sb+=5+LENVERSION;
      message[sb]=(char)(NBSW+48);                                    // nombre switchs    
      for(i=(NBSW-1);i>=0;i--){message[sb+1+(NBSW-1)-i]=(char)(48+digitalRead(pinSw[i]));}    // état -6
      if(NBSW<MAXSW){for(i=NBSW;i<MAXSW;i++){message[sb+1+i]='x';}}message[sb+5]='_';

      sb+=MAXSW+2;
      message[sb]=(char)(NBDET+48);                                   // nombre détecteurs
      for(i=(NBDET-1);i>=0;i--){message[sb+1+(NBDET-1)-i]=(char)(chexa[cstRec.memDetec[i]]);} // état -6 
      if(NBDET<MAXDET){for(i=NBDET;i<MAXDET;i++){message[sb+1+i]='x';}}                              
      strcpy(message+sb+1+MAXDET,"_\0");

      sb+=MAXDET+2;
      for(i=(MAXSW-1);i>=0;i--){message[sb+(MAXSW-1)-i]=chexa[staPulse[i]];}
      strcpy(message+sb+MAXSW,"_\0");                                 // clock pulse status          - 5

      sb+=MAXSW+1;
      memcpy(message+sb,model,LENMODEL);
      strcpy(message+sb+LENMODEL,"_\0");                                                        //   - 7

if(strlen(message)>LENVAL-4){Serial.print("******* LENVAL ***** MESSAGE ******");ledblink(BCODELENVAL);}      
  
  buildMess(nomfonction,message,"");

  return messToServer(&cli,host,port,bufServer); 
}

int dataSave()
{
  char tempstr[16];
      sprintf(tempstr,"%+02.2f",temp);                                    // 6 car
      if(strstr(tempstr,"nan")!=0){strcpy(tempstr,"+00.00\0");}
      strcat(tempstr,"_");                                                // 1 car
      sprintf((char*)(tempstr+strlen(tempstr)),"%07d",(millis())-dateon); // 7 car
      strcat(tempstr,"\0");                                               // 1 car
      
      buildReadSave("data_save_",tempstr);
}

int dataRead()
{
      buildReadSave("data_read_","_");
}

bool wifiConnexion(const char* ssid,const char* password)
{
  int i=0;
  long beg=millis();
  bool cxok=VRAI;

    ledblink(BCODEONBLINK);
    
  //if(WiFi.status()!=WL_CONNECTED){ 

    //WiFi.forceSleepWake(); //WiFi.forceSleepEnd();       // réveil modem
    //delay(100);
  
    WiFi.begin(ssid,password);
    
    if(cstRec.IpLocal!=IPAddress(0,0,0,0)){
      IPAddress dns(192,168,0,254);
      IPAddress gateway(192,168,0,254);
      IPAddress subnet(255,255,255,0);
      WiFi.config(cstRec.IpLocal, dns, gateway, subnet);
    }
  
    Serial.print(" WIFI connecting to ");Serial.print(ssid);Serial.print("...");
  
    while(WiFi.status()!=WL_CONNECTED){
      delay(100);Serial.print(i++);
      if((millis()-beg)>WIFI_TO_CONNEXION){cxok=FAUX;break;}
      }
  
    if(cxok){
      if(nbreBlink==BCODEWAITWIFI){ledblink(BCODEWAITWIFI+100);}
      Serial.print(" connected ; local IP : ");Serial.println(WiFi.localIP());
      if(cstRec.IpLocal!=WiFi.localIP()){cstRec.IpLocal=WiFi.localIP();writeConstant();}
      
      }
    else {Serial.println("\nfailed");if(nbreBlink==0){ledblink(BCODEWAITWIFI);}}
    //}
  return cxok;
}

void modemsleep()
{
  WiFi.disconnect();
  WiFi.forceSleepBegin();
  delay(100);
}

#if POWER_MODE==NO_MODE 

/* interruptions détecteurs */

void initIntPin(uint8_t det)              // enable interrupt du détecteur det ; flanc selon pinDir ; isr selon isrD
{                                         // setup memDetec
  Serial.print(det);Serial.println(" ********************* initIntPin");
  cstRec.memDetec[det] &= ~DETBITLH_VB;            // raz bit LH
  cstRec.memDetec[det] &= ~DETBITST_VB;            // raz bits ST
  cstRec.memDetec[det] |= DETWAIT<<DETBITST_PB;    // set bits ST (armé)
  if(pinDir[det]==LOW){attachInterrupt(digitalPinToInterrupt(pinDet[det]),isrD[det], FALLING);cstRec.memDetec[det] |= HIGH<<DETBITLH_PB;}
  else {attachInterrupt(digitalPinToInterrupt(pinDet[det]),isrD[det], RISING);cstRec.memDetec[det] |= LOW<<DETBITLH_PB;}
}

void isrD0(){detachInterrupt(pinDet[0]);isrDet(0);}
void isrD1(){detachInterrupt(pinDet[1]);isrDet(1);}
void isrD2(){detachInterrupt(pinDet[2]);isrDet(2);}
void isrD3(){detachInterrupt(pinDet[3]);isrDet(3);}

void isrDet(uint8_t det)      // setup memDetec après interruption sur det
{
  cstRec.memDetec[det] &= ~DETBITLH_VB;             // raz bit LH
  cstRec.memDetec[det] &= ~DETBITST_VB;             // raz bits ST
  cstRec.memDetec[det] |= DETTRIG<<DETBITST_PB;     // set bits ST (déclenché)
  if(pinDir[det]==(byte)HIGH){cstRec.memDetec[det] |= HIGH<<DETBITLH_PB;}
  else {cstRec.memDetec[det] |= LOW<<DETBITLH_PB;}
  pinDir[det]^=HIGH;                                // inversion pinDir pour prochain armement
}


/* pulses ---------------------------- */

uint8_t runPulse(uint8_t sw)                               // get état pulse sw
{
}


/* switchs action -------------------- */

uint8_t rdy(byte modesw,int sw) // pour les 3 sources, check bit enable puis etat source ; retour numsource valorisé si valide sinon 0
{
  /* ------ détecteur --------- */
  if((modesw & SWMDLEN_VB) !=0){                         // det enable
    uint8_t det=(modesw>>SWMDLNULS_PB)&0x03;          // numéro det
    uint64_t swctl;memcpy(&swctl,cstRec.pulseCtl+sw*DLSWLEN,DLSWLEN);
    uint16_t dlctl=(uint16_t)(swctl>>(det*DLBITLEN));
    if(dlctl&DLENA_VB != 0){                          // dl enable
      if(dlctl&DLEL_VB != 0){                         // dl local
        uint8_t locdet=dlctl>>(DLNLS_PB&0x07);        // num dl local 
        if(((modesw>>SWMDLHL_PB)&0x01)==(cstRec.memDetec[locdet]>>DETBITLH_PB)){return 1;}      // ok détecteur local
      }
      else {}                                         // dl externe à traiter
    }
  }                                                                                                              

  /* --------- serveur ---------- */
  if((modesw & SWMSEN_VB) != 0){
//    Serial.print("modesw & SWMSEN_VB != 0   ");Serial.print(((modesw>>SWMSHL_PB)&0x01),HEX);
//    Serial.print("   ");Serial.print(((cstRec.swCde>>((2*sw)+1))&0x01),HEX);Serial.print("    "); Serial.print(modesw,HEX);Serial.print(" ");Serial.println(SWMSEN_VB);
    if (((modesw>>SWMSHL_PB)&0x01)==((cstRec.swCde>>((2*sw)+1))&0x01)){return 2;}    // ok serveur
  }

  /* --------- pulse gen --------- */
  //if(sw==0){Serial.print(" mode=");Serial.print(modesw,HEX);Serial.print(" sta=");Serial.print(staPulse[sw],HEX);Serial.print(" sta&puact=");Serial.print(staPulse[sw]&PUACT,HEX);}
  //if(((modesw&0x02)!=0) && ((staPulse[sw]&PUACT)!=0) && ((modesw&0x01)==(staPulse[sw]&0x01))){return 3;}    // ok timer

  return 0;                                                                     // ko 
}

void swAction()                                                                 // check cde des switchs
{ 
  uint8_t swa=0;
  for(int w=0;w<NBSW;w++){
    
    // action OFF
    swa=rdy(cstRec.offCde[w],w);
//    Serial.print(cstRec.onCde[w],HEX);Serial.print(" serv=");Serial.print(cstRec.swCde,HEX);Serial.print(" ");for(int h=MAXSW;h>0;h--){Serial.print((cstRec.swCde>>(h*2-1))&0x01);}
//    Serial.print(" swa(off)=");Serial.println(swa);
    if(swa!=0){digitalWrite(pinSw[w],OFF);}
    else{

     // action ON 
      swa=rdy(cstRec.onCde[w],w);
//      Serial.print(cstRec.onCde[w],HEX);Serial.print(" serv=");Serial.print(cstRec.swCde,HEX);Serial.print(" ");for(int h=MAXSW;h>0;h--){Serial.print((cstRec.swCde>>(h*2-1))&0x01);}
//      Serial.print(" swa(on)=");Serial.println(swa);
      if(swa!=0){digitalWrite(pinSw[w],ON);swa+=10;}
      else{
        
        /*
      // action desactivation
        swa=rdy(cstRec.desCde[w],w);
        if(swa!=0){swa+=20;}                                               // à traiter
        else{
      
      // action activation  
          swa=rdy(cstRec.actCde[w],w);if(swa!=0){swa+=30;}                 // à traiter
        }
        */
      }   // pas ON
    }     // pas OFF
  }       // switch
}
#endif PM==NO_MODE


/* Read temp ------------------------- */
 
void readTemp()
{
  if(cstRec.talkStep == 0){     // ne peut se produire qu'en NO_MODE 
                                // (les autres modes terminent talkServer avec talkStep=0)
    
#if POWER_MODE==NO_MODE
    if(millis()>(tempTime+cstRec.tempPer*1000) && cstRec.serverPer!=PERTEMPKO){
      tempTime=millis();
      dateon=millis();
#endif PM==NO_MODE

#if POWER_MODE==PO_MODE
      long ms=millis();           // attente éventuelle de la fin de la conversion initiée à l'allumage
      Serial.print("debConv=");Serial.print(debConv);Serial.print(" millis()=");Serial.print(ms);
      Serial.print(" Tconversion=");Serial.print(TCONVERSION);Serial.print(" delay=");Serial.println(TCONVERSION-(ms-debConv));
      if((ms-debConv)<TCONVERSION){
        delay(TCONVERSION-(ms-debConv));}
#endif PM==PO_MODE

      temp=ds1820.readDs(WPIN);
      
#if POWER_MODE!=PO_MODE
      ds1820.convertDs(WPIN);     // conversion pendant deep/sleep ou pendant attente prochain accès
#endif PM!=PO_MODE

      tempAge=millis()/1000;
      Serial.print("temp ");Serial.print(temp);Serial.print(" ");
      Serial.print((float)ESP.getVcc()/1024.00f);Serial.println("V");
      
/* temp (suffisament) changée ? */
      if((int)(temp*10)>(cstRec.oldtemp+cstRec.tempPitch)||(int)(temp*10)<(cstRec.oldtemp-cstRec.tempPitch)){
        cstRec.oldtemp=(int)(temp*10);
        cstRec.talkStep=1;     // temp changée -> talkServer
        cstRec.serverTime=0;  
      }
      
/* avance timer server ------------------- */
      cstRec.serverTime+=cstRec.tempPer;
      if(cstRec.serverTime>cstRec.serverPer){
        cstRec.serverTime=0;
        //cstRec.tempPer=PERTEMP;
        cstRec.talkStep=1;
      }

#if POWER_MODE==NO_MODE
    }   // test boucle tempTime
#endif PM==NO_MODE
  }     // talkStep
}
