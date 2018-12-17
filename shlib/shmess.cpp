
#include "Arduino.h"
#include "shutil.h"
#include "shconst.h"

//#define PERIF

#ifdef PERIF
#include <ESP8266WiFi.h>

extern WiFiClient cli;                 // client local du serveur externe
extern WiFiClient cliext;              // client externe du serveur local

#if CONSTANT==RTCSAVED
extern int cstlen;
#endif
#endif // PERIF

#ifndef PERIF
#include <Ethernet.h> //bibliothèque W5100 Ethernet

extern EthernetClient cli;

extern int16_t*  periNum;                      // ptr ds buffer : Numéro du périphérique courant
extern int32_t*  periPerRefr;                  // ptr ds buffer : période datasave minimale
extern float*    periPitch;                    // ptr ds buffer : variation minimale de température pour datasave
extern float*    periLastVal;                  // ptr ds buffer : dernière valeur de température
extern float*    periAlim;                     // ptr ds buffer : dernière tension d'alimentation
extern char*     periLastDateIn;               // ptr ds buffer : date/heure de dernière réception
extern char*     periLastDateOut;              // ptr ds buffer : date/heure de dernier envoi
extern char*     periLastDateErr;              // ptr ds buffer : date/heure de derniere anomalie com
extern int8_t*   periErr;                      // ptr ds buffer : code diag anomalie com (voir MESSxxx shconst.h)
extern char*     periNamer;                    // ptr ds buffer : description périphérique
extern char*     periVers;                     // ptr ds buffer : version logiciel du périphérique
extern byte*     periMacr;                     // ptr ds buffer : mac address
extern byte*     periIpAddr;                   // ptr ds buffer : Ip address
extern byte*     periIntNb;                    // ptr ds buffer : Nbre d'interrupteurs (0 aucun ; maxi 4(MAXSW)
extern byte*     periIntVal;                   // ptr ds buffer : état/cde des inter
extern byte*     periIntMode;                  // ptr ds buffer : Mode fonctionnement inters (MAXTAC=4 par switch)
extern uint32_t* periIntPulseOn;               // ptr ds buffer : durée pulses sec ON (0 pas de pulse)
extern uint32_t* periIntPulseOff;              // ptr ds buffer : durée pulses sec OFF(mode astable)
extern uint32_t* periIntPulseCurrOn;           // ptr ds buffer : temps courant pulses ON
extern uint32_t* periIntPulseCurrOff;          // ptr ds buffer : temps courant pulses OFF
extern uint16_t* periIntPulseMode;             // ptr ds buffer : mode pulses
extern uint8_t*  periSondeNb;                  // ptr ds buffer : nbre sonde
extern boolean*  periProg;                     // ptr ds buffer : flag "programmable"
extern byte*     periDetNb;                    // ptr ds buffer : Nbre de détecteurs maxi 4 (MAXDET)
extern byte*     periDetVal;                   // ptr ds buffer : flag "ON/OFF" si détecteur (2 bits par détec))
extern int8_t    periMess;                     // code diag réception message (voir MESSxxx shconst.h)
extern byte      periMacBuf[6];

#endif // PERIF


extern char*  fonctions;
extern int    nbfonct;
extern char   bufServer[LBUFSERVER];

extern byte   mac[6];

char*         periText={TEXTMESS};

#ifndef PERIF
void purgeServer(EthernetClient* cli)
#endif // PERIF
#ifdef PERIF
void purgeServer(WiFiClient* cli)
#endif // PERIF
{
    Serial.print(" purge ");
    while (cli->available()){Serial.print(cli->read());delay(1);}
    cli->stop();
    Serial.println();
}


int buildMess(char* fonction,char* data,char* sep)   // concatène un message dans bufServer
{                                                    // retourne la longueur totale dans bufServer ou 0 si ovf

      if((strlen(bufServer)+strlen(data)+11+5+2+1)>LBUFSERVER)
        {return 0;}
      strcat(bufServer,fonction);
      strcat(bufServer,"=");
      int sb=strlen(bufServer);
      int d=strlen(data)+5;    //+strlen(sep);
      sprintf(bufServer+sb,"%04d",d);
      strcat(bufServer+sb+4,"_");
      strcat(bufServer+sb+5,data);
      setcrc(bufServer+sb,d);
      strcat(bufServer,sep);
      Serial.print(" sb=");Serial.print(sb);Serial.print(" bs=");Serial.println(bufServer);
      return strlen(bufServer);
}

#ifndef PERIF
int messToServer(EthernetClient* cli,const char* host,int port,char* data)    // connecte au serveur et transfère la data
#endif
#ifdef PERIF
int messToServer(WiFiClient* cli,const char* host,const int port,char* data)    // connecte au serveur et transfère la data
#endif
{
  byte crc;
  int v;

    Serial.print("connexion serveur ");
    Serial.print(host);Serial.print("/");Serial.print(port);
    Serial.print("...");
    if(!cli->connect(host,port)){
      Serial.println(" échouée");v=MESSCX;}
    else{
      Serial.println(" ok");

      cli->print(data);
      //Serial.println(data);
      cli->print("\r\n");
      cli->print(" HTTP/1.1\r\n Connection:close\r\n\r\n");
      v=MESSOK;
    }
    return v;
}

#ifndef PERIF
int waitRefCli(EthernetClient* cli,char* ref,int lref,char* buf,int lbuf)   // attente d'un chaine spécifique dans le flot
#endif PERIF
#ifdef PERIF
int waitRefCli(WiFiClient* cli,char* ref,int lref,char* buf,int lbuf)       // attente d'un chaine spécifique dans le flot
#endif PERIF
// wait for ref,lref    si lbuf<>0 accumule le flot dans buf (ref incluse)
// sortie MESSTO (0) time out   MESSDEC (-1) décap     MESSOK (1) OK
{
  boolean termine=FAUX;
  int ptref=0,ptbuf=0;
  char inch;
  long timerTo=millis()+TOINCHCLI;


      Serial.print("attente =");Serial.print(ref);Serial.print(" ");Serial.println(lref);
      while(!termine){
        if(cli->available()>0){
          timerTo=millis()+TOINCHCLI;
          inch=cli->read();Serial.print(inch);
          if(lbuf!=0){
            if(ptbuf<lbuf){buf[ptbuf]=inch;ptbuf++;}else {return MESSDEC;}}
          if(inch==ref[ptref]){
            ptref++;if(ptref>=lref){termine=VRAI;}}
          else{ptref=0;}
        }
        else if(millis()>=timerTo){return MESSTO;}
      }
      if(lbuf!=0){buf[ptbuf-lref]='\0';}

      return MESSOK;
}


int checkData(char* data)            // controle la structure des données d'un message (longueur,crc)
{                                    // renvoie MESSOK (1) OK ; MESSCRC (-2) CRC ; MESSLEN (-3)  le message d'erreur est valorisé

  int i=4;
  int ii=convStrToNum(data,&i);//Serial.print("len=");Serial.print(ii);Serial.print(" strlen=");Serial.println(strlen(data));
  uint8_t c=0;
  conv_atoh(data+ii,&c);

  if(ii!=strlen(data)-2){i=MESSLEN;}
/*  Serial.print("CRC, c, lenin  =");Serial.print(calcCrc(valf,lenin-2),HEX);Serial.print(", ");
  Serial.print(c,HEX);Serial.print(" , ");Serial.println(lenin);
*/
  else if(calcCrc(data,ii)!=c){i=MESSCRC;}
  else i=MESSOK;
  Serial.print("\nlen/crc calc ");
  Serial.print(strlen(data));Serial.print("/");Serial.print(calcCrc(data,ii),HEX);
  Serial.print(" Checkdata=");Serial.println(i);
  return i;
}

int subData(char* data,uint8_t* fonction)   // extraction de la fonction et checkData
{
    char noms[LENNOM+1];
    int q=0;

    strncpy(noms,data,LENNOM);noms[LENNOM]='\0';                       //for(i=0;i<LENNOM;i++){noms[i]=data[i];}noms[LENNOM]='\0';
    q=checkData(data+LENNOM+1);
    if(q==MESSOK){
        *fonction=(strstr(fonctions,noms)-fonctions)/LENNOM;
        if(*fonction>=nbfonct || *fonction<0){q=MESSFON;}
    }
    return q;
}

#ifndef PERIF
int getServerResponse(EthernetClient* cli, char* data,int lmax,uint8_t* fonction)  // attend un message d'un serveur ; ctle longueur et crc
#endif  PERIF
#ifdef PERIF
int getServerResponse(WiFiClient* cli, char* data,int lmax,uint8_t* fonction)      // attend un message d'un serveur ; ctle longueur et crc
#endif  PERIF
// format <body>contenu...</body></html>\n\r                        la fonction est décodée et les données sont chargées
// contenu fonction__=nnnn_datacrc                                  renvoie les codes "MESSxxx"
{
  char* body="<body>\0";
  char* bodyend="</body>\0";
  int q,v=0;
  uint8_t crc=0;

  q=waitRefCli(cli,body,strlen(body),bufServer,0);
  if(q==MESSOK){q=waitRefCli(cli,bodyend,strlen(bodyend),data,lmax-strlen(bodyend));}
  if(q==MESSOK){
    q=subData(data,fonction);
  }
  return q;
}

char* periDiag(uint8_t diag)
{
  int v=0;
  if(diag>NBMESS){diag=MESSSYS;}
  if(diag!=MESSOK){v=LPERIMESS*(diag+1);}
  return periText+v;
}

void assySet(char* message,int periCur,char* diag,char* date14)
{
#define MSETPERICUR 0
#ifndef PERIF
  sprintf(message,"%02i",periCur);message[2]='\0';periMess=MESSOK;
  strcat(message,"_");
#endif  ndef PERIF
#define MSETMAC     3
#ifndef PERIF
            if(periCur>0){                                          // si periCur >0 macaddr ok -> set
                unpackMac(message+strlen(message),periMacr);}

            else if(periCur<=0){
                  unpackMac(message+strlen(message),periMacBuf);    // si periCur>=0 plus de place -> ack
                  strcat(message,periDiag(periMess));}

            strcat(message,"_");
#endif  ndef PERIF
#define MSETDATEHEURE 21
#ifndef PERIF
            memcpy(message+strlen(message),date14,14);
            strcat(message,"_");

#endif  ndef PERIF
#define MSETPERREFR 36
#ifndef PERIF
            long v1=0;
            if(periCur!=0){                                         // periCur!=0 tfr params
                v1=*periPerRefr;
                sprintf((message+strlen(message)),"%05d",v1);       // periPerRefr
                strcat(message,"_");
#endif  ndef PERIF
#define MSETPITCH   42
#ifndef PERIF
                v1=*periPitch*100;
                sprintf((message+strlen(message)),"%04d",v1);      // periPitch
                strcat(message,"_");

#endif  ndef PERIF
#define MSETINTCDE  47
#ifndef PERIF
                v1=strlen(message);                                 // 4 bits commande (8,6,4,2)
                for(int k=MAXSW;k>0;k--){
                    byte a=*periIntVal;
                    //Serial.print("*peri==");Serial.println(a,HEX);
                    message[v1+MAXSW-k]=(char)( 48+ ((a>>((k*2)-1)) &0x01) );
                }      // periInt (cdes)
                memcpy(message+v1+MAXSW,"_\0",2);

#endif  ndef PERIF
#define MSETINTPAR  52
#ifndef PERIF
                v1+=MAXSW+1;
                for(int k=0;k<MAXSW;k++){            // 4 bytes contrÔle sw (hh*4)+1 sép /sw
                    for(int p=0;p<MAXTAC;p++){
                        //Serial.println(*(periIntMode+k*4+p),HEX);
                        conv_htoa(message+v1+k*(2*MAXTAC+1)+p*2,periIntMode+k*MAXTAC+p);
                        }
                    memcpy(message+v1+(k+1)*2*MAXTAC+k,"_\0",2);
                }
                //message[v1+MAXSW+MAXSW*9+9]='_';

#endif  ndef PERIF
#define MSETINTPULS 52+36=88
#ifndef PERIF
                v1+=MAXSW*(2*MAXTAC+1);            // len déjà copiée
                for(int k=0;k<MAXSW*2;k++){                  // 2 compteurs/sw
                    sprintf(message+v1+k*(8+1),"%08u",*(periIntPulseOn+k));
                    memcpy(message+v1+(k+1)*8+k,"_\0",2);
                }
Serial.println(message);
                v1+=MAXSW*2*(8+1);
                for(int k=0;k<MAXSW;k++){           // 2 bytes pulse Mode (hh*2) /sw
                conv_htoa(message+v1,(byte*)periIntPulseMode+1+k*3);
                conv_htoa(message+v1+2,(byte*)periIntPulseMode+k*3);
                memcpy(message+v1+4,"_\0",2);
                }
            }  // pericur != 0
#endif  ndef PERIF
#define MSETDIAG    88+4*2*9+4*5                         //  =180

            strcat(message,diag);                         // periMess

#define MSETLEN     MSETDIAG+LPERIMESS                    //  +5+1=169
        //Serial.println(message);Serial.println(*periIntPulseMode,HEX);

                                    #if MSETLEN >= LENMESS
                                    message trop petit
                                    #endif
}
