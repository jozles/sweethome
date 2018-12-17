#ifndef _UTIL_H_
#define _UTIL_H_

uint8_t calcCrc(char* bufCrc,int len);
byte setcrc(char* buf,int len);
void conv_atoh(char* ascii,byte* hex);
void conv_htoa(char* ascii,byte* hex);
float convStrToNum(char* str,int* sizeRead);
int convNumToString(char* str,float num);  // retour string terminée par '\0' ; return longueur totale '\0' inclus
boolean compMac(byte* mac1,byte* mac2);       // FAUX si != ; VRAI si ==
void packMac(byte* mac,char* ascMac);
void unpackMac(char* buf,byte* mac);
void serialPrintMac(byte* mac);
void charIp(byte* nipadr,char* aipadr);
void serialPrintIp(uint8_t* ip);
void packDate(char* dateout,char* datein);
void unpackDate(char* dateout,char* datein);
void serialPrintDate(char* datein);
void ledblink(uint8_t nbBlk);


#endif UTIL_H_

