
#include "Arduino.h"
#include "shconst.h"


extern char  pass[];
extern char* chexa;

static unsigned long blinktime=0;
uint8_t nbreBlink=0;          // si nbreBlink impair   -> blocage
uint8_t cntBlink=0;

#define TIMEOVFSLOTNB 10
uint32_t timeOvfSlot[TIMEOVFSLOTNB];


  /* debug */
int   cntdebug[NBDBPTS];
long  timedebug[NBDBPTS*NBDBOC];
int*  v0debug[NBDBPTS*NBDBOC];
int*  v1debug[NBDBPTS*NBDBOC];
char* v2debug[NBDBPTS*NBDBOC];
char* v3debug[NBDBPTS*NBDBOC];
int   int00=0;
int*  int0=&int00;


int convNumToString(char* str,float num)  // retour string terminée par '\0' ; return longueur totale '\0' inclus
{
  long num0=(long)num;
  float num1=num;
  int i=0,j=0,n=0,v=0,t=0;

  while(num0!=0){num0/=10;i++;}                         // comptage nbre chiffres partie entière
  t=i;n=i;num0=(long)num;
  for (i;i>0;i--){v=num0%10;str[i-1]=chexa[v];}      // unpack digits partie entière

  i=2;
  if(i!=0){
    str[n]='.';t=t+(i+1);                            // t longueur totale, n+1=longueur entière+'.'
    num1=num-(int)num;
    for(j=0;j<i;j++){num1=num1*10;v=(int)num1;num1=num1-v;str[n+1+j]=chexa[v];}
  }
  str[t]='\0';t++;

  return t;
}

void serialPrintIp(uint8_t* ip)
{
  for(int i=0;i<4;i++){Serial.print(ip[i]);if(i<3){Serial.print(".");}}
  Serial.print(" ");
}

void charIp(byte* nipadr,char* aipadr)
{
  char buf[8];
  for(int i=0;i<4;i++){
        sprintf(buf,"%d",nipadr[i]);strcat(aipadr,buf);if(i<3){strcat(aipadr,".");}
  }
}

void conv_atoh(char* ascii,byte* h)
{
    uint8_t c=0;
  c = (uint8_t)(strchr(chexa,ascii[0])-chexa)<<4 ;
  c |= (uint8_t)(strchr(chexa,ascii[1])-chexa) ;
  *h=c;
}

void conv_htoa(char* ascii,byte* h)
{
    uint8_t c=*h,d=c>>4,e=c&0x0f;
//    Serial.print(c,HEX);Serial.print(" ");Serial.print(d,HEX);Serial.print(" ");Serial.print(e,HEX);Serial.print(" ");
//    Serial.print(chexa[d],HEX);Serial.print(" ");Serial.println(chexa[e],HEX);
        ascii[0]=chexa[d];ascii[1]=chexa[e];
        // Serial.print(c,HEX);Serial.print(" ");Serial.print(ascii[0],HEX);Serial.print(" ");Serial.println(ascii[1],HEX);
}

void dumpstr0(char* data,uint8_t len)
{
    char a[]={0x00,0x00,0x00};
    uint8_t c;
    Serial.print((long)data,HEX);Serial.print("   ");
    for(int k=0;k<len;k++){conv_htoa(a,(byte*)&data[k]);Serial.print(a);Serial.print(" ");}
    Serial.print("    ");
    for(int k=0;k<len;k++){
            c=data[k];
            if(c<32 || c>127){c='.';}
            Serial.print((char)c);
    }
    Serial.println();
}

void dumpstr(char* data,uint16_t len)
{
    while(len>=16){len-=16;dumpstr0(data,16);data+=16;}
    if(len!=0){dumpstr0(data,len);}
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
  int i,j;

  crc=0;
  for(i=0;i<len;i++){//Serial.print(i);Serial.print(" ");
    for(j=0;j<8;j++){c=0;if((buf[i] & bitMask[j])!=0){c=1;}//Serial.print(c);Serial.print(" ");}
    crc=calcBitCrc(crc,c);}// Serial.println(crc);}
  }
  return crc;
}

byte setcrc(char* buf,int len)
{
  byte c=calcCrc(buf,len);
  conv_htoa(buf+len,&c);buf[len+2]='\0';
}


float convStrToNum(char* str,int* sizeRead)
{
  float v=0;
  uint8_t v0=0;
  float pd=1;
  int minu=1;
  int i=0;

  for(i=0;i<8;i++){
    if(i==0 && str[i]=='+'){i++;}
    if(i==0 && str[i]=='-'){i++;minu=-1;}
    if(str[i]=='.'){if(pd==1){pd=10;}i++;}
    *sizeRead=i+1;
    if(str[i]!='_' && str[i]!='\0' && str[i]>='0' && str[i]<='9'){
      v0=*(str+i)-48;
      if(pd==1){v=v*10+v0;}
      else{v+=v0/pd;pd*=10;}
    }
    else {i=8;}
  }
  //Serial.print("s>n str,num=");Serial.print(string);Serial.print(" ");Serial.println(v*minus);
  return v*minu;
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

void unpackMac(char* buf,byte* mac) //uint8_t* mac)
{
  for(int i=0;i<6;i++){conv_htoa(buf+(i*3),mac+i);if(i<5){buf[i*3+2]='.';}}
    buf[17]='\0';
}

void serialPrintMac(byte* mac)
{
  char macBuff[18];
  unpackMac(macBuff,mac);
  Serial.println(macBuff);
}


void packDate(char* dateout,char* datein)
{
    for(int i=0;i<6;i++){
        dateout[i]=datein[i*2] << 4 | (datein[i*2+1] & 0x0F);
    }
}

void unpackDate(char* dateout,char* datein)
{
    for(int i=0;i<6;i++){
        dateout[i*2]=(datein[i] >> 4)+48; dateout[i*2+1]=(datein[i] & 0x0F)+48;
    }
}

void serialPrintDate(char* datein)
{
    for(int i=0;i<6;i++){
        Serial.print((char)((datein[i] >> 4)+48));Serial.print ((char)((datein[i] & 0x0F)+48));}
        Serial.println();

}


void lb0()
{
    if(millis()>blinktime){
       if(digitalRead(PINLED)==LEDON){digitalWrite(PINLED,LEDOFF);
          if(cntBlink<=1){blinktime=millis()+SLOWBLINK;}
          else{blinktime=millis()+FASTBLINK;}
          if(cntBlink>0){cntBlink--;}
       }
       else {digitalWrite(PINLED,LEDON);blinktime=millis()+PULSEBLINK;}
       if(cntBlink==0){cntBlink=nbreBlink;}      // recharge cntblink
    }
}

void ledblink(uint8_t nbBlk)    // nbre blinks rapides tous les PERBLINK
{
         if(nbreBlink==0 ){
                if(nbBlk!=BCODEONBLINK){nbreBlink=nbBlk;}                   // une fois nbreBlink chargé, la consigne est ignorée
                else digitalWrite(PINLED,LEDON);
         }
         if(nbBlk==100+nbreBlink){nbreBlink=0;}
         while(nbreBlink%2!=0){lb0();}                        // si nbreBlink impair blocage
         lb0();                                               // sinon blink 1 ou nbreBlink
}

void timeOvfSet(uint8_t slot)
{
    if(slot<=TIMEOVFSLOTNB){timeOvfSlot[slot-1]=micros();}
}

void timeOvfCtl(uint8_t slot)
{
    if(slot<=TIMEOVFSLOTNB){
    Serial.print("tOvf[");Serial.print(slot);
    Serial.print("]=");Serial.println((micros()-timeOvfSlot[slot-1]));
    if((micros()-timeOvfSlot[slot-1])>2000){Serial.print("<<<<<<<<<<<<");}
    }
}

/* debug */


void debug(int cas){
  if(cntdebug[cas]==0 || cas==1){
    Serial.print("debug");Serial.println(cas);
  }
   cntdebug[cas]++;
}

void setdebug(int cas,int* v0,int* v1,char* v2, char* v3)
{
    cntdebug[cas]++;if(cntdebug[cas]<NBDBOC){
    timedebug[cas*NBDBOC+cntdebug[cas]]=micros();
    v0debug[cas*NBDBOC+cntdebug[cas]]=v0;
    v1debug[cas*NBDBOC+cntdebug[cas]]=v1;
    v2debug[cas*NBDBOC+cntdebug[cas]]=v2;
    v3debug[cas*NBDBOC+cntdebug[cas]]=v3;
  }
}

void showdebug()
{
  for(int deb=0;deb<NBDBPTS;deb++){
    Serial.print("point=");Serial.println(deb);
    for(int occ=0;occ<cntdebug[deb];occ++){
      Serial.print("     occ=");Serial.print(occ);
      Serial.print(" ");Serial.print(timedebug[deb*NBDBOC+occ]);
      Serial.print(" ");Serial.print((int)v0debug[deb*NBDBOC+occ]);
      Serial.print(" ");Serial.print((int)v1debug[deb*NBDBOC+occ]);
      Serial.print(" ");Serial.print((char*)v2debug[deb*NBDBOC+occ]);
      Serial.print(" ");Serial.println((char*)v3debug[deb*NBDBOC+occ]);
    }
  }
}

void initdebug()
{
  Serial.print("   *** init debug ***");
  for(int dg=0;dg<NBDBPTS;dg++){
    cntdebug[dg]=0;for(int dp=0;dp<NBDBOC;dp++){timedebug[dg*NBDBOC+dp]=0;}
  }
}

bool ctlpass(char* data,char* model)
{
  int j=0;
  bool passok=FAUX;

  dumpstr(data,16);
  for(j=0;j<LENVAL;j++){

    if(model[j]==0){passok=VRAI;break;}
    if(data[j]!=pass[j]){break;}
  }
Serial.print(passok);Serial.println(data);

  return passok;
}
