
#include "Arduino.h"
#include "const.h"
#include <ESP8266WiFi.h>
#include "ds1820.h"
#include "shconst.h"
#include "shutil.h"
#if CONSTANT==EEPROMSAVED
#include <EEPROM.h>
#endif

extern "C" {                  
#include <user_interface.h>     // pour struct rst_info, system_deep_sleep_set_option(), rtc_mem
}

extern Ds1820 ds1820;

extern WiFiClient cli;                 // client local du serveur externe
extern WiFiClient cliext;              // client externe du serveur local

extern char model[LENMODEL];

extern const char* host;
extern const int   port;
extern char bufServer[LBUFSERVER];

extern byte mac[6];
extern byte staPulse[MAXSW];            // état clock pulses
extern uint8_t pinSw[MAXSW];

extern constantValues cstRec;
extern char* cstRecA;

bool readConstant()
{

#if CONSTANT==RTCSAVED
  int temp=CONSTANTADDR;
  byte buf[4];
  system_rtc_mem_read(temp,buf,1); // charge la longueur telle qu'enregistrée
  cstRec.cstlen=buf[0];
  system_rtc_mem_read(temp,cstRecA,64);
#endif

#if CONSTANT==EEPROMSAVED
  cstRec.cstlen=EEPROM.read((int)(CONSTANTADDR));  // charge la longueur telle qu'enregistrée
  for(int temp=0;temp<cstRec.cstlen;temp++){        
    *(cstRecA+temp)=EEPROM.read((int)(temp+CONSTANTADDR));
  }
#endif

for(int k=1;k<=LENVERSION;k++){Serial.print(cstRecA[k]);}          // affichage version ; cstRec.cstVers[k]);}
Serial.print(" readConstant ");Serial.print((long)cstRecA,HEX);Serial.print(" len=");Serial.print(cstRec.cstlen);
Serial.print("/");Serial.print(sizeof(cstRec));
Serial.print(" crc=");Serial.print(*(cstRecA+cstRec.cstlen-1),HEX);Serial.print(" calc_crc=");
byte calc_crc=calcCrc(cstRecA,(uint8_t)cstRec.cstlen-1);Serial.println(calc_crc,HEX);
if(*(cstRecA+cstRec.cstlen-1)==calc_crc){return 1;}
return 0;
}

void writeConstant()
{
cstRec.cstlen=sizeof(cstRec);
memcpy(cstRec.cstVers,VERSION,LENVERSION);
cstRec.cstcrc=calcCrc(cstRecA,(uint8_t)cstRec.cstlen-1); 
// la longueur totale de la structure est plus grande que la position du crc !
  
#if CONSTANT==RTCSAVED
  int temp=CONSTANTADDR;
  system_rtc_mem_write(temp,cstRecA,64);
#endif

#if CONSTANT==EEPROMSAVED
  for(int temp=0;temp<cstRec.cstlen;temp++){
    if(*(cstRecA+temp)!=EEPROM.read((int)(temp+CONSTANTADDR))){
      EEPROM.write((int)(temp+CONSTANTADDR),*(cstRecA+temp));
    }    
  }
  EEPROM.commit();
#endif
Serial.print("writeConstant ");for(int h=0;h<4;h++){Serial.print(cstRec.cstVers[h]);};Serial.print(" ");
Serial.print((long)cstRecA,HEX);
Serial.print(" len=");Serial.print((char*)&cstRec.cstcrc-cstRecA+1);
Serial.print("/");Serial.print(cstRec.cstlen);
Serial.print(" crc=");Serial.println(cstRec.cstcrc,HEX);
}

void initConstant()  // inits mise sous tension
{
  cstRec.cstlen=sizeof(cstRec);
  memcpy(cstRec.numPeriph,"00",2);
  cstRec.serverTime=PERSERV+1;            // forçage talkserver à l'init
  cstRec.serverPer=PERSERV;
  cstRec.oldtemp=0;
  cstRec.tempPer=PERTEMP;
  cstRec.tempPitch=0;
  cstRec.talkStep=0;                       // pointeur pour l'automate talkServer()
  cstRec.swCde='\0';                       // cdes inter (4*2bits (periIntVal) ici x0 x=état demandé par le serveur pour le switch)
  memcpy(cstRec.actCde,"\0\0\0\0",4);      // mode sw
  memcpy(cstRec.desCde,"\0\0\0\0",4);      // mode sw
  memcpy(cstRec.onCde,"\0\0\0\0",4);       // mode sw
  memcpy(cstRec.offCde,"\0\0\0\0",4);      // mode sw  
  memset(cstRec.pulseCtl,0x00,DLTABLEN);   // ctle pulse
  memset(cstRec.durPulseOne,'\0',8);       // durée pulses (MAXSW=4 2 mots)
  memset(cstRec.durPulseTwo,'\0',8);       // durée pulses (MAXSW=4 2 mots)
  cstRec.IpLocal=IPAddress(0,0,0,0);
  memset(cstRec.memDetec,0x00,MAXDET);
  memcpy(cstRec.cstVers,VERSION,LENVERSION);
  memcpy(cstRec.cstModel,model,LENMODEL);
  Serial.println("Init Constant done");
  writeConstant();
}

void subprintConstant(byte swmode,char a)
{
  Serial.print(a);Serial.print(" ");
  Serial.print((swmode>>SWMDLNULS_PB));Serial.print("  ");
  for(int w=5;w>=0;w-=2){Serial.print((swmode>>w)&0x01);Serial.print((swmode>>(w-1))&0x01);Serial.print("  ");}
  Serial.println();
}

void subprintCtl(byte* ctl0)
{
  Serial.println(" Nd  e  x  f  h  Ac");
  uint64_t ctl=0;
  memcpy(&ctl,ctl0,DLSWLEN);
  for(int det=0;det<4;det++){
    Serial.print("  ");Serial.print((uint8_t)(ctl>>(det*DLBITLEN+DLNLS_PB))&0x07);
    Serial.print("  ");Serial.print((uint8_t)(ctl>>(det*DLBITLEN+DLENA_PB))&0x01);
    Serial.print("  ");Serial.print((uint8_t)(ctl>>(det*DLBITLEN+DLEL_PB))&0x01);
    Serial.print("  ");Serial.print((uint8_t)(ctl>>(det*DLBITLEN+DLMFE_PB))&0x01);
    Serial.print("  ");Serial.print((uint8_t)(ctl>>(det*DLBITLEN+DLMHL_PB))&0x01);
    Serial.print("  ");Serial.println((uint8_t)(ctl>>((det*DLBITLEN)+DLACLS_PB))&0x07);
  }
}

void printConstant()
{
  char buf[3],buff[32];memcpy(buf,cstRec.numPeriph,2);buf[2]='\0';
  Serial.print("\nnumPeriph ");Serial.print(buf);Serial.print(" IpLocal ");Serial.print(IPAddress(cstRec.IpLocal));
  Serial.print(" serverTime ");Serial.print(cstRec.serverTime);Serial.print(" serverPer ");Serial.println(cstRec.serverPer);
  Serial.print("oldtemp ");Serial.print(cstRec.oldtemp);Serial.print(" tempPer ");Serial.print(cstRec.tempPer);Serial.print(" tempPitch ");Serial.println(cstRec.tempPitch);
  Serial.print("staPulse=");for(int s=0;s<MAXSW;s++){Serial.print(s);Serial.print("-");Serial.print(staPulse[s],HEX);Serial.print(" ");}Serial.println();
  Serial.print("memDetec=");for(int s=0;s<MAXDET;s++){Serial.print(s);Serial.print("-");Serial.print(cstRec.memDetec[s],HEX);Serial.print(" ");}Serial.println();

  for(int i=0;i<NBSW;i++){
    Serial.print("sw=");Serial.print(i+1);Serial.print("-");Serial.print(digitalRead(pinSw[i]),HEX);Serial.print(" Cde=");Serial.print((cstRec.swCde>>(i*2+1))&0x01);
    Serial.print("  dur2=");Serial.print(cstRec.durPulseTwo[i]);
    Serial.print(" dur1=");Serial.println(cstRec.durPulseOne[i]);
    Serial.println(" Nd  dd  ss  pp");
    subprintConstant(cstRec.actCde[i],'A');subprintConstant(cstRec.desCde[i],'D');subprintConstant(cstRec.onCde[i],'I');subprintConstant(cstRec.offCde[i],'O');
    Serial.println();
    subprintCtl(&cstRec.pulseCtl[i*DLSWLEN]);
    Serial.println();
  }
}

/*
void serialPrintMac(byte* mac)
{
  char macBuff[18];
  unpackMac(macBuff,mac);
  Serial.println(macBuff);
}

void serialPrintIp(uint8_t* ip)
{
  for(int i=0;i<4;i++){Serial.print(ip[i]);if(i<3){Serial.print(".");}}
  Serial.print(" ");
}
*/

