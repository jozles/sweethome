
#include "const.h"
#include "Arduino.h"
#include "shconst.h"
#include "shutil.h"
#include "dynam.h"

#ifdef NEWPULS

enum {CKSTOP,CKRUN}
bool pulseClk=CKRUN;

/*   pulse mode   */

/* bits compteurs */

#define PMTOE_PB PBITE      // time one enable pos bit
#define PMTOE_VB VBITE      // val bit
#define PMTTE_PB PBITD      // time two enable pos bit
#define PMTTE_VB VBITD
#define PMFRO_PB PBITC      // freeRun/OneShoot
#define PMFRO_VB PBITC

/* bits détecteurs */

#define DLBITLEN      10      // longueur (bits) desciption détecteur
#define DLNMS_PB           // msb numéro (3 bits)
#define DLNMS_VB
#define DLNLS_PB           // lsb numéro
#define DLNLS_VB
#define DLENA_PB            // enable (1 bit)
#define DLENA_VB
#define DLEL_PB            // local/externe (1 bit)
#define DLEL_VB
#define DLMFE_PB            // mode flanc/état (1 bit)
#define DLMFE_VB
#define DLMHL_PB            // H/L (1 bit)
#define DLMHL_VB
#define DLACMS_PB           // msb action (3 bits)
#define DLACMS_VB
#define DLACLS_PB           // lsb action
#define DLACLS_VB

/* codes actions */

#define PMDCA_RESET 0       // reset
#define PMDCA_RAZ   1       // raz
#define PMDCA_STOP  2       // stop  clk
#define PMDCA_START 3       // start clk
#define PMDCA_SHORT 4       // short pulse
#define PMDCA_END   5       // end pulse

/* codes mode */

#define PMDM_STAT   0       // statique
#define PMDM_TRANS  1       // transitionnel

/* codes etats générateur (bit droite=valeur) */

#define PM_DISABLE  0x10
#define PM_IDLE0    0x20
#define PM_IDLE1    0x21
#define PM_RUN1     0x30
#define PM_RUN2     0x31
#define PM_END1     0x40
#define PM_END2     0x41

#define PM_FREERUN  0
#define PM_ONESHOOT 1
#define PM_ACTIF    1

#endif NEWPULSE





#ifdef BID

uint8_t runPulse(uint8_t sw)                               // get état pulse sw
{
  if(sw<NBSW){

  


  // disable ?
  if((cstRec.pulseCtl[sw] & PMTOE_VB) == 0){return PM_DISABLE;}
  
  // idle ?
  if(pulseClk==CKSTOP){
    if(cstRec.begPulseTwo[sw]!=0){return PM_IDLE1;}
    else return PM_IDLE0;
  }
  else(
  
    if(cstRec.begPulseOne[sw]!=0){
    //si compteur1 
      if((cstRec.begPulseOne[sw] + cstRec.durPulseOne[sw]) > millis()){return PM_RUN1;}                       // running 1
      //si pas fini run1
      else if((cstRec.pulseCtl[sw] & PMTTE_VB) == 0){return PM_END1;}                                        // end 1
      //sinon (fini) si compteur2 disable fin1
      else {cstRec.begPulseOne[sw]=0;cstRec.begPulseTwo[sw]=millis();return PM_RUN2;}                         // running 2
      //sinon raz compteur1 ; start compteur2
    }
    if(cstRec.begPulseTwo[sw]!=0){
    //si compteur 2  
      if((cstRec.begPulseTwo[sw] + cstRec.durPulseTwo[sw]) > millis()){return PM_RUN2;}                       // running 2
      //si pas fini run2
      else if(cstRec.pulseCtl[sw] & PMFRO_VB) == PM_ONESHOOT){
        cstRec.begPulseTwo[sw]=0;pulseClk=CKSTOP;return PM_IDLE0;}                                            // oneshoot = idle0
      //sinon (fini) si oneshoot raz compteur 2 stop clock : idle0 
      else if(((cstRec.pulseCtl[sw] & PMTOE_VB)!=0) && ((cstRec.pulseCtl[sw] & PMFRO_VB) == PM_FREERUN)){
        cstRec.begPulseOne[sw]=millis();return PM_RUN1;}                                                      // running 1
      //sinon (fini) et compteur1 enable et freerun ; init compteur 1 ; run1
      else return PM_END2;                                                                                    // fin 2
      //sinon fin2
      }
    }
  }
}


#endif BID


#ifdef OLDSYSTEM

/* pulse control --------------------- 

    pulse states  (PUACT!=0 active, PURMD reason ; PUACT=0 inactive, PUSMD reason) 
*/
#define PUCNT 0x01    // 0 pulse 1 ; 1 pulse 2
#define PUACT 0x02    // 1 active ; 0 halted
#define PURMD 0x0C    // 01 start ; 10 running ; 11 free run
#define PUSMD 0x30    // 01 stop ; 10 fin ; 11 idle

#define PUSTG1  0x06   // 0 - start1
#define PUSTA   0x30   // idle
#define PUSTH   0x10   // stoppé
#define PUSTR1  0x0A   // 0 - running1
#define PUSTR2  0x0B   // 1 - running2
#define PUSTF1  0x20   // fin 1
#define PUSTF2  0x21   // fin 2
#define PUSTG2  0x07   // 1 - start2
#define PUSTGF  0x0E   // 0 - start1 free run
#define PUSTOS  0x0F   // fin oneshot
#define PUSTSE  0      // system error
// test running 


uint8_t runPulse(uint8_t sw)                               // get état pulse sw
{
  if(sw<NBSW){

    uint8_t numDetOn=(cstRec.pulseCtl[sw]>>PMDINLS_PB)&0x0003;
    uint8_t numDetOff=(cstRec.pulseCtl[sw]>>PMDONLS_PB)&0x0003;

    bool enOne=(cstRec.pulseCtl[sw] & PMTOE_VB) != 0;
    bool enTwo=(cstRec.pulseCtl[sw] & PMTTE_VB) != 0;
    uint8_t cycle=((cstRec.pulseCtl[sw] & (PMCLS_VB | PMCMS_VB))>>PMCLS_PB);
    bool udcycle = cycle == UDCYCLE;
    bool freeRun = cycle == FREERUN ;
    bool oneshot = cycle == ONESHOT ;
    bool oneshotx= cycle == ONESHOTX ;
    bool runing1=cstRec.begPulseOne[sw] != 0;
    bool runing2=cstRec.begPulseTwo[sw] != 0;
    bool endOne=(millis()/1000-cstRec.begPulseOne[sw]) >= cstRec.durPulseOne[sw];
    bool endTwo=(millis()/1000-cstRec.begPulseTwo[sw]) >= cstRec.durPulseTwo[sw];
    
    bool edgeStop=((cstRec.pulseCtl[sw] & PMDOU_VB) == 0) || ((cstRec.memDetec>>(DETPRE_PB+numDetOff*LENDET))&0x0001 != (cstRec.memDetec>>(DETCUR_PB+numDetOff*LENDET))&0x0001);
    bool stopD=((cstRec.memDetec>>(DETEN_PB+numDetOff*LENDET))&0x001 !=0) && ((cstRec.memDetec>>(DETCUR_PB+numDetOff*LENDET))&0x0001 == ((cstRec.pulseCtl[sw]>>PMDOH_PB)&0x0001));
    bool enStp=(cstRec.pulseCtl[sw] & PMDOE_VB) != 0;
    bool stopS=(cstRec.pulseCtl[sw] & PMSRE_VB) != 0;
    bool stopPulse=(enStp && stopD && edgeStop) || stopS;

    bool edgeStart=((cstRec.pulseCtl[sw] & PMDIU_VB) == 0) || ((cstRec.memDetec>>(DETPRE_PB+numDetOn*LENDET))&0x0001 != (cstRec.memDetec>>(DETCUR_PB+numDetOn*LENDET))&0x0001);
    bool startD=((cstRec.memDetec>>(DETEN_PB+numDetOn*LENDET))&0x001 !=0) && ((cstRec.memDetec>>(DETCUR_PB+numDetOn*LENDET))&0x0001 == ((cstRec.pulseCtl[sw]>>PMDIH_PB)&0x0001));
    bool enStart=((cstRec.pulseCtl[sw] & PMDIE_VB ) != 0 ) && !oneshotx ;
    bool startPulse= enStart && startD && edgeStart;
  
    if(enOne && !runing1 && (!enTwo || !runing2) && startPulse){
      cstRec.begPulseOne[sw]=millis()/1000;cstRec.begPulseTwo[sw]=0;return PUSTG1;}                                                       // start 1                      
    if((!enOne || !runing1) && (!enTwo || !runing2)){return PUSTA;}                                                                       // arrêté
    if(stopPulse){cstRec.begPulseOne[sw]=0;cstRec.begPulseTwo[sw]=0;return PUSTH;}                                                        // stoppé
    if(runing1 && !endOne){return PUSTR1;}                                                                                                // running1
    if(runing2 && !endTwo){return PUSTR2;}                                                                                                // running2
    if(runing1 && endOne && !enTwo){cstRec.begPulseOne[sw]=0;cstRec.begPulseTwo[sw]=0;return PUSTF1;}                                     // fin 1
    if(runing2 && endTwo && (!enOne || !freeRun)){cstRec.begPulseOne[sw]=0;cstRec.begPulseTwo[sw]=0;return PUSTF2;}                       // fin 2
    if(runing1 && endOne && enTwo && !stopPulse){cstRec.begPulseOne[sw]=0;cstRec.begPulseTwo[sw]=millis()/1000;return PUSTG2;}            // start 2
    if(runing2 && endTwo && enOne && freeRun && !stopPulse){cstRec.begPulseOne[sw]=millis()/1000;cstRec.begPulseTwo[sw]=0;return PUSTGF;} // start 1 free run
    if(runing2 && endTwo && oneshot){
      cstRec.begPulseOne[sw]=0;cstRec.begPulseTwo[sw]=0;
      cstRec.pulseCtl[sw] &= ~(PMCLS_VB | PMCMS_VB); // effacement
      cstRec.pulseCtl[sw] |= (ONESHOTX<<PMCLS_PB);  // remplacement
      return PUSTOS;} // fin oneshot
    
    return PUSTSE;   // system error
  }
}

#endif OLDSYSTEM
