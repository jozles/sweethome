#include <Arduino.h>
#include <SD.h>
#include "const.h"
#include <Wire.h>
#include "utilether.h"
#include "shconst.h"
#include "shutil.h"
#include "periph.h"

File fperi;       // fichiers perif

extern char      periRec[PERIRECLEN];          // 1er buffer de l'enregistrement de périphérique
  
extern int       periCur;                    // Numéro du périphérique courant

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
extern byte*     periIntMode;                  // ptr ds buffer : Mode fonctionnement inters (1 par switch)           
extern uint32_t* periIntPulseOn;               // ptr ds buffer : durée pulses sec ON (0 pas de pulse)
extern uint32_t* periIntPulseOff;              // ptr ds buffer : durée pulses sec OFF(mode astable)
extern uint32_t* periIntPulseCurrOn;           // ptr ds buffer : temps courant pulses ON
extern uint32_t* periIntPulseCurrOff;          // ptr ds buffer : temps courant pulses OFF
extern uint16_t* periIntPulseMode;             // ptr ds buffer : mode pulses
extern uint8_t*  periSondeNb;                  // ptr ds buffer : nbre sonde
extern boolean*  periProg;                     // ptr ds buffer : flag "programmable"
extern byte*     periDetNb;                    // ptr ds buffer : Nbre de détecteurs maxi 4 (MAXDET)
extern byte*     periDetVal;                   // ptr ds buffer : flag "ON/OFF" si détecteur (2 bits par détec))

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

/*
void lb0()
{
    if(millis()>blinktime){
       if(digitalRead(PINLED)==HIGH){digitalWrite(PINLED,LOW);
          if(cntBlink<=1){blinktime=millis()+SLOWBLINK;}
          else{blinktime=millis()+FASTBLINK;}
          if(cntBlink>0){cntBlink--;}
       }
       else {digitalWrite(PINLED,HIGH);blinktime=millis()+PULSEBLINK;}        
       if(cntBlink==0){cntBlink=nbreBlink;}      // recharge cntblink
    }
}

void ledblink(uint8_t nbBlk)    // nbre blinks rapides tous les PERBLINK
{ 
         if(nbreBlink==0){nbreBlink=nbBlk;}                   // une fois nbreBlink chargé, la consigne est ignorée
         while(nbreBlink%2!=0){lb0();}                        // si nbreBlink impair blocage            
         lb0();                                               // sinon blink 1 ou nbreBlink
}
*/

void getdate(long* hms2,long* amj2,byte* js)
{
  char* days={"SunMonTueWedThuFriSat"};
  char* months={"JanFebMarAprMayJunJulAugSepOctNovDec"};
  int i=0;
  byte seconde,minute,heure,joursemaine,jour,mois,annee,msb,lsb; // numérique DS3231
  char buf[8];for(byte i=0;i<8;i++){buf[i]=0;} 
  readDS3231time(&seconde,&minute,&heure,js,&jour,&mois,&annee);
  *hms2=(long)(heure)*10000+(long)minute*100+(long)seconde;*amj2=(long)(annee+2000)*10000+(long)mois*100+(long)jour;
  for(i=0;i<32;i++){strdate[i]=0;}
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

void periFname(int num,char* fname)
{
  strcpy(fname,"peri");
  fname[4]=(char)(num/10+48);
  fname[5]=(char)(num%10+48);
  fname[6]='\0';
}

int periLoad(int num)
{
  //Serial.print("load table peri=");Serial.println(num);
  int i=0;
  char periFile[7];periFname(num,periFile);
  if(sdOpen(FILE_READ,&fperi,periFile)==SDKO){return SDKO;}
  for(i=0;i<PERIRECLEN;i++){periRec[i]=fperi.read();}
  fperi.close();
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
  //Serial.print("save table peri=");Serial.println(num);
  int i=0;
  char periFile[7];periFname(num,periFile);
  *periNum=periCur;
  if(sdOpen(FILE_WRITE,&fperi,periFile)==SDKO){return SDKO;}
  fperi.seek(0);
  for(i=0;i<PERIRECLEN;i++){fperi.write(periRec[i]);}
  //for(i=0;i<PERIRECLEN+1;i++){fperi.write(periRec[i]);}      // ajouter les longueurs des variables ajoutées avant de modifier PERIRECLEN
  fperi.close();
  for(int x=0;x<4;x++){lastIpAddr[x]=periIpAddr[x];}
  return SDOK;
}

void periInit()                 // pointeurs de l'enregistrement de table courant
{
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
  temp +=6;
  periLastDateOut=(char*)temp;
  temp +=6;
  periLastDateErr=(char*)temp;
  temp +=6;
  periErr=(int8_t*)temp;
  temp +=sizeof(int8_t);   
  periNamer=(char*)temp;
  temp +=PERINAMLEN;
  periVers=(char*)temp;
  temp +=3;                  
  periMacr=(byte*)temp;
  temp +=6;
  periIpAddr=(byte*)temp;
  temp +=4;
  periIntNb=(byte*)temp;
  temp +=sizeof(byte);
  periIntVal=(byte*)temp;
  temp +=sizeof(byte);
  periIntMode=(byte*)temp;
  temp +=MAXSW*MAXTAC*sizeof(byte);
  periIntPulseOn=(uint32_t*)temp;
  temp +=MAXSW*sizeof(uint32_t);
  periIntPulseOff=(uint32_t*)temp;
  temp +=MAXSW*sizeof(uint32_t);  
  periIntPulseCurrOn=(uint32_t*)temp;
  temp +=MAXSW*sizeof(uint32_t);
  periIntPulseCurrOff=(uint32_t*)temp;
  temp +=MAXSW*sizeof(uint32_t);  
  periIntPulseMode=(uint16_t*)temp;
  temp +=MAXSW*sizeof(uint16_t);  
  periSondeNb=(uint8_t*)temp;
  temp +=sizeof(uint8_t);
  periProg=(boolean*)temp;
  temp +=sizeof(boolean);
  periDetNb=(byte*)temp;
  temp +=sizeof(byte);
  periDetVal=(byte*)temp;
  temp +=sizeof(byte);
  periEndOfRecord=(byte*)temp;      // doit être le dernier !!!
  temp ++;
  
#define PERIRECCTL sizeof(int)+sizeof(char)*(15+PERINAMLEN)+sizeof(byte)*6+sizeof(long)+sizeof(boolean)*6+sizeof(float)*2+4

  periInitVar();
}

void periInitVar()
{
  *periPerRefr=PERIPREF;
  *periPitch=0;
  *periLastVal=0;
  memset(periNamer,' ',PERINAMLEN);periNamer[PERINAMLEN]='\0';
  memset(periMacr,0x00,6);
  memset(periIpAddr,0x00,4);
  memset(periLastDateIn,'0',6);
  memset(periLastDateOut,'0',6);
  memset(periLastDateErr,'0',6);
  *periErr=0;
  *periSondeNb=0;
  *periProg=FAUX;
  *periIntVal=0;
  *periIntNb=0;
   memset(periIntPulseOn,0x00,4);
   memset(periIntPulseOff,0x00,4); 
   memset(periIntPulseCurrOn,0x00,4); 
   memset(periIntPulseCurrOff,0x00,4);    
  *periDetNb=0;
  *periDetVal=0;
  *periAlim=0;
  memset(periVers,' ',3);periVers[3]='\0';
  
}

void periConvert()        
/* Pour ajouter une variable : 
 *  ajouter en fin de liste son descripteur dans frontal.ino, periInit() et periIinitVar()
 *  enlever les // devant derriere la séquence dans frontal.ino
 *  changer la ligne de "save" dans periSave() et ajouter la longueur supplémentaire
 *  NE PAS changer PERIRECLEN
 *  compiler, charger et laisser démarrer le serveur
 *  remettre la ligne de "save" normale dans periSave() 
 *  corriger PERIRECLEN dans const.h
 *  compiler, charger
 */
{
  char periFile[7];
  int i=0;
  periInitVar();            // les champs ajoutés sont initialisés ; les autres récupèreront les valeurs précédentes
  for(i=1;i<=NBPERIF;i++){
    periFname(i,periFile);Serial.print(periFile);
    if(periLoad(i)!=SDOK){Serial.print(" KO");}
    else{
      SD.remove(periFile);
      if(periSave(i)!=SDOK){Serial.print(" KO");} 
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
  byte*     IperiIntNb;                    // ptr ds buffer : Nbre d'interrupteurs (0 aucun ; maxi 4(MAXSW)            
  byte*     IperiIntVal;                   // ptr ds buffer : état/cde des inter  
  byte*     IperiIntMode;                  // ptr ds buffer : Mode fonctionnement inters            
  uint16_t* IperiIntPulse;                 // ptr ds buffer : durée pulses sec si interrupteur (0=stable, pas de pulse)
  uint16_t* IperiIntPulseCurr;             // ptr ds buffer : temps courant pulses
  byte*     IperiIntPulseMode;             // ptr ds buffer : mode pulses
  byte*     IperiIntPulseTrig;             // ptr ds buffer : durée trig 
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
  IperiIntNb=(byte*)temp;
  temp +=sizeof(byte);
  IperiIntVal=(byte*)temp;
  temp +=sizeof(byte);
  IperiIntMode=(byte*)temp;
  temp +=MAXSW*sizeof(byte);
  IperiIntPulse=(uint16_t*)temp;
  temp +=MAXSW*sizeof(uint16_t);
  IperiIntPulseCurr=(uint16_t*)temp;
  temp +=MAXSW*sizeof(uint16_t);
  IperiIntPulseMode=(byte*)temp;
  temp +=MAXSW*sizeof(byte);
  IperiIntPulseTrig=(byte*)temp;
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
  *IperiIntNb=        *periIntNb;
  *IperiIntVal=       *periIntVal;
  memset(IperiIntMode,0x00,4);
for(int k=0;k<MAXSW;k++){
  IperiIntPulse[k]=  periIntPulse[k];
  IperiIntPulseCurr[k]= 0;
  IperiIntPulseMode[k]= 0;
  IperiIntPulseTrig[k]= 0;
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

