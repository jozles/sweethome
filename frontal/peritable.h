#ifndef _PERITABLE_H_
#define _PERITABLE_H_


void periTableHtml(EthernetClient* cli);
void SwCtlTableHtml(EthernetClient* cli,int nbsw,int nbtypes);
void intModeTableHtml(EthernetClient* cli,uint16_t* valeur,int nbli,int nbtypes);
void cliPrintMac(EthernetClient* cli, byte* mac);
void cbErase();
void lnkTableHtml(EthernetClient* cli,char* nomfonct,char* lib);
void numTableHtml(EthernetClient* cli,char type,void* valfonct,char* nomfonct,int len,uint8_t td,int pol);
void xradioTableHtml(EthernetClient* cli,byte valeur,char* nomfonct,byte nbval,int nbli,byte type);
void checkboxTableHtml(EthernetClient* cli,uint8_t* val,char* nomfonct,int etat,uint8_t td);
void boutonHtml(EthernetClient* cli,byte* valfonct,char* nomfonct,uint8_t sw,uint8_t td);


#endif // _PERITABLE_H_
