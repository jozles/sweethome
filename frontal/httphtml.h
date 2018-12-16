#ifndef _HTTPHTML_H_
#define _HTTPHTML_H_

int  dumpsd();
void periTableHtml(EthernetClient* cli);
void htmlIntro0(EthernetClient* cli);
void htmlIntro(char* titre,EthernetClient* cli);
void acceuilHtml(EthernetClient* cli,bool passok);
void checkboxTableHtml(EthernetClient* cli,uint8_t* nom,char* nomfonct,int etat);
void intModeTableHtml(EthernetClient* cli,uint16_t* valeur,int nbli,int nbtypes);
void cliPrintMac(EthernetClient* cli, byte* mac);


#endif // _HTTPHTML_H_
