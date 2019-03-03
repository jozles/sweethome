#include <Arduino.h>
#include <SPI.h>      //bibliothéqe SPI pour W5100
#include <Ethernet.h>
#include <SD.h>
#include <shutil.h>
#include <shconst.h>
#include "const.h"
#include "periph.h"
#include "utilether.h"
#include "pageshtml.h"

#ifndef WEMOS
//  #include <avr/wdt.h>  //biblio watchdog
#endif ndef WEMOS

extern File      fhisto;      // fichier histo sd card
extern long      sdpos;
extern char*     nomserver;
extern byte      memDetServ;  // image mémoire NBDSRV détecteurs (8)
extern uint16_t  perrefr;

extern char* usrpass;             // mot de passe browser
extern char* modpass;             // mot de passe modif
extern char* peripass;            // mot de passe périphériques

extern char* chexa;

extern uint8_t remote_IP[4],remote_IP_cur[4];

extern char      periRec[PERIRECLEN];        // 1er buffer de l'enregistrement de périphérique
  
extern int       periCur;                    // Numéro du périphérique courant

extern int*      periNum;                      // ptr ds buffer : Numéro du périphérique courant
extern long*     periPerRefr;                  // ptr ds buffer : période datasave minimale
extern float*    periPitch;                    // ptr ds buffer : variation minimale de température pour datasave
extern float*    periLastVal;                  // ptr ds buffer : dernière valeur de température  
extern float*    periAlim;                     // ptr ds buffer : dernière tension d'alimentation
extern char*     periLastDateIn;               // ptr ds buffer : date/heure de dernière réception
extern char*     periLastDateOut;              // ptr ds buffer : date/heure de dernier envoi  
extern char*     periLastDateErr;              // ptr ds buffer : date/heure de derniere anomalie com
extern int8_t*   periErr;                      // ptr ds buffer : code diag anomalie com (voir MESSxxx shconst.h)
extern char*     periNamer;                    // ptr ds buffer : description périphérique
extern char*     periVers;                     // ptr ds buffer : version logiciel du périphérique
extern char*     periModel;                    // ptr ds buffer : model du périphérique
extern byte*     periMacr;                     // ptr ds buffer : mac address 
extern byte*     periIpAddr;                   // ptr ds buffer : Ip address
extern byte*     periSwNb;                     // ptr ds buffer : Nbre d'interrupteurs (0 aucun ; maxi 4(MAXSW)            
extern byte*     periSwVal;                    // ptr ds buffer : état/cde des inter  
extern byte*     periSwMode;                   // ptr ds buffer : Mode fonctionnement inters (4 bytes par switch)           
extern uint32_t* periSwPulseOne;               // ptr ds buffer : durée pulses sec ON (0 pas de pulse)
extern uint32_t* periSwPulseTwo;               // ptr ds buffer : durée pulses sec OFF(mode astable)
extern uint32_t* periSwPulseCurrOne;           // ptr ds buffer : temps courant pulses ON
extern uint32_t* periSwPulseCurrTwo;           // ptr ds buffer : temps courant pulses OFF
extern byte*     periSwPulseCtl;               // ptr ds buffer : mode pulses 
extern byte*     periSwPulseSta;               // ptr ds buffer : état clock pulses
extern uint8_t*  periSondeNb;                  // ptr ds buffer : nbre sonde
extern boolean*  periProg;                     // ptr ds buffer : flag "programmable" (périphériques serveurs)
extern byte*     periDetNb;                    // ptr ds buffer : Nbre de détecteurs maxi 4 (MAXDET)
extern byte*     periDetVal;                   // ptr ds buffer : flag "ON/OFF" si détecteur (2 bits par détec))
extern float*    periThOffset;                 // ptr ds buffer : offset correctif sur mesure température
extern float*    periThmin;                    // ptr ds buffer : alarme mini th
extern float*    periThmax;                    // ptr ds buffer : alarme maxi th
extern float*    periVmin;                     // ptr ds buffer : alarme mini volts
extern float*    periVmax;                     // ptr ds buffer : alarme maxi volts
extern byte*     periDetServEn;                // ptr ds buffer : 1 byte 8*enable detecteurs serveur

extern int8_t    periMess;                     // code diag réception message (voir MESSxxx shconst.h)
extern byte      periMacBuf[6]; 


extern int  chge_pwd; //=FAUX;

extern byte mask[];

  char colour[8];

void cliPrintMac(EthernetClient* cli, byte* mac)
{
  char macBuff[18];
  unpackMac(macBuff,mac);
  cli->print(macBuff);
}

void boutonHtml(EthernetClient* cli,byte* valfonct,char* nomfonct,uint8_t sw,uint8_t td)      
{
  char tonoff[]={'O','F','F','\0'};

  nomfonct[LENNOM-2]=(char)(PMFNCHAR+sw);
  nomfonct[LENNOM-1]='1';

  if(td==1 || td==2){cli->print("<td>");}
  if ((*valfonct>>(sw*2))&0x01==0) {nomfonct[LENNOM-1]='0';memcpy(tonoff,"ON \0",4);}
  cli->print("<a href=\"");cli->print(nomfonct);cli->print("\"><button class=\"button\">");cli->print(tonoff);cli->println("</button></a>");
//  cli->print("<input type=\"button\" name=\"");cli->print(nomfonct);cli->print("\" <button class=\"button\">");cli->print(tonoff);cli->println("</button></a>");
  if(td==2 || td==3){cli->println("</td>");}
//              cli->println("<p><a href=\"/B/off\"><button class=\"button button2\">OFF</button></a></p>");

}

void textTableHtml(EthernetClient* cli,char type,float* valfonct,float* valmin,float* valmax,uint8_t br,uint8_t td)
{
  memcpy(colour,"black\0",6);if(*valfonct<*valmin || *valfonct>*valmax){memcpy(colour,"red\0",4);}
  if(td==1 || td==2){cli->print("<td>");}
    cli->print("<font color=\"");cli->print(colour);cli->print("\"> ");
/*    switch (type){
      case 'b':cli->print(*(byte*)valfonct);break;
      case 'd':cli->print(*(uint16_t*)valfonct);break;
      case 'i':cli->print(*(int*)valfonct);break;
      case 'l':cli->print(*(long*)valfonct);break;
      case 'f':cli->print(*(float*)valfonct);break;
      case 'g':cli->print(*(uint32_t*)valfonct);break;    
      default:break;}
*/
  cli->print(*valfonct);  
  cli->print("</font>");
  if(br==1){cli->print("<br>");}
  if(td==2 || td==3){cli->println("</td>");}
}


void lnkTableHtml(EthernetClient* cli,char* nomfonct,char* lib)
{
  cli->print("<a href=page.html?");cli->print(nomfonct);cli->print(": target=_self>");
  cli->print(lib);cli->println("</a>");
}

void numTableHtml(EthernetClient* cli,char type,void* valfonct,char* nomfonct,int len,uint8_t td,int pol)
{                          
  if(td==1 || td==2){cli->print("<td>");}
  if(pol!=0){cli->print("<font size=\"");cli->print(pol);cli->println("\">");}
  cli->print("<input type=\"text\" name=\"");cli->print(nomfonct);
  if(len<=2){cli->print("\" id=\"nt");cli->print(len);}
  cli->print("\" value=\"");
  switch (type){
    case 'b':cli->print(*(byte*)valfonct);break;
    case 'd':cli->print(*(uint16_t*)valfonct);break;
    case 'i':cli->print(*(int*)valfonct);break;
    case 'l':cli->print(*(long*)valfonct);break;
    case 'f':cli->print(*(float*)valfonct);break;
    case 'g':cli->print(*(uint32_t*)valfonct);break;    
    default:break;
  }
  int sizeHtml=1;if(len>=3){sizeHtml=2;}if(len>=6){sizeHtml=4;}if(len>=9){sizeHtml=6;}
  cli->print("\" size=\"");cli->print(sizeHtml);cli->print("\" maxlength=\"");cli->print(len);cli->print("\" >");
  if(pol!=0){cli->print("</font>");}
  if(td==1 || td==3){cli->println("</td>");}
}

void xradioTableHtml(EthernetClient* cli,byte valeur,char* nomfonct,byte nbval,int nbli,byte type)
{
    for(int i=0;i<nbli;i++){
      char oi[]="OI";
      byte b,a=valeur; a=a >> i*2 ;b=a&0x01;a&=0x02;a=a>>1;          // mode periSwVal        a bit poids fort commande ; b bit poids faible état
      //Serial.print("swVal=");Serial.println(*periSwVal,HEX);
      for(int j=0;j<nbval;j++){
        if(type&0x02!=0){
          cli->print("<input type=\"radio\" name=\"");cli->print(nomfonct);cli->print((char)(i+48));cli->print("\" value=\"");cli->print((char)(PMFNCVAL+j));cli->print("\"");
          if(a==j){cli->print(" checked");}cli->print("/>");
        }
      }
      if(type&0x01!=0){cli->print(" ");cli->print(oi[b]);}
      cli->println("<br>");
    }
}

void checkboxTableHtml(EthernetClient* cli,uint8_t* val,char* nomfonct,int etat,uint8_t td)
{
  if(td==1 || td==2){cli->print("<td>");}
  cli->print("\n<input type=\"checkbox\" name=\"");cli->print(nomfonct);cli->print("\" id=\"cb1\" value=\"1\"");
  if((*val & 0x01)!=0){cli->print(" checked");}
  cli->print(">");
  if(etat>=0 && !(*val & 0x01)){etat=2;}
      switch(etat){
        case 2 :cli->print("___");break;
        case 1 :cli->print("_ON");break;
        case 0 :cli->print("OFF");break;
        default:break;
      }
  if(td==1 || td==3){cli->print("</td>");}
  cli->println();
}

void subDSn(EthernetClient* cli,char* fnc,uint8_t val,uint8_t num) // checkbox transportant 1 bit 
                                                                   // num le numéro du bit dans le byte
                                                                   // le caractère LENNOM-1 est le numéro du bit(+PMFNCHAR) dans periDetServ 

{
  char fonc[LENNOM+1];
  memcpy(fonc,fnc,LENNOM+1);
  fonc[LENNOM-1]=(char)(PMFNCHAR+num);
  val=(val>>num)&0x01;
  checkboxTableHtml(cli,&val,fonc,-1,0);
}

void subMPn(EthernetClient* cli,uint8_t sw,uint8_t num,uint8_t nb)   // numbox transportant une valeur avec fonction peri_pmo__
                                                                     // nb est le nombre de bits de la valeur
                                                                     // num le numéro du lsb dans le switch de periSwPulseCtl
                                                                     // le caractère LENNOM-2 est le numéro de sw sur 3 bits (+PMFNCHAR) et bit 0x10=1
                                                                     // le caractère LENNOM-1 est le numéro du lsb(+PMFNCHAR) dans periSwPulseCtl
                                                                     // seules les lettres maj et min sont utilisées (les car de ponctuation sont sautés)
{                                                                    // LENNOM-2  0100 0000 -> 0101 0000 -> 0101 0xxx
  char fonc[]="peri_pmo__\0";
  uint64_t pipm=0;memcpy(&pipm,((char*)(periSwPulseCtl+sw*DLSWLEN)),DLSWLEN);
  uint8_t val=(pipm>>num)&mask[nb];                          
  fonc[LENNOM-2]=(char)(PMFNCHAR+sw+0x10);      
  if(num>25){num+=6;}                                                 // '+6' pour skip ponctuations (valeur maxi utilisable 2*26=52)
  fonc[LENNOM-1]=(char)(PMFNCHAR+num);

  numTableHtml(cli,'b',(void*)&val,fonc,1,0,1);
}

void subMPc(EthernetClient* cli,uint8_t sw,uint8_t num)         // checkbox transportant un bit avec fonction peri_pmo__
                                                                // num le numéro du bit dans le switch de periSwPulseCtl
                                                                // le caractère LENNOM-2 est le numéro de sw sur 3 bits (+PMFNCHAR)
                                                                // le caractère LENNOM-1 est le numéro du bit(+PMFNCHAR) dans periSwPulseCtl 
{                                                               // LENNOM-2  0100 0000 -> 0100 0xxx
  char fonc[]="peri_pmo__\0";
  uint64_t pipm=0;memcpy(&pipm,((char*)(periSwPulseCtl+sw*DLSWLEN)),DLSWLEN);
  uint8_t val=(pipm>>num)&0x01;
  fonc[LENNOM-2]=(char)(PMFNCHAR+sw);
  if(num>26){num+=6;}                                            // '+6' pour skip ponctuations (valeur maxi utilisable 2*26=52)
  fonc[LENNOM-1]=(char)(PMFNCHAR+num);                     
  
  checkboxTableHtml(cli,&val,fonc,-1,0);
}

void subModePulseTime(EthernetClient* cli,uint8_t sw,uint32_t* pulse,uint32_t* dur,char* fonc1,char* fonc2,char onetwo)
{
  uint64_t pipm;memcpy(&pipm,((char*)(periSwPulseCtl+sw*DLSWLEN)),DLSWLEN);
  uint8_t val,pbit=PMTTE_PB;if(onetwo=='O'){pbit=PMTOE_PB;}
  val=((pipm>>pbit)&0x01)+PMFNCVAL;                                        
  cli->print("<font size=\"2\">");
  fonc1[LENNOM-1]=onetwo;
  checkboxTableHtml(cli,&val,fonc1,-1,0);                       // bit enable pulse
  numTableHtml(cli,'l',(pulse+sw),fonc2,8,0,2);                 // durée pulse   
  char a[8];sprintf(a,"%06d",*(dur+sw));a[6]='\0';              // valeur courante
  cli->print("<br>(");cli->print(a);cli->println(")</font>");
}

void SwCtlTableHtml(EthernetClient* cli,int nbsw,int nbtypes)
{
  htmlIntro(nomserver,cli);
            
  cli->println("<body><form method=\"get\" >");
  cli->println(VERSION);cli->print("  ");numTableHtml(cli,'i',&periCur,"peri_t_sw_",2,1,0); // pericur n'est pas modifiable (fixation pericur, periload, cberase)
  cli->print("-");cli->print(periNamer);cli->println("<br>");
  char pwd[32]="password__=\0";strcat(pwd,usrpass);lnkTableHtml(cli,pwd,"retour");
  cli->println(" <input type=\"submit\" value=\"MàJ\"><br>");

  cli->println("<table>");
  cli->println("<tr><th>sw</th><th>time One<br>time Two</th><th>f<br>r</th><th>e.l _f_H.a<br>n.x _t_L.c</th><th>0-3<br>___det__srv._pul</th></tr>");
  
  
  char nfonc[]="peri_imn__\0";            // transporte le numéro de detecteur des sources
  char cfonc[]="peri_imc__\0";            // transporte la valeur des bits des sources
  char pfonc[]="peri_pto__\0";            // transporte la valeur pulse time One
  char qfonc[]="peri_ptt__\0";            // transporte la valeur pulse time Two
  char rfonc[]="peri_otf__\0";            // transporte les bits freerun et enable pulse de periPulseMode (LENNOM-1= ,'F','O','T')

  char nac[]="ADIO";                      // nom du type d'acttion (activ/désactiv/ON/OFF)
   
    for(int i=0;i<nbsw;i++){                                           // i n° de switch

      cli->print("<tr><td>");cli->print(i);cli->print("</td>");

      nfonc[LENNOM-2]=(char)(i+48);
      cfonc[LENNOM-2]=(char)(i+48);
      rfonc[LENNOM-2]=(char)(i+PMFNCHAR);
      pfonc[LENNOM-2]=(char)(i+PMFNCHAR);
      qfonc[LENNOM-2]=(char)(i+PMFNCHAR);
      
// colonne des durées de pulses

      cli->print("<td>");
      subModePulseTime(cli,i,periSwPulseOne,periSwPulseCurrOne,rfonc,pfonc,'O');          // bit et valeur pulse one
      cli->print("<br>");
      subModePulseTime(cli,i,periSwPulseTwo,periSwPulseCurrTwo,rfonc,qfonc,'T');          // bit et valeur pulse two

// colonne freerun

      cli->print("</td><td>");
      uint64_t pipm;memcpy(&pipm,(char*)(periSwPulseCtl+i*DLSWLEN),DLSWLEN);
      uint8_t val=((pipm>>PMFRO_PB)&0x01)+PMFNCVAL;rfonc[LENNOM-1]='F';                    // bit freerun
      checkboxTableHtml(cli,&val,rfonc,-1,0);                    
      cli->print("<br>");cli->print((char)periSwPulseSta[i]);cli->print("</td>");          // staPulse 
                                                                 
// colonne des détecteurs

      cli->print("<td><font size=\"1\">");
      for(int nd=0;nd<DLNB;nd++){
        subMPc(cli,i,nd*DLBITLEN+DLENA_PB);          // enable
        subMPc(cli,i,nd*DLBITLEN+DLEL_PB);           // local/externe
        subMPn(cli,i,nd*DLBITLEN+DLNLS_PB,DLNULEN);  // numéro det
        subMPc(cli,i,nd*DLBITLEN+DLMFE_PB);          // flanc/trans
        subMPc(cli,i,nd*DLBITLEN+DLMHL_PB);          // H/L
        subMPn(cli,i,nd*DLBITLEN+DLACLS_PB,DLACLEN); // numéro action
        if(nd<DLNB-1){cli->print("<br>");}                           
      }
      cli->print("</font></td>");
      
// colonne des actionneurs des switchs

      cli->print("<td>");                                               // colonne des types d'action  
      for(int k=0;k<nbtypes;k++){                                       // k n° de type d'action (act/des/ON/OFF) (1 ligne par type)
        nfonc[LENNOM-1]=(char)(k*16+64);
        uint8_t val=(*(periSwMode+i*MAXTAC+k)>>SWMDLNULS_PB);           // 4 bytes par sw ; 2 bits gauche n° détecteur + 3*2 bits (enable - H/L)
        cli->print("<font size=\"2\">");cli->print(nac[k]);cli->print("</font>");
        numTableHtml(cli,'b',(uint32_t*)&val,nfonc,1,0,2);              // affichage-saisie n°détec
        
        for(int j=5;j>=0;j--){                                          // 3*2 checkbox enable+on/off pour les 3 sources d'un des 4 types de déclenchement du switch 
          cfonc[LENNOM-1]=(char)(k*16+j+64);                            // codage nom de fonction aaaaaaasb ; s n° de switch (0-3) ; b=01tt0bbb tt type ; bbb n° bit (0x40+0x30+0x07)
          uint8_t valb=((*(periSwMode+i*MAXTAC+k)>>j) & 0x01);                        
          //Serial.print("     *chk* ");Serial.print(cfonc);Serial.print(" ");Serial.println(valb,HEX);
          checkboxTableHtml(cli,&valb,cfonc,-1,0);
        }
        if(k<nbtypes-1){cli->print("<br>");}                            // fin ligne types
      }
      cli->print("<br></td>");                                          // fin colonne types
    cli->print("</tr>"); 
    } // switch suivant
   cli->print("</form></body></html>");
}

void periTableHtml(EthernetClient* cli)
{
  int i,j;

Serial.print("début péritable ; remote_IP ");serialPrintIp(remote_IP_cur);Serial.println();

  htmlIntro(nomserver,cli);

  
  char bufdate[15];alphaNow(bufdate);
  byte msb=0,lsb=0;                        // pour temp DS3231
  readDS3231temp(&msb,&lsb);

        cli->println("<body>");cli->println("<form method=\"GET \">");

          cli->print(VERSION);
          #ifdef _MODE_DEVT
            cli->print(" _MODE_DEVT ");
          #endif _MODE_DEVT
          for(int zz=0;zz<14;zz++){cli->print(bufdate[zz]);if(zz==7){cli->print("-");}}
          cli->println(" GMT ; local IP ");cli->print(Ethernet.localIP());cli->println(" ");
          cli->print(msb);cli->print(".");cli->print(lsb);cli->println("°C<br>");

          cli->print("<input type=\"password\" name=\"macmaster_\" value=\"\" size=\"6\" maxlength=\"8\" > ");
          lnkTableHtml(cli,"reset_____","reset");
          char pwd[32]="password__=\0";strcat(pwd,modpass);lnkTableHtml(cli,pwd,"refresh");
          lnkTableHtml(cli,"cfgserv___","config");

          numTableHtml(cli,'d',&perrefr,"per_refr__",4,0,0);cli->println("<input type=\"submit\" value=\"ok\">");
          lnkTableHtml(cli,"testhtml__","test_html");
          lnkTableHtml(cli,"dump_sd___","dump SDcard");
          cli->print("(");long sdsiz=fhisto.size();cli->print(sdsiz);cli->println(") ");
          numTableHtml(cli,'i',(uint32_t*)&sdpos,"sd_pos____",9,0,0);cli->print("<input type=\"submit\" value=\"ok\"> détecteurs serveur :");

          for(int k=0;k<NBDSRV;k++){subDSn(cli,"mem_dsrv__\0",memDetServ,k);}
          cli->println("<br>");
          

          cli->println("<table>");
              cli->println("<tr>");
                //cli->println(" ON=VRAI=1=HAUT=CLOSE=GPIO2=ROUGE");
                cli->println("<th></th><th><br>nom_periph</th><th><br>TH</th><th><br>  V </th><th>per<br>pth<br>ofs</th><th>nb<br>sd<br>pg</th><th>nb<br>sw</th><th><br>_O_I___</th><th>nb<br>dt</th><th></th><th>mac_addr<br>ip_addr</th><th>version DS18x<br>last out<br>last in</th><th></th><th>det<br>srv<br>en</th>"); //<th>time One<br>time Two</th><th>f<br>r</th><th>e.l _f_H.a<br>n.x _t_L.c</th><th>___det__srv._pul<br></th>");
              cli->println("</tr>");

              for(i=1;i<=NBPERIF;i++){
                // !!!!!!!!!!!!!!!!!! pericur doit étre le premier de la liste !!!!!!!!!!!!!!!!!!!!!!!!!
                // pour permettre periLoad préalablement aux mises à jour des données quand le navigateur 
                // envoie une commande GET/POST 
                // et pour assurer l'effacement des bits de checkbox : le navigateur ne renvoie que ceux "checkés"
                periInitVar();periLoad(i);periCur=i;
                if(*periSwNb>MAXSW){periCheck(i,"perT");periInitVar();periSave(i);}
                if(*periDetNb>MAXDET){periCheck(i,"perT");periInitVar();periSave(i);}

                cli->println("<tr>");
                  cli->println("<form method=\"GET \">");
                      
                      numTableHtml(cli,'i',&periCur,"peri_cur__",2,1,0);
                      cli->print("<td><input type=\"text\" name=\"peri_nom__\" value=\"");
                         cli->print(periNamer);cli->print("\" size=\"12\" maxlength=\"");cli->print(PERINAMLEN-1);cli->print("\" ></td>");
                     // boutonHtml(cli,periSwVal,"peri_swb__",0,2);
                      textTableHtml(cli,'f',periLastVal,periThmin,periThmax,1,1);
                      numTableHtml(cli,'f',periThmin,"peri_thmin",5,0,0);cli->print("<br>");
                      numTableHtml(cli,'f',periThmax,"peri_thmax",5,3,0);
                      textTableHtml(cli,'f',periAlim,periVmin,periVmax,1,1);
                      numTableHtml(cli,'f',periVmin,"peri_vmin_",5,0,0);cli->print("<br>");
                      numTableHtml(cli,'f',periVmax,"peri_vmax_",5,3,0);
                      numTableHtml(cli,'l',(uint32_t*)periPerRefr,"peri_refr_",5,2,0);cli->print("<br>");
                      numTableHtml(cli,'f',periPitch,"peri_pitch",5,0,0);cli->print("<br>");
                      numTableHtml(cli,'f',periThOffset,"peri_tofs_",5,3,0);
                      numTableHtml(cli,'b',periSondeNb,"peri_sonde",2,2,0);cli->print("<br>");
                      checkboxTableHtml(cli,(uint8_t*)periProg,"peri_prog_",-1,3);
                      numTableHtml(cli,'b',periSwNb,"peri_intnb",1,1,0);
                      cli->println("<td>");
                      xradioTableHtml(cli,*periSwVal,"peri_intv\0",2,*periSwNb,3);
                      numTableHtml(cli,'b',periDetNb,"peri_detnb",1,1,0);
                      cli->print("<td>");
                      for(uint8_t k=0;k<*periDetNb;k++){char oi[2]={'O','I'};cli->print(oi[(*periDetVal>>(k*2))&DETBITLH_VB]);if(k<*periDetNb-1){cli->print("<br>");}}
                      cli->println("</td>");
                      cli->print("<td><input type=\"text\" name=\"peri_mac__\" value=\"");for(int k=0;k<6;k++){cli->print(chexa[periMacr[k]/16]);cli->print(chexa[periMacr[k]%16]);}
                        cli->println("\" size=\"11\" maxlength=\"12\" ><br><br>");
                      cli->print("<font size=\"2\">");for(j=0;j<4;j++){cli->print(periIpAddr[j]);if(j<3){cli->print(".");}}cli->println("</font></td>");
                      cli->print("<td><font size=\"2\">");for(j=0;j<LENVERSION;j++){cli->print(periVers[j]);}cli->println("<br>");
                      char dateascii[12];
                      unpackDate(dateascii,periLastDateOut);for(j=0;j<12;j++){cli->print(dateascii[j]);if(j==5){cli->print(" ");}}cli->println("<br>");
                      unpackDate(dateascii,periLastDateIn);for(j=0;j<12;j++){cli->print(dateascii[j]);if(j==5){cli->print(" ");}}
                        cli->println("</font></td>");
                      
                      cli->println("<td><input type=\"submit\" value=\"MàJ\"><br>");
                      char swf[]="switchs___";swf[LENNOM-1]=periCur+PMFNCHAR;swf[LENNOM]='\0';lnkTableHtml(cli,swf,"switchs");cli->println("</td>");

                      cli->println("<td>");for(int k=0;k<NBDSRV;k++){
                        subDSn(cli,"peri_dsv__\0",*periDetServEn,k);if(k%2==1 && k<NBDSRV-1){cli->println("<br>");}}
                      cli->println("</td>");


                      
                      //SwCtlTableHtml(cli,*periSwNb,4);

                  cli->print("</form>");
                cli->println("</tr>");
              }
          cli->println("</table>");
        cli->println("</form></body></html>");
Serial.println("fin péritable");
}

void cbErase()      // ******************** effacement des bits checkbox ********************* 
                    // seuls les bits des switchs existants sont effacés pour accélérer la séquence
{
        uint64_t sh0,sh1,shx;
        *periProg=0;

        /* periSwPulseCtl */                                                                               
        if(*periSwNb!=0){
          sh0=PMFRO_VB+PMTOE_VB+PMTTE_VB;                            // mask des bits / switch 
          sh1=DLMHL_VB+DLMFE_VB+DLEL_VB+DLENA_VB;                    // mask des bits / switch / détecteur 
          //dumpstr((char*)&sh0,8);dumpstr((char*)&sh1,8);Serial.println();
          for(int det=0;det<DLNB;det++){sh0+=(sh1<<(DLBITLEN*det));           // sh0 complété avec bits tous détecteurs
          shx=sh1<<(DLBITLEN*det);//dumpstr((char*)&shx,8);
          }
          //Serial.println();dumpstr((char*)&sh0,16);shx=~sh0;dumpstr((char*)&shx,8);Serial.println();
                                            //   00011111 1110000111 1110000111 1110000111 1110000111
                                            //  0001 1111 1110 0001 1111 1000 0111 1110 0001 1111 1000 0111
                                            //    1   F     E   1     F   8     7   E     1   F     8   7
                                            //   11100000 0001111000 0001111000 0001111000 0001111000
                                            //  1110 0000 0001 1110 0000 0111 1000 0001 1110 0000 0111 1000
                                            //    E   0     1   E     0   7     8   1     E   0     7   8
          for(int sw=0;sw<*periSwNb;sw++){                                    // pour chaque switch effacement octet par octet 
            for(int bb=0;bb<DLSWLEN;bb++){*(periSwPulseCtl+sw*DLSWLEN+bb) &= ~*((byte*)&sh0+bb);//}    // effacement
            //Serial.print((byte)*(periSwPulseCtl+sw*DLSWLEN+bb),HEX);Serial.print(" ");Serial.print((byte)(~*((byte*)&sh0+bb)),HEX);Serial.print("   ");
          }//Serial.println();
          }//Serial.println();
        }

        /* periSwMode */
        byte sh=SWMDLEN_VB+SWMDLHL_VB+SWMSEN_VB+SWMSHL_VB+SWMPEN_VB+SWMPHL_VB; // mask des bits / mode
        for(int sw=0;sw<(*periSwNb)*MAXTAC;sw++){*(periSwMode+sw)&= ~sh;}      // effacement 

        /* periDetServ */
        *periDetServEn=0;
}

