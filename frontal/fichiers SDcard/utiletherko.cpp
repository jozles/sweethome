
#include <Arduino.h>
#include <Ethernet.h>
#include <EthernetUdp.h>
#include <SD.h>
#include "const.h"
#include "periph.h"

// udp DATA

unsigned int localUDPPort = 8888;         // local port to listen for UDP packets
char timeServer[] = "ntp-p1.obspm.fr\0";  // 145.238.203.14
const int NTP_PACKET_SIZE = 48;           // NTP time stamp is in the first 48 bytes of the message
byte packetBuffer[ NTP_PACKET_SIZE];      // buffer to hold incoming and outgoing packets

// UDP instance 
EthernetUDP Udp;

#define SECDAY 24*3600
#define SECYEAR 365*SECDAY
#define M28 SECDAY*28
#define M30 M28+SECDAY+SECDAY
#define M31 M30+SECDAY

extern char* months;
extern char* days;

extern long amj,hms,amj2,hms2;
extern byte js;
extern char strdate[33];
extern int     affmask;
extern boolean afficser; 
extern boolean stopwr;

extern uint8_t* chexa;

int sdOpen(char mode,File* fileS,char* fname)
{
  fileS->close();  
  if(!(*fileS=SD.open(fname,mode))){if(afficser==VRAI && ((affmask&AFF_SDerr)!=0)){Serial.print(fname);Serial.println(" inaccessible");return SDKO;}}
  
  return SDOK;
}


void sdstore_textdh0(File* fhisto,char* val1,char* val2,char* val3)
{
        int i=0; 
        char text[RECCHAR],bufx[7],crlf[3]={13,10,0};bufx[6]={0};
        for(i=0;i<RECCHAR;i++){text[i]=0;}
        sprintf(text,"%.8lu",amj);strcat(text,"   ");
        strcat(text," ");sprintf(bufx,"%.6lu",hms);strcat(text,bufx);
        strcat(text," ");strcat(text,val1);strcat(text," ");strcat(text,val2);
        strcat(text," ");strcat(text,val3);strcat(text,"<br>");strcat(text,crlf);
        
        if(stopwr!=VRAI)
          {fhisto->print(text);if(afficser==VRAI && ((affmask & AFF_SDstore)!=0)){Serial.print(text);}}
}

void sdstore_textdh(File* fhisto,char* val1,char* val2,char* val3)
{
  getdate(&hms,&amj,&js);
  sdstore_textdh0(fhisto,val1,val2,val3);
}


int htmlSave(File* fhtml,char* fname,char* buff)                 // enregistre 1 buffer html
{
  char inch;
  fhtml->close();
  if(sdOpen(FILE_WRITE,fhtml,fname)==SDKO){if(afficser==VRAI && ((affmask&AFF_SDhtml)!=0)){Serial.print("html ");Serial.print(fname);Serial.println("KO");return SDKO;}}
  
  fhtml->print(buff);if(afficser==VRAI && ((affmask&AFF_SDhtml)!=0)){Serial.print(inch);}
  return SDOK;
}

int htmlPrint(EthernetClient* cli,File* fhtml,char* fname)                 // lis une page html sur cli
{
  char inch;
  int brcrlf=0;
  if(sdOpen(FILE_READ,fhtml,fname)==SDKO){cli->print("</title></head><body>");cli->print(fname);cli->println(" inaccessible<br>");return SDKO;}
  
  long htmlSiz=fhtml->size();
  long ptr=0;
  while (ptr<htmlSiz || inch==-1)
    {
    inch=fhtml->read();ptr++;
        cli->print(inch);
        if(afficser==VRAI && ((affmask&AFF_SDhtml)!=0)){Serial.print(inch);}
        switch(brcrlf){
          case 0:if(inch=="<"){brcrlf=1;}break;
          case 1:if(inch=="b"){brcrlf=2;}else {brcrlf=0;}break;
          case 2:if(inch=="r"){brcrlf=3;}else {brcrlf=0;}break;
          case 3:if(inch==">"){brcrlf=4;cli->println("");}else {brcrlf=0;}break;
          case 4:brcrlf=0;break;
        default:break;
        }
    }
   fhtml->close();
   return SDOK;
}

/*
void getdate_google(EthernetClient* cligoogle)
{
  if(afficser==VRAI && ((affmask&AFF_getdate)!=0)){Serial.print("cligoogle connected ?...");}
  if (!cligoogle.connected()){
    if (!cligoogle.connect(extserver1, 80)){return 0;}
  }   
  if(afficser==VRAI && ((affmask&AFF_getdate)!=0)){Serial.println("cligoogle connected");}
  
  cligoogle->println("GET /search?q=test  HTTP/1.1");
  cligoogle->println("Host: www.google.com");
  cligoogle->println("Connection: close");
  cligoogle->println(); 

  boolean termine=FAUX,fini=FAUX,fini2=FAUX; 
  int ptr=0,cnt=0;
  int posdate=-1;
  if(afficser==VRAI && ((affmask&AFF_getdate)!=0)){Serial.println("*getdate google*");}
  while (!fini || !fini2)
    {
    if (cligoogle->available()) 
      {
      char c = cligoogle->read();if(afficser==VRAI && ((affmask&AFF_getdate)!=0)){Serial.print(c);}
      //debug(c,ptr,posdate,strdate,cnt);
      if(posdate==strdate){strdate[ptr]=c;strdate[ptr+1]={0};ptr++;if(c<' ' || ptr>=32){fini=VRAI;}}
      if(posdate!=strdate && termine==VRAI){fini=VRAI;}
      if (!termine)
          {
          cnt++;if(cnt>500){termine=VRAI;Serial.println("***terminé");}
          strdate[ptr]=c;posdate=strstr(strdate,"Date: ");
          if(posdate!=strdate){ptr++;if(ptr>=5){for(ptr=0;ptr<5;ptr++){strdate[ptr]=strdate[ptr+1];}ptr=5;}}    
          else {termine=VRAI;ptr=0;}
          }
      }
    else {fini2=VRAI;}
    }
    cligoogle->stop();if(afficser==VRAI && ((affmask&AFF_getdate)!=0)){Serial.println("\n---- fin getdate google");}
}
*/

void sendNTPpacket(char* address) 
{
  // set all bytes in the buffer to 0
  memset(packetBuffer, 0, NTP_PACKET_SIZE);
  // Initialize values needed to form NTP request
  // (see URL above for details on the packets)
  packetBuffer[0] = 0b11100011;   // LI, Version, Mode
  packetBuffer[1] = 0;     // Stratum, or type of clock
  packetBuffer[2] = 6;     // Polling Interval
  packetBuffer[3] = 0xEC;  // Peer Clock Precision
  // 8 bytes of zero for Root Delay & Root Dispersion
  packetBuffer[12]  = 49;
  packetBuffer[13]  = 0x4E;
  packetBuffer[14]  = 49;
  packetBuffer[15]  = 52;

  // all NTP fields have been given values, now
  // you can send a packet requesting a timestamp:
  Udp.beginPacket(address, 123); //NTP requests are to port 123
  Udp.write(packetBuffer, NTP_PACKET_SIZE);
  Udp.endPacket();
}

void convertNTP(unsigned long *dateUnix,int *year,int *month,int *day,byte *js,int *hour,int *minute,int *second)
{
    int feb29;
    unsigned long monthSec[]={0,M31,M31+M28,2*M31+M28,2*M31+M28+M30,3*M31+M28+M30,3*M31+M28+2*M30,4*M31+M28+2*M30,5*M31+M28+2*M30,5*M31+M28+3*M30,6*M31+M28+3*M30,6*M31+M28+4*M30,7*M31+M28+4*M30};
    unsigned long sec1quart=0;
    unsigned long secLastQuart=0;
    unsigned long secLastYear=0;
    unsigned long secLastMonth=0;
    unsigned long secLastDay=0;
    
    if(*dateUnix<2*SECYEAR){*year=(int)(*dateUnix/SECYEAR)+1970L;feb29=1;}
    else
      {sec1quart = 4*SECYEAR+SECDAY;
      secLastQuart = (*dateUnix-(2*SECYEAR))%sec1quart;  // sec dans quartile courant (1972 bisextile)
//      Serial.print(secLastQuart);Serial.print(" ");
      *year = (int)((((*dateUnix-(2*SECYEAR))/sec1quart)*4)+1972L); // 1ère année quartile courant
//      Serial.print(*year);Serial.print(" ");
      if(secLastQuart<=(monthSec[2]+SECDAY)){feb29=0;monthSec[2]+=SECDAY;}        // les 29 premiers jours de la 1ère année du quartile
      else{*year += ((secLastQuart-SECDAY)/SECYEAR);feb29=1;}
      }
    secLastYear=((secLastQuart-feb29*SECDAY)%SECYEAR);        
    for(int i=0;i<12;i++){if(monthSec[i+1]>secLastYear){*month=i+1;i=12;}}
    
    secLastMonth=secLastYear-monthSec[*month-1];
    *day  = (int)(secLastMonth/SECDAY)+1;
    secLastDay = secLastMonth-((*day-1)*SECDAY);
    *hour = (int)(secLastDay/3600);
    *minute = (int)(secLastDay-*hour*3600L)/60;
    *second = (int)(secLastDay-*hour*3600L-*minute*60);

    /* joursemaine
      z= y-1
      si m >= 3, D = { [(23m)/9] + d + 4 + y + [z/4] - [z/100] + [z/400] - 2 } mod 7
      si m < 3, D = { [(23m)/9] + d + 4 + y + [z/4] - [z/100] + [z/400] } mod 7
    */
    
    int k=0;if(*month>=3){k=2;}
    *js=( (int)((23*(*month))/9) + *day + 4 + *year + (int)((*year-1)/4) - (int)((*year-1)/100) + (int)((*year-1)/400) - k )%7;

//char buf[4]={0};strncat(buf,days+(*js)*3,3);    
//Serial.print(*js);Serial.print(" ");Serial.print(buf);Serial.print(" ");
//Serial.print(*year);Serial.print("/");Serial.print(*month);Serial.print("/");Serial.print(*day);Serial.print(" ");
//Serial.print(*hour);Serial.print(":");Serial.print(*minute);Serial.print(":");Serial.print(*second);
}

int getUDPdate(long* hms,long* amj,byte* js)
{
  int year=0,month=0,day=0,hour=0,minute=0,second=0;
  
  Udp.begin(localUDPPort);
  
  sendNTPpacket(timeServer); // send an NTP packet to a time server

  // wait to see if a reply is available
  delay(1000);
  if (Udp.parsePacket()) {
    // We've received a packet, read the data from it
    Udp.read(packetBuffer, NTP_PACKET_SIZE); // read the packet into the buffer   // sec1900- 2208988800UL;
    unsigned long highWord = word(packetBuffer[40], packetBuffer[41]);
    unsigned long lowWord = word(packetBuffer[42], packetBuffer[43]);
    unsigned long secsSince1900 = highWord << 16 | lowWord;
    unsigned long sec1970 = secsSince1900 -2208988800UL;               //sec1970=1456790399UL;  // 29/02/2016
//Serial.print(" sec 1900/1970 : ");Serial.print(secsSince1900,10);Serial.print(" ");Serial.println(sec1970,10);
//Serial.print("AAMMJ HHMMSS ");
  
    convertNTP(&sec1970,&year,&month,&day,js,&hour,&minute,&second);
    *amj=year*10000L+month*100+day;*hms=hour*10000L+minute*100+second;
    return 1;
  }
  return 0;
}

byte calcBitCrc (byte shiftReg, byte data_bit)
{
  byte fb;
  
  fb = (shiftReg & 0x01) ^ data_bit;
   /* exclusive or least sig bit of current shift reg with the data bit */
   shiftReg = shiftReg >> 1;                  /* shift one place to the right */
   if (fb==1){shiftReg = shiftReg ^ 0x8C;}    /* CRC ^ binary 1000 1100 */
   return(shiftReg); 
}


uint8_t calcCrc(char* buf,int len)
{
  char bitMask[]={0x01,0x02,0x04,0x08,0x10,0x20,0x40,0x80};
  uint8_t c,crc;
  int i,j,k;
  crc=0;
  for(int i=0;i<len;i++){//Serial.print(i);Serial.print(" ");
  //Serial.print(buf[i]);
    for(j=0;j<8;j++){c=0;if((buf[i] & bitMask[j])!=0){c=1;}//Serial.print(c);Serial.print(" ");}
    crc=calcBitCrc(crc,c);}// Serial.println(crc);}
  }
  //Serial.print(" nbre car pour crc ");Serial.print(len);Serial.print(" crc ");
  //Serial.print((char)chexa[(int)(crc/16)]);Serial.println((char)chexa[(int)(crc%16)]);
  return crc;
}

void conv_atoh(char* ascii,uint8_t* h)
{
  *h='\0';
  uint8_t v0=(uint8_t)strchr(chexa,*ascii)-(uint8_t)chexa,v1=(uint8_t)strchr(chexa,*(ascii+1))-(uint8_t)chexa;
  //Serial.print(v0);Serial.print(" ");Serial.println(v1);
  if(v0>15){v0-=6;};
  if(v1>15){v1-=6;};
  *h=(uint8_t)(v0*16+v1);
}

void conv_htoa(char* ascii,byte* hex)
{
  *ascii=*(chexa+(int)((*hex)/16));*(ascii+1)=*(chexa+(int)((*hex)%16));
}

boolean compMac(byte* mac1,byte* mac2)
{
  for(int i=0;i<6;i++){if(mac1[i] != mac2[i]){return FAUX;}}
  return VRAI; 
}

void packMac(uint8_t* mac,char* ascMac)
{
  for(int i=0;i<6;i++){conv_atoh(ascMac+i*3,mac+i);
  //Serial.print(ascMac+i*3);Serial.print(" ");Serial.println((uint8_t)(mac+i),HEX);
  }
}

void unpackMac(char* buf,uint8_t* mac)
{
  for(int i=0;i<6;i++){conv_htoa(buf+(i*3),mac+i);if(i<5){buf[i*3+2]='.';}}
    buf[17]='\0';
}

