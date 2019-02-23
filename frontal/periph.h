#ifndef _PERIPH_H_
#define _PERIPH_H_


typedef struct Ymdhms Ymdhms;

struct Ymdhms
{
  uint8_t year;
  uint8_t month;
  uint8_t day;
  uint8_t hour;
  uint8_t minute;
  uint8_t second;
};

Ymdhms now();

void alphaNow(char* buff);  // charge 15 car YYYYMMDDHHMMSS\0

void setDS3231time(byte second, byte minute, byte hour, 
    byte dayOfWeek,byte dayOfMonth, byte month, byte year);

void readDS3231time(byte *second,byte *minute,byte *hour,
    byte *dayOfWeek,byte *dayOfMonth,byte *month,byte *year);

void readDS3231temp(byte *msb,byte *lsb);

//int  temphydro(char* temp,char* temp0,char*humid);

void ledblink(uint8_t nbre);

void getdate(uint32_t* hms2,uint32_t* amj2,byte* js);

int   periLoad(int num);
int   periSave(int num);
int   periRemove(int num);
void  periModif();
void  periConvert();
void  periInit();
void  periInitVar(); // le contenu de periRec seul

void  configInit();
int   configLoad();
int   configSave();
void  configInit();
void  configInitVar(); 
void  configPrint();

#endif // _PERIPH_
