#include <Arduino.h>
#include <SPI.h>      //bibliothéqe SPI pour W5100
#include <Ethernet.h>
#include <SD.h>
#include <shutil.h>
#include <shconst.h>
#include "const.h"
#include "periph.h"
#include "httphtml.h"
#include "utilether.h"

#ifndef WEMOS
  #include <avr/wdt.h>  //biblio watchdog
#endif ndef WEMOS

extern File fhisto;      // fichier histo sd card
extern long sdpos;
extern char strSD[RECCHAR];

extern EthernetClient cli;

extern int   perrefr;
extern char* chexa;

extern uint8_t remote_IP[4],remote_IP_cur[4];

extern char      periRec[PERIRECLEN];          // 1er buffer de l'enregistrement de périphérique
  
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
extern byte*     periMacr;                     // ptr ds buffer : mac address 
extern byte*     periIpAddr;                   // ptr ds buffer : Ip address
extern byte*     periSwNb;                    // ptr ds buffer : Nbre d'interrupteurs (0 aucun ; maxi 4(MAXSW)            
extern byte*     periSwVal;                   // ptr ds buffer : état/cde des inter  
extern byte*     periSwMode;                  // ptr ds buffer : Mode fonctionnement inters (4 bytes par switch)           
extern uint32_t* periSwPulseOne;               // ptr ds buffer : durée pulses sec ON (0 pas de pulse)
extern uint32_t* periSwPulseTwo;              // ptr ds buffer : durée pulses sec OFF(mode astable)
extern uint32_t* periSwPulseCurrOne;           // ptr ds buffer : temps courant pulses ON
extern uint32_t* periSwPulseCurrTwo;          // ptr ds buffer : temps courant pulses OFF
extern byte*     periSwPulseCtl;             // ptr ds buffer : mode pulses 
extern uint8_t*  periSondeNb;                  // ptr ds buffer : nbre sonde
extern boolean*  periProg;                     // ptr ds buffer : flag "programmable"
extern byte*     periDetNb;                    // ptr ds buffer : Nbre de détecteurs maxi 4 (MAXDET)
extern byte*     periDetVal;                   // ptr ds buffer : flag "ON/OFF" si détecteur (2 bits par détec))
extern float*    periThOffset;                 // ptr ds buffer : offset correctif sur mesure température
extern int8_t    periMess;                     // code diag réception message (voir MESSxxx shconst.h)
extern byte      periMacBuf[6]; 

extern void init_params();
extern int  chge_pwd; //=FAUX;

extern byte mask[];

void cliPrintMac(EthernetClient* cli, byte* mac)
{
  char macBuff[18];
  unpackMac(macBuff,mac);
  cli->print(macBuff);
}

void htmlIntro0(EthernetClient* cli)    // suffisant pour commande péripheriques
{
  cli->println("HTTP/1.1 200 OK");
  //cli->println("Location: http://82.64.32.56:1789/");
  //cli->println("Cache-Control: private");
  cli->println("CONTENT-Type: text/html; charset=UTF-8");
  cli->println("Connection: close\n");
  cli->println("<!DOCTYPE HTML ><html>");
}

void htmlIntro(char* titre,EthernetClient* cli)
{
  htmlIntro0(cli);

  cli->println("<head>");
  char buf[10]={0};
  if(perrefr!=0){cli->print("<meta HTTP-EQUIV=\"Refresh\" content=\"");sprintf(buf,"%d",perrefr);cli->print(buf);cli->print("\">");}
  cli->print("\<title>");cli->print(titre);cli->println("</title>");
            cli->println("<style>");
            cli->println("table {");
              cli->println("font-family: Courier, sans-serif;");
              cli->println("border-collapse: collapse;");
              cli->println("width: 100%;");
              cli->println("overflow: auto;");
              cli->println("white-space:nowrap;"); 
            cli->println("}");

            cli->println("td, th {");
              cli->println("border: 1px solid #dddddd;");
              cli->println("text-align: left;"); 
            cli->println("}");

            cli->println("#nt1{width:10px;}");
            cli->println("#nt2{width:18px;}");
            cli->println("#cb1{width:10px; padding:0px; margin:0px; text-align: center};");
            cli->println("#cb2{width:20px; text-align: center};");

          cli->println("</style>");
  cli->println("</head>");
}

int dumpsd()                 // liste le fichier de la carte sd
{
  char inch=0;
  
  if(sdOpen(FILE_READ,&fhisto,"fdhisto.txt")==SDKO){return SDKO;}
  
  long sdsiz=fhisto.size();
  long pos=fhisto.position();
  fhisto.seek(sdpos);
  
  Serial.print("histoSD ");Serial.println(sdsiz);

  htmlIntro(NOMSERV,&cli);cli.println("<body>");

  //char strds[5];strds[0]=';';sprintf(strds+1,"%d",fdatasave);strcat(strds," \0");
  cli.print("histoSD ");cli.print(sdsiz);cli.print(" ");cli.print(pos);cli.print("/");cli.print(sdpos);cli.print("<br>\n");
  //if(sddata){cli.print(" filtre \"");cli.print(strds);cli.print("\"");}
  
  int cnt=0,strSDpt=0;
  int ignore=VRAI; // ignore until next line
  long ptr=sdpos;
  while (ptr<sdsiz || inch==-1)
    {
    inch=fhisto.read();ptr++;
#ifndef WEMOS
    cnt++;if(cnt>5000){wdt_reset();cnt=0;}  
#endif ndef WEMOS
    if(!ignore)
      {
      //cli.print(inch);
        
      strSD[strSDpt]=inch;strSDpt++;strSD[strSDpt]='\0';
      if((strSDpt>=(RECCHAR-1)) || (strSD[strSDpt-2]=='\r' && strSD[strSDpt-1]=='\n')){
        cli.println(strSD);                                                                                                                                                                                                        
        memset(strSD,'\0',RECCHAR);strSDpt=0;
        Serial.print(inch);
        }
      }
      else if (inch==10){ignore=FAUX;}
    }
  cli.println(strSD);

  cli.println("</body>");

  return sdOpen(FILE_WRITE,&fhisto,"fdhisto.txt");
  return SDOK;
}

void lnkTableHtml(EthernetClient* cli,char* nomfonct,char* lib)
{
  cli->print("<a href=page.html?");cli->print(nomfonct);cli->print(": target=_self>");
  cli->print(lib);cli->println("</a>");
}

void numTableHtml(EthernetClient* cli,char type,void * valfonct,char* nomfonct,int len,uint8_t td,int pol)
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

void subMPn(EthernetClient* cli,uint8_t sw,uint8_t num,uint8_t nb)   // numbox transportant une valeur avec fonction peri_pmo__
                                                                // nb est le nombre de bits de la valeur
                                                                // num le numéro du lsb dans le switch de periSwPulseCtl
                                                                // le caractère LENNOM-2 est le numéro de sw sur 3 bits (+PMFNCHAR) et bit 0x10=1
                                                                // le caractère LENNOM-1 est le numéro du lsb(+PMFNCHAR) dans periSwPulseCtl 
{
  char fonc[]="peri_pmo__\0";
  uint64_t pipm;memcpy(&pipm,((char*)(periSwPulseCtl+sw*PMSWLEN)),PMSWLEN);
  uint8_t val=(pipm>>num)&mask[nb];                          
  fonc[LENNOM-2]=(char)(PMFNCHAR+sw);      
  fonc[LENNOM-1]=(char)(PMFNCHAR+num);

  numTableHtml(cli,'b',(void*)&val,fonc,1,0,1);
}

void subMPc(EthernetClient* cli,uint8_t sw,uint8_t num)         // checkbox transportant un bit avec fonction peri_pmo__
                                                                // num le numéro du bit dans le switch de periSwPulseCtl
                                                                // le caractère LENNOM-2 est le numéro de sw sur 3 bits (+PMFNCHAR)
                                                                // le caractère LENNOM-1 est le numéro du bit(+PMFNCHAR) dans periSwPulseCtl 
{
  char fonc[]="peri_pmo__\0";
  uint64_t pipm;memcpy(&pipm,((char*)(periSwPulseCtl+sw*PMSWLEN)),PMSWLEN);
  uint8_t val=(pipm>>num)&0x01;
  fonc[LENNOM-2]=(char)(PMFNCHAR+sw);
  fonc[LENNOM-1]=(char)(PMFNCHAR+num);                     
  
  checkboxTableHtml(cli,&val,fonc,-1,0);
}

void subModePulseTime(EthernetClient* cli,uint8_t sw,uint32_t* pulse,uint32_t* dur,char* fonc1,char* fonc2,char onetwo)
{
  uint64_t pipm;memcpy(&pipm,((char*)(periSwPulseCtl+sw*PMSWLEN)),PMSWLEN);
  uint8_t val,pbit=PMTTE_PB;if(onetwo=='O'){pbit=PMTOE_PB;}
  val=((pipm>>pbit)&0x01)+PMFNCVAL;                                        
  cli->print("<font size=\"2\">");
  fonc1[LENNOM-1]=onetwo;
  checkboxTableHtml(cli,&val,fonc1,-1,0);                       // bit enable pulse
  numTableHtml(cli,'l',(pulse+sw*PMSWLEN),fonc2,8,0,2);                 // durée pulse   
  char a[8];sprintf(a,"%06d",*(dur+sw*PMSWLEN));a[6]='\0';              // valeur courante
  cli->print("<br>(");cli->print(a);cli->println(")</font>");
}

void SwCtlTableHtml(EthernetClient* cli,int nbsw,int nbtypes)
{
  char nfonc[]="peri_imn__\0";            // transporte le numéro de detecteur des sources
  char cfonc[]="peri_imc__\0";            // transporte la valeur des bits des sources
  char pfonc[]="peri_pto__\0";            // transporte la valeur pulse time One
  char qfonc[]="peri_ptt__\0";            // transporte la valeur pulse time Two
  char rfonc[]="peri_sfp__\0";            // transporte les bits freerun et enable pulse de periPulseMode (LENNOM-1= ,'F','O','T')

  char nac[]="ADIO";                      // nom du type d'acttion (activ/désactiv/ON/OFF)
   
    for(int i=0;i<nbsw;i++){                                           // i n° de switch

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
      uint64_t pipm;memcpy(&pipm,(char*)(periSwPulseCtl+i*PMSWLEN),PMSWLEN);
      uint8_t val=((pipm>>PMFRO_PB)&0x01)+PMFNCVAL;rfonc[LENNOM-1]='F';                    // bit freerun
      //Serial.print(val);Serial.print(" - ");dumpstr((periSwPulseCtl+i*PMSWLEN),6);
      checkboxTableHtml(cli,&val,rfonc,-1,0);                    
      cli->print("</td>");
                                                                 
// colonne des détecteurs

      cli->print("<td><font size=\"1\">");
      for(int nd=0;nd<PMNBDET;nd++){
        subMPc(cli,i,nd*PMDLEN+PMDEN_PB);         // enable
        subMPc(cli,i,nd*PMDLEN+PMDLE_PB);         // local/externe
        subMPn(cli,i,nd*PMDLEN+PMDNLS_PB,PMDLNU); // numéro det
        subMPc(cli,i,nd*PMDLEN+PMDMO_PB);         // flanc/trans
        subMPc(cli,i,nd*PMDLEN+PMDHL_PB);         // H/L
        subMPn(cli,i,nd*PMDLEN+PMDALS_PB,PMDLAC); // numéro action
        if(nd<PMNBDET-1){cli->print("<br>");}                           
      }
      cli->print("</font></td>");
      
// colonne des actionneurs des switchs

      cli->print("<td>");                                               // colonne des types d'action  
      for(int k=0;k<nbtypes;k++){                                       // k n° de type d'action (act/des/ON/OFF) (1 ligne par type)
        nfonc[LENNOM-1]=(char)(k*16+64);
        uint8_t val=(*(periSwMode+i*MAXTAC+k)>>6);                     // 4 bytes par sw ; 2 bits gauche n° détecteur + 3*2 bits (enable - H/L)
        cli->print("<font size=\"2\">");cli->print(nac[k]);cli->print("</font>");
        numTableHtml(cli,'b',(uint32_t*)&val,nfonc,1,0,2);              // affichage-saisie n°détec
        
        for(int j=5;j>=0;j--){                                          // 3*2 checkbox enable+on/off pour les 3 sources d'un des 4 types de déclenchement du switch 
          cfonc[LENNOM-1]=(char)(k*16+j+64);                            // codage nom de fonction aaaaaaasb ; s n° de switch (0-3) ; b=01tt0bbb tt type ; bbb n° bit (0x40+0x30+0x07)
          uint8_t valb=((*(periSwMode+i*MAXTAC+k)>>j) & 0x01);                        
          //Serial.print("     *chk* ");Serial.print(cfonc);Serial.print(" ");Serial.println(valb,HEX);
          checkboxTableHtml(cli,&valb,cfonc,-1,0);
        }
        if(k<nbtypes-1){cli->print("<br>");}                            // fin ligne typez
      }
      cli->print("<br></td>");                                          // fin colonne types

    } // switch suivant
}

void periTableHtml(EthernetClient* cli)
{
  int i,j;
    
Serial.print("début péritable ; remote_IP ");serialPrintIp(remote_IP_cur);Serial.println();

  htmlIntro(NOMSERV,cli);
  char bufdate[15];alphaNow(bufdate);
  byte msb=0,lsb=0;                        // pour temp DS3231
  readDS3231temp(&msb,&lsb);

        cli->println("<body>");cli->println("<form method=\"POST \">");

          cli->print(VERSION);
          #ifdef _MODE_DEVT
            cli->print(" _MODE_DEVT ");
          #endif _MODE_DEVT
          //cli->println("<br>");
          for(int zz=0;zz<12;zz++){cli->print(bufdate[zz]);if(zz==7){cli->print("-");}}
          cli->println(" GMT<br>");
          cli->print("local IP ");cli->print(Ethernet.localIP());//cli->println("<br>");
          cli->print(" - temp 3231 ");cli->print(msb);cli->print(".");cli->print(lsb);cli->println("°C<br>");
          
          lnkTableHtml(cli,"reset_____","reset");

          lnkTableHtml(cli,"peri_table","refresh");
          numTableHtml(cli,'i',(uint32_t*)&perrefr,"per_refr__",4,0,0);cli->println("<input type=\"submit\" value=\"ok\">");

          lnkTableHtml(cli,"test2sw___","testsw");
          
          lnkTableHtml(cli,"dump_sd___","dump SDcard");
          cli->print("(");long sdsiz=fhisto.size();cli->print(sdsiz);cli->println(") ");
          numTableHtml(cli,'i',(uint32_t*)&sdpos,"sd_pos____",9,0,0);cli->print("<input type=\"submit\" value=\"ok\">");cli->println("<br>");

          cli->println("<table>");
              cli->println("<tr>");
                cli->println(" ON=VRAI=1=HAUT=CLOSE=GPIO2=ROUGE");
                cli->println("<th></th><th><br>nom_periph</th><th><br>TH</th><th><br>  V </th><th>per<br>pth<br>ofs</th><th>nb<br>sd<br>pg</th><th>nb<br>sw</th><th><br>_O_I___</th><th>nb<br>dt</th><th></th><th>mac_addr<br>ip_addr</th><th>version<br>last out<br>last in</th><th></th><th>time One<br>time Two</th><th>f<br>r</th><th>e.l _f_H.a<br>n.x _t_L.c</th><th>___det___srv_pul<br></th>");
              cli->println("</tr>");

              for(i=1;i<=NBPERIF;i++){
                // !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!! pericur doit étre le premier de la liste !!!!!!!!!!!!!!!!!!!!!!!!!
                // car l'enregistrement de la table va étre rechargé avant mise à jour
                periInit();periCur=i;periLoad(i);if(*periSwNb>4){*periSwNb=4;periSave(i);}

/*                Serial.print(i);Serial.print(" ");Serial.print(periNamer);Serial.print(" ");Serial.print(*periPerRefr);Serial.print(" ");
                Serial.print(*periPitch);Serial.print(" ");Serial.print(*periLastVal);Serial.print(" ");Serial.print(*periLastDate);Serial.print(" ");
                Serial.print(*periSwNb);Serial.print(" ");Serial.print(*periDetNb);
                Serial.print(" ");serialPrintIp(periIpAddr);Serial.print(" ");serialPrintMac(periMacr);
*/
                cli->println("<tr>");
                  cli->println("<form method=\"POST\">");
                      numTableHtml(cli,'i',&periCur,"peri_cur__",2,1,0);
                      cli->print("<td><input type=\"text\" name=\"peri_nom__\" value=\"");
                         cli->print(periNamer);cli->print("\" size=\"12\" maxlength=\"");cli->print(PERINAMLEN-1);cli->print("\" ></td>");
                      cli->print("<td>");cli->print(*periLastVal);cli->print("</td>");
                      cli->print("<td>");cli->print(*periAlim);cli->println("</td>");
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
                      for(uint8_t k=0;k<*periDetNb;k++){char oi[2]={'O','I'};byte b=*periDetVal; b=b >> k*2;b&=0x01;cli->print(oi[b]);if(k<*periDetNb-1){cli->print("<br>");}}
                      cli->println("</td>");
                      cli->print("<td><input type=\"text\" name=\"peri_mac__\" value=\"");for(int k=0;k<6;k++){cli->print(chexa[periMacr[k]/16]);cli->print(chexa[periMacr[k]%16]);}
                         ;cli->println("\" size=\"11\" maxlength=\"12\" ><br><br>");
                      cli->print("<font size=\"2\">");for(j=0;j<4;j++){cli->print(periIpAddr[j]);if(j<3){cli->print(".");}}cli->println("</font></td>");
                      cli->print("<td><font size=\"2\">");for(j=0;j<3;j++){cli->print(periVers[j]);}cli->println("<br>");
                      char dateascii[12];
                      unpackDate(dateascii,periLastDateIn);for(j=0;j<12;j++){cli->print(dateascii[j]);if(j==5){cli->print(" ");}}cli->println("<br>");
                      unpackDate(dateascii,periLastDateOut);for(j=0;j<12;j++){cli->print(dateascii[j]);if(j==5){cli->print(" ");}}
                         cli->println("</font></td>");
                      
                      cli->println("<td><input type=\"submit\" value=\"MàJ\"></td>");
                      SwCtlTableHtml(cli,*periSwNb,4);

                  cli->print("</form>");
                cli->println("</tr>");
              }
          cli->println("</table>");
        cli->println("</form></body></html>");
Serial.println("fin péritable");
}

void acceuilHtml(EthernetClient* cli,bool passok)
{

          if(!passok){
            Serial.println(" saisie pwd");
            init_params();chge_pwd=VRAI;
            htmlIntro(NOMSERV,cli);

 
            cli->println(VERSION);
            cli->println("<body><form method=\"get\" ><p><label for=\"password\">password</label> : <input type=\"password\"");
            cli->println(" name=\"password__\" id=\"password\" value=\"\" size=\"6\" maxlength=\"8\" ></p></form></body></html>");
          }
          else periTableHtml(cli);
}          



