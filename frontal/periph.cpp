#include <Arduino.h>
#include <SD.h>
#include "const.h"
#include <Wire.h>
#include "utilether.h"
#include "shconst.h"
#include "shutil.h"
#include "periph.h"

/* >>>>>>>> fichier config <<<<<<< */

File fconfig;     // fichier config

extern uint16_t perrefr;

extern char configRec[CONFIGRECLEN];
  
extern byte* mac;
extern byte* localIp;
extern int*  portserver;
extern char* nomserver;
extern char* usrpass;
extern char* modpass;
extern char* peripass;
extern char* ssid;   
extern char* passssid;
extern int*  nbssid;

extern byte* configBegOfRecord;
extern byte* configEndOfRecord;

byte lip[]=LOCALSERVERIP;

/* >>>>>>> fichier périphériques <<<<<<<  */

File fperi;       // fichiers perif

extern char      periRec[PERIRECLEN];          // 1er buffer de l'enregistrement de périphérique
extern char      periCache[PERIRECLEN*NBPERIF];   // cache des périphériques
extern byte      periCacheStatus[NBPERIF];     // indicateur de validité du cache d'un périph
  
extern int       periCur;                      // Numéro du périphérique courant

extern int16_t*  periNum;                      // ptr ds buffer : Numéro du périphérique courant
extern int32_t*  periPerRefr;                  // ptr ds buffer : période datasave minimale
extern uint16_t* periPerTemp;                    // ptr ds buffer : période de lecture tempèrature
extern float*    periPitch;                    // ptr ds buffer : variation minimale de température pour datasave
extern float*    periLastVal;                  // ptr ds buffer : dernière valeur de température  
extern float*    periAlim;                     // ptr ds buffer : dernière tension d'alimentation
extern char*     periLastDateIn;               // ptr ds buffer : date/heure de dernière réception
extern char*     periLastDateOut;              // ptr ds buffer : date/heure de dernier envoi  
extern char*     periLastDateErr;              // ptr ds buffer : date/heure de derniere anomalie com
extern int8_t*   periErr;                      // ptr ds buffer : code diag anomalie com (voir MESSxxx shconst.h)
extern char*     periNamer;                    // ptr ds buffer : description périphérique
extern char*     periVers;                     // ptr ds buffer : version logiciel du périphérique
extern char*     periModel;                    // ptr ds buffer : model du périphérique
extern byte*     periMacr;                     // ptr ds buffer : mac address 
extern byte*     periIpAddr;                   // ptr ds buffer : Ip address
extern byte*     periSwNb;                     // ptr ds buffer : Nbre d'interrupteurs (0 aucun ; maxi 4(MAXSW)            
extern byte*     periSwVal;                    // ptr ds buffer : état/cde des inter  
extern byte*     periSwMode;                   // ptr ds buffer : Mode fonctionnement inters (1 par switch)           
extern uint32_t* periSwPulseOne;               // ptr ds buffer : durée pulses sec ON (0 pas de pulse)
extern uint32_t* periSwPulseTwo;               // ptr ds buffer : durée pulses sec OFF(mode astable)
extern uint32_t* periSwPulseCurrOne;           // ptr ds buffer : temps courant pulses ON
extern uint32_t* periSwPulseCurrTwo;           // ptr ds buffer : temps courant pulses OFF
extern byte*     periSwPulseCtl;               // ptr ds buffer : mode pulses
extern byte*     periSwPulseSta;               // ptr ds buffer : état clock pulses
extern uint8_t*  periSondeNb;                  // ptr ds buffer : nbre sonde
extern boolean*  periProg;                     // ptr ds buffer : flag "programmable" (périphériques serveurs)
extern byte*     periDetNb;                    // ptr ds buffer : Nbre de détecteurs maxi 4 (MAXDET)
extern byte*     periDetVal;                   // ptr ds buffer : flag "ON/OFF" si détecteur (2 bits par détec))
extern float*    periThOffset;                 // ptr ds buffer : offset correctif sur mesure température
extern float*    periThmin;                    // ptr ds buffer : alarme mini th
extern float*    periThmax;                    // ptr ds buffer : alarme maxi th
extern float*    periVmin;                     // ptr ds buffer : alarme mini volts
extern float*    periVmax;                     // ptr ds buffer : alarme maxi volts
extern byte*     periDetServEn;                // ptr ds buffer : 1 byte 8*enable detecteurs serveur
      
extern byte*     periBegOfRecord;
extern byte*     periEndOfRecord;

extern int8_t    periMess;                     // code diag réception message (voir MESSxxx shconst.h)
extern byte      periMacBuf[6]; 

extern byte      lastIpAddr[4];

extern char strdate[33];
extern char temp[3],temp0[3],humid[3];

byte decToBcd(byte val)
{
  return( (val/10*16) + (val%10) );
}

byte bcdToDec(byte val)
{
  return( (val/16*10) + (val%16) );
}

void setDS3231time(byte second, byte minute, byte hour, 
    byte dayOfWeek,byte dayOfMonth, byte month, byte year)
{
  Wire.beginTransmission(DS3231_I2C_ADDRESS);
  Wire.write(0); // set next input to start at the seconds register
  Wire.write(decToBcd(second)); // set seconds
  Wire.write(decToBcd(minute)); // set minutes
  Wire.write(decToBcd(hour)); // set hours
  Wire.write(decToBcd(dayOfWeek)); // set day of week (1=Sunday, 7=Saturday)
  Wire.write(decToBcd(dayOfMonth)); // set date (1 to 31)
  Wire.write(decToBcd(month)); // set month
  Wire.write(decToBcd(year)); // set year (0 to 99)
  Wire.endTransmission();
}


void readDS3231time(byte *second,byte *minute,byte *hour,
    byte *dayOfWeek,byte *dayOfMonth,byte *month,byte *year)
{
  Wire.beginTransmission(DS3231_I2C_ADDRESS);
  Wire.write(0); // set DS3231 register pointer to 00h
  Wire.endTransmission();
  Wire.requestFrom(DS3231_I2C_ADDRESS, 7);
  // request seven bytes of data from DS3231 starting from register 00h
  *second = bcdToDec(Wire.read() & 0x7f);
  *minute = bcdToDec(Wire.read());
  *hour = bcdToDec(Wire.read() & 0x3f);
  *dayOfWeek = bcdToDec(Wire.read());
  *dayOfMonth = bcdToDec(Wire.read());
  *month = bcdToDec(Wire.read());
  *year = bcdToDec(Wire.read());
}

Ymdhms now()
{
  Ymdhms ndt;
  uint8_t dayOfWeek;
  readDS3231time(&ndt.second,&ndt.minute,&ndt.hour,&dayOfWeek,&ndt.day,&ndt.month,&ndt.year);
  //Serial.print("DS3231=");Serial.print(ndt.year);Serial.print(ndt.month);Serial.println(ndt.day);
  return ndt;
}

void readDS3231temp(byte* msb,byte* lsb)
{
  Wire.beginTransmission(DS3231_I2C_ADDRESS);
  Wire.write(17); // set DS3231 register pointer to 11h
  Wire.endTransmission();
  Wire.requestFrom(DS3231_I2C_ADDRESS, 2);
  // request two bytes of data from DS3231 starting from register 11h
  *msb = Wire.read();
  *lsb = Wire.read();
  //Serial.print(*msb);Serial.print(":");Serial.println(*lsb);
  switch(*lsb)
  {
    case 0:break;
    case 64: *lsb=25;break;
    case 128:*lsb=50;break;
    case 192:*lsb=75;break;
    default: *lsb=1;break; // error 
  }
}

void getdate(uint32_t* hms2,uint32_t* amj2,byte* js)
{
  char* days={"SunMonTueWedThuFriSat"};
  char* months={"JanFebMarAprMayJunJulAugSepOctNovDec"};
  int i=0;
  byte seconde,minute,heure,joursemaine,jour,mois,annee,msb,lsb; // numérique DS3231
  char buf[8];for(byte i=0;i<8;i++){buf[i]=0;} 
  readDS3231time(&seconde,&minute,&heure,js,&jour,&mois,&annee);
  *hms2=(long)(heure)*10000+(long)minute*100+(long)seconde;*amj2=(long)(annee+2000)*10000+(long)mois*100+(long)jour;
  memset(strdate,0x00,sizeof(strdate));
  strncpy(strdate,days+(*(js)-1)*3,3);strcat(strdate,", ");sprintf(buf,"%u",(byte)jour);strcat(strdate,buf);
  strcat(strdate," ");strncat(strdate,months+(mois-1)*3,3);strcat(strdate," ");sprintf(buf,"%u",annee+2000);strcat(strdate,buf);
  strcat(strdate," ");sprintf(buf,"%.2u",heure);strcat(strdate,buf);strcat(strdate,":");sprintf(buf,"%.2u",minute);
  strcat(strdate,buf);strcat(strdate,":");sprintf(buf,"%02u",seconde);strcat(strdate,buf);strcat(strdate," GMT");
}

void alphaNow(char* buff)
{
  Ymdhms ndt=now();
  
  sprintf(buff,"%.8lu",(long)(ndt.year+2000)*10000+(long)ndt.month*100+(long)ndt.day);
  sprintf((buff+8),"%.2u",ndt.hour);sprintf((buff+10),"%.2u",ndt.minute);sprintf((buff+12),"%.2u",ndt.second);
  buff[14]='\0';
  //Serial.print("alphaNow ");Serial.print(buff);Serial.print(" ");Serial.print(ndt.hour);Serial.print(" ");Serial.print(ndt.minute);Serial.print(" ");Serial.println(ndt.second);
  //Serial.println();
}

/* >>>>>>>>> configuration <<<<<<<<<< */

void configInit()
{
byte* temp=(byte*)configRec;

  configBegOfRecord=(byte*)temp;         // doit être le premier !!!
 
  mac=(byte*)temp;
  temp+=6;
  localIp=(byte*)temp;
  temp+=4;
  portserver=(int*)temp;
  temp+=sizeof(int);
  nomserver=(char*)temp;
  temp+=LNSERV;
  usrpass=(char*)temp;
  temp+=(LPWD+1);
  modpass=(char*)temp;  
  temp+=(LPWD+1);  
  peripass=(char*)temp;
  temp+=(LPWD+1);
  ssid=(char*)temp;
  temp+=(MAXSSID*(LENSSID+1));
  passssid=(char*)temp;
  temp+=(MAXSSID*(LPWSSID+1));
  nbssid=(int*)temp;
  temp+=sizeof(int);

  configEndOfRecord=(byte*)temp;      // doit être le dernier !!!
  temp ++;
//Serial.print((long)temp);Serial.print(" ");Serial.print((long)configEndOfRecord);Serial.print(" ");Serial.println((long)configBegOfRecord);
  
  configInitVar();
//Serial.print((long)temp);Serial.print(" ");Serial.print((long)configEndOfRecord);Serial.print(" ");Serial.println((long)configBegOfRecord);
}

void configInitVar()
{
memcpy(mac,MACADDR,6);
memcpy(localIp,lip,4);
*portserver = PORTSERVER;
memset(nomserver,0x00,LNSERV);memcpy(nomserver,NOMSERV,strlen(NOMSERV));
memset(usrpass,0x00,LPWD+1);memcpy(usrpass,USRPASS,strlen(SRVPASS));
memset(modpass,0x00,LPWD+1);memcpy(modpass,MODPASS,strlen(MODPASS));
memset(peripass,0x00,LPWD+1);memcpy(peripass,SRVPASS,strlen(PERIPASS));
//Serial.print(" strlen(SRVPASS)=");Serial.print(strlen(SRVPASS));Serial.print(" peripass=");for(int pp=0;pp<(LPWD+1);pp++){Serial.print(peripass[pp],HEX);Serial.print(" ");}Serial.println();
memset(ssid,0x00,MAXSSID*(LENSSID+1));
memcpy(ssid,SSID1,strlen(SSID1));memcpy(ssid+LENSSID+1,SSID2,strlen(SSID2));   
//Serial.print(" strlen(SSID1)=");Serial.print(strlen(SSID1));Serial.print(" ssid=");for(int pp=0;pp<(LENSSID+1);pp++){Serial.print(ssid[pp]);}Serial.println();
memset(passssid,0x00,MAXSSID*(LPWSSID+1));
memcpy(passssid,PWDSSID1,strlen(PWDSSID1));memcpy(passssid+LPWSSID+1,PWDSSID2,strlen(PWDSSID2));
*nbssid = MAXSSID;
}

void configPrint()
{
  Serial.print("Mac=");serialPrintMac(mac,0);
  Serial.print(" ");Serial.print(nomserver);
  Serial.print(" localIp=");for(int pp=0;pp<4;pp++){Serial.print((uint8_t)localIp[pp]);if(pp<3){Serial.print(".");}}Serial.print("/");Serial.println(*portserver);
  Serial.print("password=");Serial.print(usrpass);Serial.print(" modpass=");Serial.print(modpass);Serial.print(" peripass=");Serial.println(peripass);
  char bufssid[74];
  for(int nb=0;nb<MAXSSID;nb++){
    if(*(ssid+(nb*(LENSSID+1)))!='\0'){
        memset(bufssid,0x00,74);bufssid[0]=0x20;sprintf(bufssid+1,"%1u",nb);strcat(bufssid," ");
        strcat(bufssid,ssid+(nb*(LENSSID+1)));strcat(bufssid," ");
        int lsp=(LENSSID-strlen(ssid+nb*(LENSSID+1)));for(int ns=0;ns<lsp;ns++){strcat(bufssid," ");}
        strcat(bufssid,passssid+(nb*(LPWSSID+1)));
        Serial.println(bufssid);
    }
  }
}

int configLoad()
{
  //Serial.println("load config");
  int i=0;
  char configFile[]="srvconf\0";
  if(sdOpen(FILE_READ,&fconfig,configFile)==SDKO){return SDKO;}
  for(i=0;i<CONFIGRECLEN;i++){configRec[i]=fconfig.read();}
  fconfig.close();
  return SDOK;
}

int configSave()
{
  int i=0;
  int sta;
  char configFile[]="srvconf\0";
  
  if(sdOpen(FILE_WRITE,&fconfig,configFile)!=SDKO){
    sta=SDOK;
    fconfig.seek(0);
    for(i=0;i<CONFIGRECLEN;i++){fconfig.write(configRec[i]);}
//for(i=0;i<CONFIGRECLEN+4*sizeof(float)+2*sizeof(byte);i++){fconfig.write(configRec[i]);}      // ajouter les longueurs des variables ajoutées avant de modifier PERIRECLEN
    fconfig.close();
  }
  else sta=SDKO;
  Serial.print("configSave status=");Serial.println(sta);
  return sta;
}

/* >>>>>>>>> périphériques <<<<<<<<<< */

void periCheck(int num,char* text){periSave(NBPERIF+1);periLoad(num);Serial.print(" ");Serial.print(text);Serial.print(" Nb(");Serial.print(num);Serial.print(") sw=");Serial.print(*periSwNb);Serial.print(" det=");Serial.println(*periDetNb);periLoad(NBPERIF+1);}


void periFname(int num,char* fname)
{
  strcpy(fname,"PERI");
  fname[4]=(char)(num/10+48);
  fname[5]=(char)(num%10+48);
  fname[6]='\0';
}

void  periPrint(int num)
{
  Serial.print(num);Serial.print("/");Serial.print(*periNum);Serial.print(" ");Serial.print(periNamer);Serial.print(" ");
  serialPrintMac(periMacr,0);Serial.print(" ");serialPrintIp(periIpAddr);Serial.print(" sw=");Serial.print(*periSwNb);Serial.print(" det=");Serial.print(*periDetNb);Serial.print(" ");Serial.println(periVers);
}

int periLoad(int num)
{
  int i=0;
  if(periCacheStatus[num]==0){
    Serial.print("sdload table peri=");Serial.println(num);
    char periFile[7];periFname(num,periFile);
    if(sdOpen(FILE_READ,&fperi,periFile)==SDKO){return SDKO;}
    for(i=0;i<PERIRECLEN;i++){periCache[(num-1)*PERIRECLEN+i]=fperi.read();}              // periRec[i]=fperi.read();}
    fperi.close();
    periCacheStatus[num]=1;
  }
  for(i=0;i<PERIRECLEN;i++){periRec[i]=periCache[(num-1)*PERIRECLEN+i];}
  return SDOK;
}

int periRemove(int num)
{
  int i=0;
  char periFile[7];periFname(num,periFile);
  if(SD.exists(periFile)){SD.remove(periFile);}
  return SDOK;
}

int periSave(int num)
{

  int i=0;
  int sta;
  char periFile[7];periFname(num,periFile);
  
  //if(periCacheStatus[num]==0){ledblink(BCODEPERICACHEKO);}
  for(i=0;i<PERIRECLEN;i++){periCache[(num-1)*PERIRECLEN+i]=periRec[i];}    // copie dans cache
  periCacheStatus[num]=1;                                               // cache ok

  *periNum=periCur;
  if(sdOpen(FILE_WRITE,&fperi,periFile)!=SDKO){
    sta=SDOK;
    fperi.seek(0);
    for(i=0;i<PERIRECLEN;i++){fperi.write(periRec[i]);}
//for(i=0;i<PERIRECLEN+sizeof(uint16_t);i++){fperi.write(periRec[i]);}      // ajouter les longueurs des variables ajoutées avant de modifier PERIRECLEN
    fperi.close();

    for(int x=0;x<4;x++){lastIpAddr[x]=periIpAddr[x];}
  }
  else sta=SDKO;
  Serial.print("periSave(");Serial.print(num);Serial.print(")status=");Serial.print(sta);Serial.print(" periNum=");Serial.print(*periNum);Serial.print(" NbSw=");Serial.print(*periSwNb);if(num!=NBPERIF+1){Serial.println();}
  return sta;
}

void periInit()                 // pointeurs de l'enregistrement de table courant
{
  for(int nbp=0;nbp<NBPERIF;nbp++){periCacheStatus[nbp]=0x00;}
  
  periCur=0;
  int* filler;
  byte* temp=(byte*)periRec;

  periBegOfRecord=temp;         // doit être le premier !!!
  periNum=(int16_t*)temp;                           
  temp +=sizeof(int16_t);
  periPerRefr=(int32_t*)temp;
  temp +=sizeof(int32_t);
  periPitch=(float*)temp;
  temp +=sizeof(float);
  periLastVal=(float*)temp;
  temp +=sizeof(float);
  periAlim=(float*)temp;
  temp +=sizeof(float);
  periLastDateIn=(char*)temp;
  temp +=LENPERIDATE;
  periLastDateOut=(char*)temp;
  temp +=LENPERIDATE;
  periLastDateErr=(char*)temp;
  temp +=LENPERIDATE;
  periErr=(int8_t*)temp;
  temp +=sizeof(int8_t);   
  periNamer=(char*)temp;
  temp +=PERINAMLEN;
  periVers=(char*)temp;
  temp +=LENVERSION;  
  periModel=(char*)temp;
  temp +=LENMODEL;                
  periMacr=(byte*)temp;
  temp +=6;
  periIpAddr=(byte*)temp;
  temp +=4;
  periSwNb=(byte*)temp;
  temp +=sizeof(byte);
  periSwVal=(byte*)temp;
  temp +=sizeof(byte);
  periSwMode=(byte*)temp;
  temp +=MAXSW*MAXTAC*sizeof(byte);
  periSwPulseOne=(uint32_t*)temp;
  temp +=MAXSW*sizeof(uint32_t);
  periSwPulseTwo=(uint32_t*)temp;
  temp +=MAXSW*sizeof(uint32_t);  
  periSwPulseCurrOne=(uint32_t*)temp;
  temp +=MAXSW*sizeof(uint32_t);
  periSwPulseCurrTwo=(uint32_t*)temp;
  temp +=MAXSW*sizeof(uint32_t);  
  periSwPulseCtl=(byte*)temp;
  temp +=DLTABLEN*sizeof(byte);  
  periSwPulseSta=(byte*)temp;
  temp +=MAXSW*sizeof(byte);
  periSondeNb=(uint8_t*)temp;
  temp +=sizeof(uint8_t);
  periProg=(boolean*)temp;
  temp +=sizeof(boolean);
  periDetNb=(byte*)temp;
  temp +=sizeof(byte);
  periDetVal=(byte*)temp;
  temp +=sizeof(byte);
  periThOffset=(float*)temp;
  temp +=sizeof(float);
  periThmin=(float*)temp;
  temp +=sizeof(float);
  periThmax=(float*)temp;
  temp +=sizeof(float);  
  periVmin=(float*)temp;
  temp +=sizeof(float);  
  periVmax=(float*)temp;
  temp +=sizeof(float);  
  periDetServEn=(byte*)temp;
  temp +=1*sizeof(byte);
  periPerTemp=(uint16_t*)temp;
  temp +=sizeof(uint16_t);
//  dispo=(byte*)temp;
  temp +=1*sizeof(byte);
  periEndOfRecord=(byte*)temp;      // doit être le dernier !!!
  temp ++;
  

  periInitVar();
}

void periInitVar()
{
  *periNum=0;
  *periPerRefr=MAXSERVACCESS;
  *periPerTemp=TEMPERPREF;
  *periPitch=0;
  *periLastVal=0;
  *periAlim=0;
  memset(periLastDateIn,0x00,LENPERIDATE);
  memset(periLastDateOut,0x00,LENPERIDATE);
  memset(periLastDateErr,0x00,LENPERIDATE);
  *periErr=0;
  memset(periNamer,' ',PERINAMLEN);periNamer[PERINAMLEN-1]='\0';
  memset(periVers,' ',LENVERSION);periVers[LENVERSION-1]='\0';
  memset(periModel,' ',LENMODEL);
  memset(periMacr,0x00,6);
  memset(periIpAddr,0x00,4);
  *periSwNb=0;
  *periSwVal=0;
   memset(periSwMode,0x00,MAXSW*MAXTAC);
   memset(periSwPulseOne,0x00,MAXSW*SIZEPULSE);
   memset(periSwPulseTwo,0x00,MAXSW*SIZEPULSE); 
   memset(periSwPulseCurrOne,0x00,MAXSW*SIZEPULSE); 
   memset(periSwPulseCurrTwo,0x00,MAXSW*SIZEPULSE);       
   memset(periSwPulseCtl,0x00,MAXSW*DLSWLEN);
   memset(periSwPulseSta,0x00,MAXSW);   
  *periSondeNb=0;
  *periProg=FAUX;
  *periDetNb=0;
  *periDetVal=0;
  *periThOffset=0;
  *periThmin=-99;
  *periThmax=+125;
  *periVmin=3.25;
  *periVmax=3.50;
   memset(periDetServEn,0x00,2);
}

void periConvert()        
/* Pour ajouter une variable : 
 *  ajouter en fin de liste son descripteur dans frontal.ino, periInit() et periIinitVar()
 *  enlever les // devant derriere la séquence dans frontal.ino
 *  mettre en rem la ligne d'affichage et ctle de PERIRECLEN dans frontal.ino
 *  changer la ligne de "save" dans periSave() et ajouter la longueur supplémentaire
 *  NE PAS changer PERIRECLEN
 *  compiler, charger et laisser démarrer le serveur
 *  remettre la ligne de "save" normale dans periSave() 
 *  corriger PERIRECLEN dans const.h
 *  enlever les // devant la ligne d'affichage et ctle de PERIRECLEN dans frontal.ino
 *  remettre les /* ... devant derriere la séquence dans frontal.ino
 *  compiler, charger
 */
{
  char periFile[7];
  int i=0;
  periInitVar();            // les champs ajoutés sont initialisés ; les autres récupèreront les valeurs précédentes
  for(i=1;i<=NBPERIF+1;i++){
    periFname(i,periFile);Serial.print(periFile);
    if(periLoad(i)!=SDOK){Serial.print(" load KO");}
    else{
      SD.remove(periFile);
      if(periSave(i)!=SDOK){Serial.print(" save KO");} 
    }
    Serial.println();
  }
}

void periModif()
/*    Pour modifier la structure des données 
 *     
 *     COPIER LES FICHIERS AVANT MODIF
 *     
 *     modifier les 2 listes de pointeurs (Iperixxx) plus loin avec la nouvelle structure 
 *    
 *     modifier les transferts de données 
 *     modifier la nouvelle longueur totale
 *     enlever les // devant derrière la séquence dans frontal.ino
 *     
 *     compiler, charger et laisser démarrer le serveur
 *     
 *     remettre les // devant derrière la séquence dans frontal.ino     
 *     
 *     corriger PERIRECLEN dans const.h
 *     copier les pointeurs dans frontal.ino et periInit
 *     enlever les 'I'
 *     
 *  compiler, charger 
 */
{
/*
  int*      IperiNum;                      // ptr ds buffer : Numéro du périphérique courant
  long*     IperiPerRefr;                  // ptr ds buffer : période datasave minimale
  float*    IperiPitch;                    // ptr ds buffer : variation minimale de température pour datasave
  float*    IperiLastVal;                  // ptr ds buffer : dernière valeur de température  
  float*    IperiAlim;                     // ptr ds buffer : dernière tension d'alimentation
  char*     IperiLastDateIn;               // ptr ds buffer : date/heure de dernière réception
  char*     IperiLastDateOut;              // ptr ds buffer : date/heure de dernier envoi  
  char*     IperiLastDateErr;              // ptr ds buffer : date/heure de derniere anomalie com
  int8_t*   IperiErr;                      // ptr ds buffer : code diag anomalie com (voir MESSxxx shconst.h)
  char*     IperiNamer;                    // ptr ds buffer : description périphérique
  char*     IperiVers;                     // ptr ds buffer : version logiciel du périphérique
  byte*     IperiMacr;                     // ptr ds buffer : mac address 
  byte*     IperiIpAddr;                   // ptr ds buffer : Ip address
  byte*     IperiSwNb;                    // ptr ds buffer : Nbre d'interrupteurs (0 aucun ; maxi 4(MAXSW)            
  byte*     IperiSwVal;                   // ptr ds buffer : état/cde des inter  
  byte*     IperiSwMode;                  // ptr ds buffer : Mode fonctionnement inters            
  uint16_t* IperiSwPulse;                 // ptr ds buffer : durée pulses sec si interrupteur (0=stable, pas de pulse)
  uint16_t* IperiSwPulseCurr;             // ptr ds buffer : temps courant pulses
  byte*     IperiSwPulseCtl;             // ptr ds buffer : mode pulses
  byte*     IperiSwPulseTrig;             // ptr ds buffer : durée trig 
  uint8_t*  IperiSondeNb;                  // ptr ds buffer : nbre sonde
  boolean*  IperiProg;                     // ptr ds buffer : flag "programmable"
  byte*     IperiDetNb;                    // ptr ds buffer : Nbre de détecteurs maxi 4 (MAXDET)
  byte*     IperiDetVal;                   // ptr ds buffer : flag "ON/OFF" si détecteur (2 bits par détec))
  

  
  periCur=0;
  int* filler;

  char periRecNew[200];    // la taille n'a pas d'importance, c'est la longueur exacte qui sera enregistrée
  
  char* temp=periRecNew;
  
  IperiNum=(int*)temp;
  temp +=sizeof(int);
  IperiPerRefr=(long*)temp;
  temp +=sizeof(long);
  IperiPitch=(float*)temp;
  temp +=sizeof(float);
  IperiLastVal=(float*)temp;
  temp +=sizeof(float);
  IperiAlim=(float*)temp;
  temp +=sizeof(float);
  IperiLastDateIn=(char*)temp;
  temp +=6;
  IperiLastDateOut=(char*)temp;
  temp +=6;
  IperiLastDateErr=(char*)temp;
  temp +=6;
  IperiErr=(int8_t*)temp;
  temp +=sizeof(int8_t);   
  IperiNamer=(char*)temp;
  temp +=PERINAMLEN;
  IperiVers=(char*)temp;
  temp +=3;                  
  IperiMacr=(byte*)temp;
  temp +=6;
  IperiIpAddr=(byte*)temp;
  temp +=4;
  IperiSwNb=(byte*)temp;
  temp +=sizeof(byte);
  IperiSwVal=(byte*)temp;
  temp +=sizeof(byte);
  IperiSwMode=(byte*)temp;
  temp +=MAXSW*sizeof(byte);
  IperiSwPulse=(uint16_t*)temp;
  temp +=MAXSW*sizeof(uint16_t);
  IperiSwPulseCurr=(uint16_t*)temp;
  temp +=MAXSW*sizeof(uint16_t);
  IperiSwPulseCtl=(byte*)temp;
  temp +=MAXSW*sizeof(byte);
  IperiSwPulseTrig=(byte*)temp;
  temp +=MAXSW*sizeof(byte);      
  IperiSondeNb=(uint8_t*)temp;
  temp +=sizeof(uint8_t);
  IperiProg=(boolean*)temp;
  temp +=sizeof(boolean);
  IperiDetNb=(byte*)temp;
  temp +=sizeof(byte);
  IperiDetVal=(byte*)temp;
  temp +=sizeof(boolean);

  temp ++;

int periNewLen=temp-periRecNew;
Serial.print("periNewLen=");Serial.println(periNewLen);

  // =========== transfert =============== 

  char periFile[7];
  char periNewFile[8];
  int i=0;

  for(i=1;i<=NBPERIF;i++){
    periFname(i,periFile);Serial.print(periFile);

// chargement ancien enregistrement
    
    if(periLoad(i)!=SDOK){Serial.println(" inaccessible");}
    else{ 
      
Serial.print(" transfert enregistrement ");
// transfert

  *IperiNum=          i;
  *IperiPerRefr=      *periPerRefr;
  *IperiPitch=        *periPitch;
  *IperiLastVal=      *periLastVal;
  *IperiAlim=         *periAlim;
  packDate(IperiLastDateIn,periLastDate+2);
  packDate(IperiLastDateOut,periLastDate+2);
  packDate(IperiLastDateErr,periLastDate+2);
  *IperiErr=          0;
  memcpy(IperiNamer,periNamer,PERINAMLEN);
  memcpy(IperiVers,periVers,3);
  memcpy(IperiMacr,periMacr,6);
  memcpy(IperiIpAddr,periIpAddr,4);
  *IperiSwNb=        *periSwNb;
  *IperiSwVal=       *periSwVal;
  memset(IperiSwMode,0x00,4);
for(int k=0;k<MAXSW;k++){
  IperiSwPulse[k]=  periSwPulse[k];
  IperiSwPulseCurr[k]= 0;
  IperiSwPulseCtl[k]= 0;
  IperiSwPulseTrig[k]= 0;
}
  *IperiSondeNb=      1;
  *IperiProg=         0;
  *IperiDetNb=        *periDetNb;
  *IperiDetVal=       0;

Serial.print(" save ");

// nouveau fichier et save

    memcpy(periNewFile,periFile,7);periNewFile[6]='N';periNewFile[7]='\0';
    if(sdOpen(FILE_WRITE,&fperi,periNewFile)==SDKO){
      Serial.print(" SDOPEN WRITE KO");}
    else{fperi.seek(0);
      for(int j=0;j<periNewLen;j++){fperi.write(periRecNew[j]);}
      fperi.close();
     }
    Serial.println();
    }
  }
*/
}

