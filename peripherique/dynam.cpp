
#include "const.h"
#include "Arduino.h"
#include "shconst.h"
#include "shutil.h"
#include "dynam.h"
#include "util.h"

#if POWER_MODE==NO_MODE

extern constantValues cstRec;

extern  uint8_t pinSw[MAXSW];                                  // les switchs
extern  byte    staPulse[MAXSW];                               // état clock pulses
extern  uint8_t pinDet[MAXDET];
extern  long    detTime[MAXDET];
extern  byte    pinDir[MAXDET];
extern  long    isrTime;
extern  void    (*isrD[4])(void);                              // tableau des pointeurs des isr détecteurs

//extern  int     cntdebug[];
//extern  long    timedebug[]={0,0,0,0};
extern  int*    int0;

/*extern  int   cntdebug[NBDBPTS];
extern  long  timedebug[NBDBPTS*NBDBOC];
extern  int*  v0debug[NBDBPTS*NBDBOC];
extern  int*  v1debug[NBDBPTS*NBDBOC];
extern  char* v2debug[NBDBPTS*NBDBOC];
extern  char* v3debug[NBDBPTS*NBDBOC];
extern  int*  int0=&(0x00);*/



void pulseClkisr()     // interrupt clk @10Hz
{
  int sw;
  uint64_t spctl=0;memcpy(&spctl,cstRec.pulseCtl+sw*DLSWLEN,DLSWLEN);
  
  for(sw=0;sw<MAXSW;sw++){
    
    switch(staPulse[sw]){
      
      case PM_DISABLE: break;            // changement d'état quand le bit enable d'un compteur sera changé
      
      case PM_IDLE: break;               // changement d'état par l'action d'un détecteur logique

      case PM_RUN1: cstRec.cntPulseOne[sw]++;
                    if(cstRec.cntPulseOne[sw]>=cstRec.durPulseOne[sw]*10){              // (decap cnt1)
                      if((spctl&PMTTE_VB)!=0){                                          // cnt2 enable -> run2
                        cstRec.cntPulseOne[sw]=0;cstRec.cntPulseTwo[sw]=1;staPulse[sw]=PM_RUN2;}
                      else {staPulse[sw]=PM_END1;}                                      // sinon fin1
                    }break;
                    
      case PM_RUN2: cstRec.cntPulseTwo[sw]++;
                    if(cstRec.cntPulseTwo[sw]>=cstRec.durPulseTwo[sw]*10){              // (decap cnt2)
                      if((spctl&&PMFRO_VB)==0){                                         // oneshot -> idle
                        cstRec.cntPulseTwo[sw]=0;cstRec.cntPulseOne[sw]=0;staPulse[sw]=PM_IDLE;}
                      else if((spctl&&PMTOE_VB)!=0){                                    // free run -> run1
                        cstRec.cntPulseTwo[sw]=0;cstRec.cntPulseOne[sw]=1;staPulse[sw]=PM_RUN1;}
                      else {staPulse[sw]=PM_END2;}                                      // free run bloqué -> fin2
                    }break;
                    
      case PM_END1: break;               // changement d'état quand le bit enable du compteur 2 sera changé
      case PM_END2: break;               // changement d'état quand le bit enable du compteur 1 sera changé ou changement freerun->oneshot
      default:break;
    }
  }
}

/* interruptions détecteurs */

void isrDet(uint8_t det)      // setup memDetec et staPulse après interruption sur det
{
  long sht=micros();
  
  /* setup memDetec */
  detTime[det]=millis();
  cstRec.memDetec[det] &= ~DETBITLH_VB;             // raz bit LH
  cstRec.memDetec[det] &= ~DETBITST_VB;             // raz bits ST
  cstRec.memDetec[det] |= DETTRIG<<DETBITST_PB;     // set bits ST (déclenché)
  if(pinDir[det]==(byte)HIGH){cstRec.memDetec[det] |= HIGH<<DETBITLH_PB;}
  else {cstRec.memDetec[det] |= LOW<<DETBITLH_PB;}
  pinDir[det]^=HIGH;                                // inversion pinDir pour prochain armement

  /* setup staPulse */

  if(staPulse==PM_IDLE){
    setdebug(0,int0,int0,(char*)int0,(char*)int0);
    for(int sw=0;sw<MAXSW;sw++){                                              // explo sw
      uint64_t spctl=0;memcpy(&spctl,cstRec.pulseCtl+sw*DLSWLEN,DLSWLEN);     // les DL d'un switch
      for(int nb=0;nb<DLNB;nb++){                                             // explo DL 
        uint16_t spctlnb=spctl>>(nb*DLBITLEN);                                // spctnb les DLBITLEN bits du detecteur logique numéro nb
        // =det courant && enable && start && local && même flanc/état ?
        if(    (spctlnb>>DLNLS_PB)&0x03==det 
            && (spctlnb&DLENA_VB)!=0 
            && (spctlnb>>DLACLS_PB)==PMDCA_START 
            && (spctlnb&DLEL_VB)!=0
            && ((spctlnb>>DLMHL_PB)&0x01)==((cstRec.memDetec[det]>>DETBITLH_PB)&0x01)
           )
           {
           if(cstRec.cntPulseOne[sw]!=0){staPulse[sw]=PM_RUN1;}
           else if(cstRec.cntPulseTwo[sw]!=0){staPulse[sw]=PM_RUN2;}
           else {staPulse[sw]=PM_RUN1;}
        }
      }
    }
  }
  isrTime=micros()-sht;
}


void isrD0(){detachInterrupt(pinDet[0]);isrDet(0);}
void isrD1(){detachInterrupt(pinDet[1]);isrDet(1);}
void isrD2(){detachInterrupt(pinDet[2]);isrDet(2);}
void isrD3(){detachInterrupt(pinDet[3]);isrDet(3);}

void initIntPin(uint8_t det)              // enable interrupt du détecteur det ; flanc selon pinDir ; isr selon isrD
{                                         // setup memDetec

  Serial.print(det);Serial.println(" ********************* initIntPin");
  cstRec.memDetec[det] &= ~DETBITLH_VB;            // raz bit LH
  cstRec.memDetec[det] &= ~DETBITST_VB;            // raz bits ST
  cstRec.memDetec[det] |= DETWAIT<<DETBITST_PB;    // set bits ST (armé)
  if(pinDir[det]==LOW){attachInterrupt(digitalPinToInterrupt(pinDet[det]),isrD[det], FALLING);cstRec.memDetec[det] |= HIGH<<DETBITLH_PB;}
  else {attachInterrupt(digitalPinToInterrupt(pinDet[det]),isrD[det], RISING);cstRec.memDetec[det] |= LOW<<DETBITLH_PB;}

}

#endif NO_MODE
