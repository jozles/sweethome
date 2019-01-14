
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
extern  bool    pinLev[MAXDET];
extern  byte    pinDir[MAXDET];
extern  long    detTime[MAXDET];
extern  long    isrTime;
extern  void    (*isrD[4])(void);                              // tableau des pointeurs des isr détecteurs
extern  byte    mask[];

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


/* ------------------ généralités -------------------- 

  switchs :

  les switchs sont actionnés par poling de la table des ccommandes de switch 
  via swAction qui utilise les tables pulseCtl, swCde et staPulse

  (actuellement) 2 types : ON et OFF ; OFF est prioritaire

  3 sources possibles pour chaque type : 1 détecteur logique choisi dans la table des détecteurs logiques
                                         le bit de commande du serveur
                                         le générateur d'impulsion du switch

  chaque source a un bit enable et un bit de niveau actif



  détecteurs :

  3 types : physiques, spéciaux, externes ; (spéciaux : bits serveur, alarmes etc ; externes positionnés par commande serveur)
  
  levdet() récupère le niveau de tous types

  les détecteurs (physiques, spéciaux et externes) sont polés et, lorsqu'un changement d'état se produit,
  leur image mémoire est mise à jour dans memDetec

  le croisement de memDetec et pulseCtl fournit l'état du générateur d'impulsion (staPulse)

  pulseCtl (devrait se nommer detecCtl) paramètre le fonctionnement de 4 détecteurs logiques (internes ou externes)
  
  

  memDetec contient l'image mémoire des détecteurs locaux (physiques et spéciaux) et externes
  1 bit indique le niveau L/H (suit en continu le niveau du détecteur)
  1 bit indique le dernier flanc UP/DOWN
  2 bits indiquent l'état (DIS/IDLE/WAIT/TRIG)
  memDetec est mis à jour par polDx.
  le niveau de chaque détecteur est obtenu via levdet

  placé en mode suspendu  (IDLE) au reset et, si (TRIG) après traitement des pulses
  initialisé en mode armé (WAIT) en fin de débounce si le serveur a le détecteur enable
  placé en mode déclenché (TRIG) par la détection du flanc actif selon le serveur
  
  staPulse est l'état du générateur
  placé en mode débranché (DISABLE) en cas d'erreur système (action invalide d'un détecteur)
  placé en mode suspendu  (IDLE) suite à une action STOP d'un détecteur logique ou en fin de oneshot
  placé en mode comptage  (RUN)  suite à une action START d'un détecteur logique
  placé en mode fin       (END)  lorsque le comptage d'une phase est terminé et que la phase suivante est débranchée 
  staPulse est mis à jour par isrPulse
  l'état des pulses est obtenu par croisement de memDetec et swPulseCtl
 
*/


  

/* -------------- gestion commande switchs ----------------- */

uint8_t rdy(byte modesw,int sw) // pour les 3 sources, check bit enable puis etat source ; retour numsource valorisé si valide sinon 0
{
  /* ------ détecteur --------- */
  if((modesw & SWMDLEN_VB) !=0){                      // det enable
    uint8_t det=(modesw>>SWMDLNULS_PB)&0x03;          // numéro det
    uint64_t swctl;memcpy(&swctl,cstRec.pulseCtl+sw*DLSWLEN,DLSWLEN);
    uint16_t dlctl=(uint16_t)(swctl>>(det*DLBITLEN));
    if(dlctl&DLENA_VB != 0){                          // dl enable
      if(dlctl&DLEL_VB != 0){                         // dl local
        uint8_t locdet=(dlctl>>DLNLS_PB)&0x07;        // num dl local 
        if(((modesw>>SWMDLHL_PB)&0x01)==(cstRec.memDetec[locdet]>>DETBITLH_PB)){return 1;}      // ok détecteur local
      }
      else {}                                         // dl externe à traiter
    }
  }                                                                                                              

  /* --------- serveur ---------- */
  if((modesw & SWMSEN_VB) != 0){
    if (((modesw>>SWMSHL_PB)&0x01)==((cstRec.swCde>>((2*sw)+1))&0x01)){return 2;}    // ok serveur
  }

  /* --------- pulse gen --------- */
  if((modesw & SWMPEN_VB) != 0){
    if ((((modesw>>SWMPHL_PB)&0x01)==1 && staPulse[sw]==PM_RUN2)
     || (((modesw>>SWMPHL_PB)&0x01)==0 && staPulse[sw]==PM_RUN1)){return 3;}         // ok timer
  }
  return 0;                                                                          // ko 
}

uint8_t swAction()      // poling check cde des switchs
{ 
  uint8_t swa=0;
  for(int sw=0;sw<NBSW;sw++){
    
    // action OFF
    if(swa=rdy(cstRec.offCde[sw],sw)!=0){digitalWrite(pinSw[sw],OFF);}
    else{

      // action ON 
      if(swa=rdy(cstRec.onCde[sw],sw)!=0){digitalWrite(pinSw[sw],ON);swa+=10;}
      else{
        
        /*
      // action desactivation
        swa=rdy(cstRec.desCde[w],w);
        if(swa!=0){swa+=20;}                                               // à traiter
        else{
      
      // action activation  
          swa=rdy(cstRec.actCde[w],w);if(swa!=0){swa+=30;}                 // à traiter
        }
        */
      }   // pas ON
    }     // pas OFF
  }       // switch
  return swa;
}


/* ------------- gestion pulses ------------- */


void setPulseChg(int sw ,uint64_t* spctl,char timeOT)     // traitement fin de temps 
                                                          // spctl les DLSWLEN bytes d'un switch dans pulseCtl
                                                          // timeOT ='O' fin timeOne ; ='T' fin timeTwo
{
  if(timeOT=='O'){
        if((*spctl&PMTTE_VB)!=0){                                          // cnt2 enable -> run2
          cstRec.cntPulseOne[sw]=0;cstRec.cntPulseTwo[sw]=1;staPulse[sw]=PM_RUN2;}
        else {staPulse[sw]=PM_END1;}                                      // sinon fin1
  }
  else {
        if((*spctl&&PMFRO_VB)==0){                                         // oneshot -> idle
          cstRec.cntPulseTwo[sw]=0;cstRec.cntPulseOne[sw]=0;staPulse[sw]=PM_IDLE;}
        else if((*spctl&&PMTOE_VB)!=0){                                    // free run -> run      
          cstRec.cntPulseTwo[sw]=0;cstRec.cntPulseOne[sw]=1;staPulse[sw]=PM_RUN1;}
        else {staPulse[sw]=PM_END2;}                                      // free run bloqué -> fin2
  }
}

void pulseClkisr()     // poling ou interruption ; action horloge sur pulses tous switchs
{
  uint8_t sw;
  uint64_t spctl=0;memcpy(&spctl,cstRec.pulseCtl+sw*DLSWLEN,DLSWLEN);
  
  for(sw=0;sw<MAXSW;sw++){
    //Serial.print("pulseClkisr() sw=");Serial.print(sw);Serial.print( " staPulse=");Serial.println(staPulse[sw]);
    switch(staPulse[sw]){
      
      case PM_DISABLE: break;            // changement d'état quand le bit enable d'un compteur sera changé
      
      case PM_IDLE: break;               // changement d'état par l'action d'un détecteur logique

      case PM_RUN1: cstRec.cntPulseOne[sw]++;//printConstant();
                    if(cstRec.cntPulseOne[sw]>=cstRec.durPulseOne[sw]*10){              // (decap cnt1)
                      setPulseChg(sw,&spctl,'O');
                    }break;
                    
      case PM_RUN2: cstRec.cntPulseTwo[sw]++;//printConstant();
                    if(cstRec.cntPulseTwo[sw]>=cstRec.durPulseTwo[sw]*10){              // (decap cnt2)
                      setPulseChg(sw,&spctl,'T');
                    }break;
                    
      case PM_END1: break;               // changement d'état quand le bit enable du compteur 2 sera changé
      case PM_END2: break;               // changement d'état quand le bit enable du compteur 1 sera changé ou changement freerun->oneshot
      default:break;
    }
  }
}


void isrPul(uint8_t det)                        // maj staPulse au changement d'état d'un détecteur
{

  for(int sw=0;sw<NBSW;sw++){                                               // explo sw

    if(staPulse[sw]==PM_IDLE){

      uint64_t spctl=0;memcpy(&spctl,cstRec.pulseCtl+sw*DLSWLEN,DLSWLEN);     // les DL d'un switch
      for(int nb=0;nb<DLNB;nb++){                                             // explo DL 
        uint16_t spctlnb=(uint16_t)(spctl>>(nb*DLBITLEN))&DLBITMSK;           // spctnb les DLBITLEN bits du detecteur logique numéro nb

/*        if(det==0 && sw==0 && nb==0){
        Serial.print("\n10bits(");Serial.print(det);Serial.print(")=");Serial.print(spctlnb,HEX);
        Serial.print(" ds=");Serial.print((spctlnb>>DLNLS_PB)&mask[DLNMS_PB-DLNLS_PB+1]);
        Serial.print(" en=");Serial.print((spctlnb&DLENA_VB));
        Serial.print(" lo=");Serial.print((spctlnb&DLEL_VB));
        Serial.print(" lh=");Serial.print((spctlnb>>DLMHL_PB)&0x01);
        Serial.print(" lh=");Serial.print(((cstRec.memDetec[det]>>DETBITLH_PB)&0x01));}
*/        
        uint8_t test=0;
        if( (uint8_t)((spctlnb>>DLNLS_PB)&mask[DLNMS_PB-DLNLS_PB+1])==det ){ Serial.print("1");test+=1;}            // =det courant
        if( (spctlnb&DLENA_VB)!=0 ){  Serial.print("2");test+=2;}                                                   // enable
        if( (spctlnb&DLEL_VB)!=0  ){  Serial.print("3");test+=4;}                                                   // local
        if( (byte)((spctlnb>>DLMHL_PB)&0x01)==(byte)((cstRec.memDetec[det]>>DETBITLH_PB)&0x01) ){  Serial.print("4");test+=8;}    // LH ok
        
        if( test==15){  
          byte action=(byte)(spctlnb>>DLACLS_PB)&mask[DLACMS_PB-DLACLS_PB+1];
          byte actions[]={PMDCA_START,PMDCA_STOP,PMDCA_SHORT,PMDCA_RAZ,PMDCA_RESET,PMDCA_END};

          switch(strchr((char*)actions,action)-(char*)actions){
            case 0: Serial.print(" start");
                    if(cstRec.cntPulseOne[sw]!=0){staPulse[sw]=PM_RUN1;}                   
                    else if(cstRec.cntPulseTwo[sw]!=0){staPulse[sw]=PM_RUN2;}
                    else {staPulse[sw]=PM_RUN1;}
                    break;
            case 1: Serial.print(" stop");
                    staPulse[sw]=PM_IDLE;
                    break;
            case 2: Serial.print(" short");
                    if(staPulse[sw]==PM_RUN1 || cstRec.cntPulseOne[sw]!=0){cstRec.cntPulseOne[sw]=cstRec.durPulseOne[sw]*10;}
                    else if(staPulse[sw]==PM_RUN2 || cstRec.cntPulseTwo[sw]!=0){cstRec.cntPulseTwo[sw]=cstRec.durPulseTwo[sw]*10;}
                    break;                 
            case 3: Serial.print(" raz");
                    cstRec.cntPulseOne[sw]=0;
                    cstRec.cntPulseTwo[sw]=0;
                    break;               
            case 4: Serial.print(" reset");
                    cstRec.cntPulseOne[sw]=0;cstRec.durPulseOne[sw]=0;
                    cstRec.cntPulseTwo[sw]=0;cstRec.durPulseTwo[sw]=0;
                    staPulse[sw]=PM_IDLE;
                    break;                 
            case 5: Serial.print(" end");
                    if(staPulse[sw]==PM_RUN1 || cstRec.cntPulseOne[sw]!=0){setPulseChg(sw,&spctl,'O');}
                    else if(staPulse[sw]==PM_RUN2 || cstRec.cntPulseTwo[sw]!=0){setPulseChg(sw,&spctl,'T');}
                    break;                            
            default:Serial.print(" syserr=");Serial.print(action,HEX);
                    staPulse[sw]=PM_DISABLE;
                    break;
          }
        }
      }
    }
  }
}

byte levdet(uint8_t det)             // recup level détecteur det
{
  byte lev;
  if(det<MAXDET){lev=digitalRead(pinDet[det]);}
  else{ switch(det-MAXSW){
        case 0:lev=0;break;
        case 1:lev=1;break;
        case 2:lev=(cstRec.swCde>>(1))&0x01;break;       // bouton commande serveur 1
        case 3:lev=(cstRec.swCde>>(2+1))&0x01;break;     // bouton commande serveur 2
        default:lev=0;break;
        }
  }
  return lev;
}


void initPolPin(uint8_t det)         // setup détecteur det selon pulseCtl en fin de debounce ;
{                                     
  uint64_t swctl;
  byte lev=levdet(det);

  Serial.print(det);Serial.print(" ********* initPolPin (");Serial.print((cstRec.memDetec[det]>>DETBITLH_PB)&0x01);Serial.print("->");Serial.print(lev);Serial.print(") time=");

   cstRec.memDetec[det] &= ~DETBITST_VB;                                        // raz bits ST
   cstRec.memDetec[det] |= DETIDLE<<DETBITST_PB;                                // set bits ST (idle par défaut)
   cstRec.memDetec[det] &= ~DETBITLH_VB;                                        // raz bits LH
   cstRec.memDetec[det] |= lev<<DETBITLH_PB;                                    // set bit  LH  = detec   
   
  for(uint8_t sw=0;sw<NBSW;sw++){
    memcpy(&swctl,&cstRec.pulseCtl[sw*DLSWLEN],DLSWLEN);
    for(uint8_t dd=0;dd<DLNB;dd++){
      if(
           ((swctl>>(dd*DLBITLEN+DLEL_PB))&0x01==DLLOCAL)                       // local
        && ((swctl>>(dd*DLBITLEN+DLNLS_PB))&mask[DLNMS_PB-DLNLS_PB+1]==det)     // num ok
        && ((swctl>>(dd*DLBITLEN+DLENA_PB)&0x01)!=0)                            // enable
        )
        {
          cstRec.memDetec[det] &= ~DETBITUD_VB;                                       // raz bits UD
          cstRec.memDetec[det] |= (swctl>>(dd*DLBITLEN+DLMHL_PB)&0x01)<<DETBITUD_PB;  // set bit UD selon server          
          cstRec.memDetec[det] &= ~DETBITST_VB;                                       // raz bits ST
          cstRec.memDetec[det] |= DETWAIT<<DETBITST_PB;                               // set bits ST (armé)
        }
    }
  }
  Serial.println(detTime[det]);  
  detTime[det]=0;
}

void polDx(uint8_t det)              // poling détecteurs physiques et spéciaux -> maj memDetec
{
   
  if(cstRec.memDetec[det]>>DETBITST_PB != DETDIS) {
    
    byte lev=levdet(det);

    if( ((byte)(cstRec.memDetec[det]>>DETBITLH_PB)&0x01) != lev ){
      // level change -> update memDetec
      cstRec.memDetec[det] &= ~DETBITLH_VB;                           // raz bits LH
      cstRec.memDetec[det] |= lev<<DETBITLH_PB;                       // set bit LH 
      detTime[det]=millis();                                          // arme debounce
      Serial.print("  >>>>>>>>> det ");Serial.print(det);Serial.print(" change to ");Serial.print(lev);
    
      if( ((byte)(cstRec.memDetec[det]>>DETBITUD_PB)&0x01) == lev ){
        // waited edge
        Serial.print(" edge=");Serial.print((byte)(cstRec.memDetec[det]>>DETBITUD_PB)&0x01);
        cstRec.memDetec[det] &= ~DETBITST_VB;                           // raz bits ST
        cstRec.memDetec[det] |= DETTRIG<<DETBITST_PB;                   // set bits ST (déclenché)
        isrPul(det);                                                    // staPulse setup
        cstRec.memDetec[det] &= ~DETBITST_VB;                           // raz bits ST    
        cstRec.memDetec[det] |= DETIDLE<<DETBITST_PB;                   // retour Idle
      }
    Serial.println();printConstant();  
    }
  }
}

void polAllDet()
{
 for(uint8_t det=0;det<(MAXDET+MAXDSP+MAXDEX);det++){if(detTime[det]==0){polDx(det);}}    // pas de debounce en cours  
}
  

void swDebounce()                     
{
  for(uint8_t det=0;det<NBDET;det++){
    if(detTime[det]!=0 && (millis()>(detTime[det]+TDEBOUNCE))){
      detTime[det]=0; 
      //initIntPin(det);
      initPolPin(det);
    }
  }
}

/*/* --------------- interruptions détecteurs ----------------  

void isrDet(uint8_t det)      // setup memDetec et staPulse après interruption sur det
{
  long sht=micros();
  
  // setup memDetec 
  detTime[det]=millis();
  cstRec.memDetec[det] &= ~DETBITLH_VB;             // raz bit LH
  cstRec.memDetec[det] &= ~DETBITST_VB;             // raz bits ST
  cstRec.memDetec[det] |= DETTRIG<<DETBITST_PB;     // set bits ST (déclenché)
  if(pinDir[det]==(byte)HIGH){cstRec.memDetec[det] |= HIGH<<DETBITLH_PB;}
  else {cstRec.memDetec[det] |= LOW<<DETBITLH_PB;}
  pinDir[det]^=HIGH;                                // inversion pinDir pour prochain armement
  
  isrTime=micros()-sht;
}


void isrD0(){detachInterrupt(pinDet[0]);isrDet(0);}
void isrD1(){detachInterrupt(pinDet[1]);isrDet(1);}
void isrD2(){detachInterrupt(pinDet[2]);isrDet(2);}
void isrD3(){detachInterrupt(pinDet[3]);isrDet(3);}



void initIntPin(uint8_t det)              // enable interrupt du détecteur det ; flanc selon pinDir ; isr selon isrD
{                                         // setup memDetec

  Serial.print(det);Serial.println(" ********************* initIntPin");
  delay(1);
  cstRec.memDetec[det] &= ~DETBITLH_VB;            // raz bit LH
  cstRec.memDetec[det] &= ~DETBITST_VB;            // raz bits ST
  cstRec.memDetec[det] |= DETWAIT<<DETBITST_PB;    // set bits ST (armé)
  if(pinDir[det]==LOW){cstRec.memDetec[det] |= HIGH<<DETBITLH_PB;attachInterrupt(digitalPinToInterrupt(pinDet[det]),isrD[det], FALLING);}
  else {cstRec.memDetec[det] |= LOW<<DETBITLH_PB;attachInterrupt(digitalPinToInterrupt(pinDet[det]),isrD[det], RISING);}
}
*/


#endif NO_MODE
