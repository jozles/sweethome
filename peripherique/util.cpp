
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

extern const char* host;
extern const int   port;
extern char bufServer[LBUFSERVER];

extern byte  mac[6];

extern constantValues cstRec;
extern char* cstRecA;

bool readConstant()
{

#if CONSTANT==RTCSAVED
  int temp=CONSTANTADDR;
  system_rtc_mem_read(temp,&cstRec,cstRec.cstlen);
#endif

#if CONSTANT==EEPROMSAVED
  for(int temp=1;temp<cstRec.cstlen;temp++){               // temp=1 : skip len
    *(cstRecA+temp)=EEPROM.read((int)(temp+CONSTANTADDR));
    //Serial.print(*(cstRecA+temp),HEX);Serial.print(" ");
    }
  //Serial.println();

#endif
for(int k=0;k<4;k++){Serial.print(cstRec.cstVers[k]);}
Serial.print(" readConstant ");Serial.print((long)cstRecA,HEX);Serial.print(" len=");Serial.print(cstRec.cstlen);Serial.print(" crc=");Serial.print(cstRec.cstcrc,HEX);Serial.print(" calc_crc=");
byte calc_crc=calcCrc(cstRecA,(char*)&cstRec.cstcrc-cstRecA);Serial.println(calc_crc,HEX);
if(cstRec.cstcrc==calc_crc){return 1;}
return 0;
}

void writeConstant()
{
cstRec.cstcrc=calcCrc(cstRecA,(char*)&cstRec.cstcrc-cstRecA); 
// la longueur totale de la structure est plus grande que la position du crc !
  
#if CONSTANT==RTCSAVED
  int temp=CONSTANTADDR;
  system_rtc_mem_write(temp,&cstRec,cstRec.cstlen);
#endif

#if CONSTANT==EEPROMSAVED
  for(int temp=0;temp<cstRec.cstlen;temp++){
    if(*(cstRecA+temp)!=EEPROM.read((int)(temp+CONSTANTADDR))){
      EEPROM.write((int)(temp+CONSTANTADDR),*(cstRecA+temp));
    }    
  }
  EEPROM.commit();
#endif
Serial.print("writeConstant ");Serial.print((long)cstRecA,HEX);
Serial.print(" len=");Serial.print((char*)&cstRec.cstcrc-cstRecA);
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
  cstRec.intCde='\0';                     // cdes inter (4*2bits (periIntVal) ici x0 x=état demandé par le serveur pour le switch)
  memcpy(cstRec.actCde,"\0\0\0\0",4);     // mode sw
  memcpy(cstRec.desCde,"\0\0\0\0",4);     // mode sw
  memcpy(cstRec.onCde,"\0\0\0\0",4);      // mode sw
  memcpy(cstRec.offCde,"\0\0\0\0",4);     // mode sw  
  memcpy(cstRec.pulseMode,"\0\0\0\0",4);  // mode pulse
  memset(cstRec.intIPulse,'\0',8);        // durée pulses (MAXSW=4 2 mots)
  memset(cstRec.intOPulse,'\0',8);        // durée pulses (MAXSW=4 2 mots)  
  cstRec.IpLocal=IPAddress(0,0,0,0);
  memcpy(cstRec.cstVers,VERSION,4);
  Serial.println("initConstant");
  writeConstant();
}

void subprintConstant(byte swmode)
{
  Serial.print("   ");
  Serial.print((swmode>>6));Serial.print(" ");
  for(int w=5;w>=0;w-=2){Serial.print((swmode>>w)&0x01);Serial.print((swmode>>(w-1))&0x01);Serial.print("  ");}
  Serial.println();
}

void printConstant()
{
  char buf[3],buff[32];memcpy(buf,cstRec.numPeriph,2);buf[2]='\0';
  Serial.print("\nnumPeriph ");Serial.print(buf);Serial.print(" IpLocal ");Serial.print(IPAddress(cstRec.IpLocal));
  Serial.print(" serverTime ");Serial.print(cstRec.serverTime);Serial.print(" serverPer ");Serial.println(cstRec.serverPer);
  Serial.print("oldtemp ");Serial.print(cstRec.oldtemp);Serial.print(" tempPer ");Serial.print(cstRec.tempPer);Serial.print(" tempPitch ");Serial.println(cstRec.tempPitch);
  
  for(int i=0;i<NBSW;i++){
    Serial.print(i+1);Serial.print("  ");
    Serial.print((cstRec.intCde>>(i*2+1))&0x01);Serial.print("   ");Serial.print(cstRec.pulseMode[i],HEX);Serial.print(" ");Serial.print(cstRec.intOPulse[i]);Serial.print(" ");Serial.println(cstRec.intIPulse[i]);
    subprintConstant(cstRec.actCde[i]);subprintConstant(cstRec.desCde[i]);subprintConstant(cstRec.onCde[i]);subprintConstant(cstRec.offCde[i]);
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

