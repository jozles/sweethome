#ifndef DYNAM_H_INCLUDED
#define DYNAM_H_INCLUDED

#if POWER_MODE==NO_MODE


void pulseClkisr();             // interrupt ou poling clk @10Hz



// gestion interruptions détecteurs
//void isrDet(uint8_t det);       // setup memDetec et staPulse après interruption sur det
//void isrD0();
//void isrD1();
//void isrD2();
//void isrD3();
//void initIntPin(uint8_t det);
//void initPolPin(uint8_t det);
void polDx(uint8_t det);
void polAllDet();
void swDebounce();
void memdetinit();
uint8_t swAction();

#endif NO_MODE
#endif // DYNAM_H_INCLUDED
