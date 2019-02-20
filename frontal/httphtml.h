#ifndef _HTTPHTML_H_
#define _HTTPHTML_H_

int  dumpsd(EthernetClient* cli);
void periTableHtml(EthernetClient* cli);
void accueilHtml(EthernetClient* cli);
void htmlIntro0(EthernetClient* cli);
void htmlIntro(char* titre,EthernetClient* cli);
void checkboxTableHtml(EthernetClient* cli,uint8_t* nom,char* nomfonct,int etat);
void intModeTableHtml(EthernetClient* cli,uint16_t* valeur,int nbli,int nbtypes);
void cliPrintMac(EthernetClient* cli, byte* mac);
void cbErase();

#endif // _HTTPHTML_H_
