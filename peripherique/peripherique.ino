
#define _MODE_DEVT    
/* Mode développement */

#include <ESP8266WiFi.h>
#include "const.h"
#include <shconst.h>
#include <shutil.h>
#include <shmess.h>
#include "ds1820.h"
#include "util.h"

extern "C" {                  
#include <user_interface.h>     // pour struct rst_info, system_deep_sleep_set_option(), rtc_mem
}

#if CONSTANT==EEPROMSAVED
#include <EEPROM.h>
#endif


Ds1820 ds1820;

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
WiFiServer server(80);
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

  long swActTime=millis();

  long  blkTime=millis();
  int   blkPer=2000;
  long  debTime=millis();   // pour mesurer la durée power on
  long  debConv=millis();   // pour attendre la fin du délai de conversion
  int   debPer=TDEBOUNCE;
  int   cntIntA=0;
  int   cntIntB=0;

  uint8_t pinsw[4]={PINSWA,PINSWB,PINSWC,PINSWD};   // les switchs
  uint8_t pindet[4]={PINDTA,PINDTB,PINDTC,PINDTD};  // les détecteurs

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

void isrBascA();
void isrBascB();
void initIntPin();

bool  wifiConnexion(const char* ssid,const char* password);
int   dataSave();
int   dataRead();  
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
  ds1820.convertDs(WPIN);
  debConv=millis();
#endif PM==PO_MODE

#if POWER_MODE==NO_MODE
  ds1820.convertDs(WPIN);
  delay(TCONVERSION);
#endif PM==NO_MODE

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
#endif PM==NO_MODE

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
  Serial.print("\n\n");Serial.print(VERSION);Serial.print(" power_mode=");Serial.print(POWER_MODE);
  Serial.print(" carte=");Serial.println(CARTE);
#ifdef _MODE_DEVT
  Serial.println("Slave 8266 _MODE_DEVT");
#endif _MODE_DEVT
  delay(100);

#if CONSTANT==EEPROMSAVED
  EEPROM.begin(512);
#endif

cstRec.cstlen=sizeof(cstRec);

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
    }
#endif PM==DS_MODE

#if POWER_MODE!=DS_MODE
/* si le crc des variables permanentes est faux, initialiser
   et lancer une conversion */
  if(!readConstant()){
    Serial.println("initialisation constantes");
    initConstant();
  }
Serial.print("ready !DS ; CONSTANT=");Serial.print(CONSTANT);Serial.print(" time=");Serial.println(millis()-debTime);
#endif PM!=DS_MOD
  yield();
  printConstant();

#if POWER_MODE==NO_MODE
    /* connexion au wifi (clignotement led 50/50 en attente) */
    while(!wifiConnexion(ssid1,password1)){ledblink(BCODEWAITWIFI);delay(FASTBLINK);} // delay(500);digitalWrite(PINLED,!digitalRead(PINLED));} 
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
  debug(0);
   
  if(cstRec.talkStep!=0){
    debug(1);
    talkServer();
    writeConstant();
    }
  else {
#ifdef  _SERVER_MODE
  debug(2);  
  if(millis()>(swActTime+PERACT)){swAction();swActTime=millis();}
#endif def_SERVER_MODE  
  debug(3);

  ledblink(0);

/* debounce switchs sur interrupt */
  if(millis()>debTime+debPer && debPer!=0){
    initIntPin();
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
            cstRec.intCde='\0';
            for(int i=0;i<MAXSW;i++){                                             // 1 byte état/cdes serveur + 4 bytes par switch (voir const.h du frontal)
              cstRec.intCde |= (*(data+MPOSINTCDE+MAXSW-1-i)-48)<<(2*(i+1)-1);    // bit cde (bits 8,6,4,2)  
              conv_atoh((data+MPOSINTPAR0+i*9),&cstRec.actCde[i]);
              conv_atoh((data+MPOSINTPAR0+i*9+2),&cstRec.desCde[i]);
              conv_atoh((data+MPOSINTPAR0+i*9+4),&cstRec.onCde[i]);
              conv_atoh((data+MPOSINTPAR0+i*9+6),&cstRec.offCde[i]);
              cstRec.pulseMode[i]=(*(data+MPOSPULSMOD+i*2)-48)<<4 | (*(data+MPOSPULSMOD+i*2+1)-48)&0x0F;
              cstRec.intIPulse[i]=(long)convStrToNum(data+MPOSPULSON0+i*9,&sizeRead);  
              cstRec.intOPulse[i]=(long)convStrToNum(data+MPOSPULSOF0+i*9,&sizeRead);
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
      else if(retry>=NBRETRY && cstRec.talkStep==2){cstRec.talkStep=99;}
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
        else {cstRec.talkStep=9;}
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

  case 99:      // pas réussi à connecter au WiFi ; tempo 2h
        cstRec.serverPer=PERTEMPKO;
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
  
      strcpy(message,cstRec.numPeriph);                              // N° périf
      memcpy(message+2,"_\0",2);
      sb=3;
      unpackMac((char*)(message+sb),mac);                             // macaddr
      strncpy((char*)(message+sb+17),"_\0",2);
      strcat(message,data);strcat(message,"_");                       // temp, âge (dans data_save seul)

      sb=strlen(message);
      sprintf(message+sb,"%1.2f",(float)ESP.getVcc()/1024.00f);       // alim
      strncpy((char*)(message+sb+4),"_\0",2);
      strcat(message,VERSION);  // VERSION contient le "_"...         // version

      sb=strlen(message);
      message[sb]=(char)(NBSW+48);                                    // nombre switchs    
      for(i=(NBSW-1);i>=0;i--){message[sb+1+(NBSW-1)-i]=(char)(48+digitalRead(pinsw[i]));}    // état
      if(NBSW<MAXSW){for(i=NBSW;i<MAXSW;i++){message[sb+1+i]='x';}}message[sb+5]='_';

      sb+=MAXSW+2;
      message[sb]=(char)(NBDET+48);                                   // nombre détecteurs    
      for(i=(NBDET-1);i>=0;i--){message[sb+1+(NBDET-1)-i]=(char)(48+digitalRead(pindet[i]));}  // état 
      if(NBDET<MAXDET){for(i=NBDET;i<MAXDET;i++){message[sb+1+i]='x';}}message[sb+5]='_';
      strcpy(message+sb+1+MAXDET,"_\0");
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
      Serial.println("connected");
      Serial.print("local IP : ");Serial.println(WiFi.localIP());
      if(cstRec.IpLocal!=WiFi.localIP()){cstRec.IpLocal=WiFi.localIP();writeConstant();}
      
      }
    else {Serial.println("\nfailed");}
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
void initIntPin()
{
  if(cntIntA<=cntIntB){attachInterrupt(digitalPinToInterrupt(PININTA), isrBascA, FALLING);}
  else {attachInterrupt(digitalPinToInterrupt(PININTB), isrBascB, FALLING);}
  debPer=0;
}

int act2sw(int sw1,int sw2)
{
  if(sw1>=0){digitalWrite(PINSWA,CLOSA);digitalWrite(PINSWB,CLOSB);return 0;}      // 1-0 "opened";
  else if(sw2>=0){digitalWrite(PINSWA,OPENA);digitalWrite(PINSWB,OPENB);return 1;} // 0-1 "closed";
  else {digitalWrite(PINSWB,digitalRead(PINSWA));return 2;}       // off state
}

void isrBascA()
{
 // interruptions inactives le temps du debounce
  detachInterrupt(PININTA);
  //detachInterrupt(PININTB);
  act2sw(0,-1);
  cntIntA++;
  debTime=millis();debPer=TDEBOUNCE;
}

void isrBascB()
{
 // interruptions inactives le temps du debounce
  //detachInterrupt(PININTA);
  detachInterrupt(PININTB);
  act2sw(-1,0);
  cntIntB++;
  debTime=millis();debPer=TDEBOUNCE;
}

/* switchs action -------------------- */

uint8_t rdy(byte modesw,int w) // pour les 3 sources, check bit enable puis etat source ; retour numsource valorisé si valide sinon 0
{
  if(modesw&0x20 !=0){
    if((modesw>>4)&0x01==digitalRead(pindet[((modesw>>6)&0x03)])){return 1;}   // détecteur 
  }
//  Serial.print(w);Serial.print(" modesw=");Serial.print(modesw,HEX);Serial.print(" ");Serial.print(cstRec.intCde,HEX);Serial.print(" ");
//  Serial.print(modesw&0x08);Serial.print(" ");Serial.print((modesw>>2)&0x01);Serial.print(" ");Serial.println(cstRec.intCde>>((w+1)*2-1)&0x01);
  
  if((modesw & 0x08) != 0){
    if (((modesw>>2)&0x01)==((cstRec.intCde>>((w+1)*2-1))&0x01)){return 2;}       // serveur
  }

//if(((modesw>>2)&0x02)!=0 && (modesw>>2)&0x01==pulsestatus(){return 3;}                      // timer à traiter

  return 0;
}

void swAction()                                                            // check cétat des switchs à changer
{ 
  uint8_t swa=0;
  for(int w=0;w<NBSW;w++){
    swa=rdy(cstRec.offCde[w],w);
//    Serial.print(cstRec.onCde[w],HEX);Serial.print(" serv=");Serial.print(cstRec.intCde,HEX);Serial.print(" ");for(int h=MAXSW;h>0;h--){Serial.print((cstRec.intCde>>(h*2-1))&0x01);}
//    Serial.print(" swa(off)=");Serial.println(swa);
    if(swa!=0){digitalWrite(pinsw[w],OFF);}
    else{
      swa=rdy(cstRec.onCde[w],w);
//      Serial.print(cstRec.onCde[w],HEX);Serial.print(" serv=");Serial.print(cstRec.intCde,HEX);Serial.print(" ");for(int h=MAXSW;h>0;h--){Serial.print((cstRec.intCde>>(h*2-1))&0x01);}
//      Serial.print(" swa(on)=");Serial.println(swa);
      if(swa!=0){digitalWrite(pinsw[w],ON);swa+=10;}
      else{
        swa=rdy(cstRec.desCde[w],w);
        //Serial.print(" swa(des)=");Serial.println(swa);
        if(swa!=0){swa+=20;}                                               // à traiter
        else{
          swa=rdy(cstRec.actCde[w],w);if(swa!=0){swa+=30;}                 // à traiter
          //Serial.print(" swa(act)=");Serial.println(swa);
        }
      }
    }
    //if(swa!=oldswa[w]){Serial.print("+++++++++++++++++swAction=");Serial.print(w);Serial.print(" ");Serial.println(swa);oldswa[w]=swa;}
  }
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
