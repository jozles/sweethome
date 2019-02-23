#ifndef _UTILETHER_H_
#define _UTILETHER_H_

#include <SD.h>
#include <Ethernet.h>

int sdOpen(char mode,File* fileS,char* fname);
int htmlPrint(EthernetClient* cli,File* fhtml,char* fname);
int htmlSave(File* fhtml,char* fname,char* buff);
//void getdate_google(EthernetClient* cligoogle);
void sdstore_textdh(File* fhisto,char* val1,char* val2,char* val3);  // getdate()
void sdstore_textdh0(File* fhisto,char* val1,char* val2,char* val3);
int getUDPdate(uint32_t* hms,uint32_t* amj,byte* js);

void ShowMemory(void);


#endif // _UTILETHER_H_
