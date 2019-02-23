#ifndef _PAGESHTML_H_
#define _PAGESHTML_H_

int  dumpsd(EthernetClient* cli);
void htmlIntro0(EthernetClient* cli);
void htmlIntro(char* titre,EthernetClient* cli);
void cfgServerHtml(EthernetClient* cli);
void accueilHtml(EthernetClient* cli);


#endif // _PAGESHTML_H_
