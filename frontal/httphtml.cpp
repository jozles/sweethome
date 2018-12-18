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
extern byte*     periIntNb;                    // ptr ds buffer : Nbre d'interrupteurs (0 aucun ; maxi 4(MAXSW)            
extern byte*     periIntVal;                   // ptr ds buffer : état/cde des inter  
extern byte*     periIntMode;                  // ptr ds buffer : Mode fonctionnement inters (4 bytes par switch)           
extern uint32_t* periIntPulseOn;               // ptr ds buffer : durée pulses sec ON (0 pas de pulse)
extern uint32_t* periIntPulseOff;              // ptr ds buffer : durée pulses sec OFF(mode astable)
extern uint32_t* periIntPulseCurrOn;           // ptr ds buffer : temps courant pulses ON
extern uint32_t* periIntPulseCurrOff;          // ptr ds buffer : temps courant pulses OFF
extern uint16_t* periIntPulseMode;             // ptr ds buffer : mode pulses 
extern uint8_t*  periSondeNb;                  // ptr ds buffer : nbre sonde
extern boolean*  periProg;                     // ptr ds buffer : flag "programmable"
extern byte*     periDetNb;                    // ptr ds buffer : Nbre de détecteurs maxi 4 (MAXDET)
extern byte*     periDetVal;                   // ptr ds buffer : flag "ON/OFF" si détecteur (2 bits par détec))
extern int8_t    periMess;                     // code diag réception message (voir MESSxxx shconst.h)
extern byte      periMacBuf[6]; 




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
              cli->println("padding: 2px;");
              cli->println("padding-right: 2px;"); 
            cli->println("}");

            cli->println("#nt1{width:10px;}");
            cli->println("#nt2{width:18px;}");
            cli->println("#cb1{width:10px; text-align: center};");
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

void numTableHtml(EthernetClient* cli,char type,void * valfonct,char* nomfonct,int len,bool td)
{                          
  if(td){cli->print("<td>");}
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
  if(td){cli->println("</td>");}
}

void xradioTableHtml(EthernetClient* cli,byte valeur,char* nomfonct,byte nbval,int nbli,byte type)
{
    for(int i=0;i<nbli;i++){
      char oi[]="OI";
      byte b,a=valeur; a=a >> i*2 ;b=a&0x01;a&=0x02;a=a>>1;          // mode periIntVal        a bit poids fort commande ; b bit poids faible état
      for(int j=0;j<nbval;j++){
        if(type&0x02!=0){
          cli->print("<input type=\"radio\" name=\"");cli->print(nomfonct);cli->print((char)(i+48));cli->print("\" value=\"");cli->print((char)(48+j));cli->print("\"");
          if(a==j){cli->print(" checked");}cli->print("/>");
        }
      }
      if(type&0x01!=0){cli->print(" ");cli->print(oi[b]);}
      cli->println("<br>");
    }
}

void checkboxTableHtml(EthernetClient* cli,uint8_t* nom,char* nomfonct,int etat,bool td)
{
  if(td){cli->print("<td>");}
  cli->print("\n<input type=\"checkbox\" name=\"");cli->print(nomfonct);cli->print("\" id=\"cb1\" value=\"1\"");
  if((*nom & 0x01)!=0){cli->print(" checked");}
  cli->print(">");
  if(etat>=0 && !(*nom & 0x01)){etat=2;}
      switch(etat){
        case 2 :cli->print("___");break;
        case 1 :cli->print("_ON");break;
        case 0 :cli->print("OFF");break;
        default:break;
      }
  if(td){cli->println("</td>");}
}


void subMPn(EthernetClient* cli,uint8_t sw,uint8_t num)          // n° source dans les 16 bits du PulseMode
{
                                  // structure nom fonction peri_pmoxy x="_" si checkbox
                                  //                                   sinon x=0x40 | 0x30 | 0x07 ; 0x30 num sw (0 à 3); 0x07 num source (1 à 7) 
                                  //                                   y="_" si num source
                                  //                                   sinon y=0x40 | 0x30 | 0x07 ; 0x30 num sw (0 à 3); 0x07 num bit (1 à 6)
                                  // 
                                  // structure PulseMode (num source 1-3 0=disable)
                                  // byte gauche xx332211 
                                  //        xx dispo
                                  //        33 poids faible (bits 1,0) num source 3
                                  //        22 poids faible (bits 1,0) num source 2
                                  //        11 poids faible (bits 1,0) num source 1
                                  // byte droite 321abcdef
                                  //        3 poids fort (bit 2) num source 3
                                  //        2 poids fort (bit 2) num source 2
                                  //        1 poids fort (bit 2) num source 1
                                  //        a enable pulse
                                  //        b ON/OFF cde serveur
                                  //        c UP/DOWN source 3
                                  //        d UP/DOWN source 2
                                  //        e UP/DOWN source 1
                                  
  char fonc[]="peri_pmo__\0";
  char* pipm=(char*)periIntPulseMode;
  uint8_t val=(*(pipm+sw*2)>>((num-1)*2)&0x03) | ((*(pipm+sw*2+1)>>((num-1)+3))&0x04);
  fonc[LENNOM-2]=(char)(0x40 | sw<<4 | num);
  //Serial.print(fonc);Serial.print(" *pipm=");Serial.print(*pipm,HEX);Serial.print(" *(pipm+1)=");Serial.print(*(pipm+1),HEX);Serial.print(" val=");Serial.print(val);
  numTableHtml(cli,'b',(void*)&val,fonc,1,0);
}

void subMPc(EthernetClient* cli,uint8_t sw,uint8_t num)          // n° checkbox
{

  char* pipm=(char*)periIntPulseMode;
  char fonc[]="peri_pmo__\0";
  uint8_t val=(*(pipm+sw*2+1)>>((num-1))&0x01);
  fonc[LENNOM-1]=(char)(0x40 | sw<<4 | num);
  
  checkboxTableHtml(cli,&val,fonc,-1,0);
}

void subModePulse(EthernetClient* cli,uint8_t numsw)   // modes pulses
{

  subMPc(cli,numsw,5);  // checkbox 5 enable général
  subMPc(cli,numsw,4);  // checkbox 4 cde serveur
  subMPn(cli,numsw,1);  // num de souce pour ON
  subMPc(cli,numsw,1);  // checkbox UP/DOWN pour source ON
  subMPn(cli,numsw,2);  // num de source pour OFF
  subMPc(cli,numsw,2);  // checkbox UP/DOWN pour source OFF
  subMPn(cli,numsw,3);  // num de source pour strobe
  subMPc(cli,numsw,3);  // checkbox UP/DOWN pour source strobe
}

void intModeTableHtml(EthernetClient* cli,byte* valeur,int nbli,int nbtypes)   // pulses, modes pulses, modes switchs
{
  char nac[]="ADOI";  // nom du type d'acttion (activ/désactiv/ON/OFF)
  
  char pfonc[]="peri_pon__\0";
  char qfonc[]="peri_pof__\0";
  char nfonc[]="peri_imn__\0";
  char cfonc[]="peri_imc__\0";
   
    for(int i=0;i<nbli;i++){                                           // i n° de switch
      nfonc[LENNOM-2]=(char)(i+48);
      cfonc[LENNOM-2]=(char)(i+48);
      pfonc[LENNOM-2]=(char)(i+48);
      qfonc[LENNOM-2]=(char)(i+48);
      pfonc[LENNOM-1]=(char)(64);
      qfonc[LENNOM-1]=(char)(64);
   
      cli->print("<td><font size=\"2\">");
      numTableHtml(cli,'l',(periIntPulseOn+i),pfonc,8,0);              // affichage-saisie pulse   
      subModePulse(cli,i);
      cli->print("<br>(");cli->print(*(periIntPulseCurrOn+i));cli->println(")<br>");
      numTableHtml(cli,'l',(periIntPulseOff+i),qfonc,8,0);             // affichage-saisie pulse       
 
      cli->print("<br>(");cli->print(*(periIntPulseCurrOff+i));cli->print(")");
      cli->print("</font></td>");
      
      cli->print("<td>");                                               // colonne des types d'action  
      for(int k=0;k<nbtypes;k++){                                       // k n° de type d'action (act/des/ON/OFF) (1 ligne par type)
        nfonc[LENNOM-1]=(char)(k*16+64);
        uint8_t val=(*(valeur+i*MAXTAC+k)>>6);                          // 4 bytes par sw ; 2 bits gauche n° détecteur + 3*2 bits enable - H/L
        //Serial.print(" *num* ");Serial.print(nfonc);Serial.print(" ");Serial.println(val,HEX);
        cli->print(nac[k]);
        numTableHtml(cli,'b',(uint32_t*)&val,nfonc,1,0);                // affichage-saisie n°détec
        
        for(int j=5;j>=0;j--){                                           // 3*2 checkbox enable+on/off pour les 3 sources d'un des 4 types de déclenchement du switch 
          cfonc[LENNOM-1]=(char)(k*16+j+64);                            // codage nom de fonction aaaaaaasb ; s n° de switch (0-3) ; b=01tt0bbb tt type ; bbb n° bit (0x40+0x30+0x07)
          uint8_t valb=((*(valeur+i*MAXTAC+k)>>j) & 0x01);                        
          //Serial.print("     *chk* ");Serial.print(cfonc);Serial.print(" ");Serial.println(valb,HEX);
          checkboxTableHtml(cli,&valb,cfonc,-1,0);
        }
        if(k<nbtypes-1){cli->print("<br>");}                             // fin ligne typez
      }
      cli->print("<br></td>");                                           // fin colonne types
    }
}

void periTableHtml(EthernetClient* cli)
{
  int i,j;
    
Serial.print("début péritable ; remote_IP ");serialPrintIp(remote_IP_cur);Serial.println();

  htmlIntro(NOMSERV,cli);
  char bufdate[15];alphaNow(bufdate);
  byte msb=0,lsb=0;                        // pour temp DS3231
  readDS3231temp(&msb,&lsb);

        cli->println("<body>");cli->print("<form method=\"POST \">");

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
          lnkTableHtml(cli,"test2sw___","testsw");

              numTableHtml(cli,'i',(uint32_t*)&perrefr,"per_refr__",4,0);cli->print("<input type=\"submit\" value=\"ok\">    ");//cli->println("    ");
              lnkTableHtml(cli,"dump_sd___","dump SDcard");

              cli->print("(");long sdsiz=fhisto.size();cli->print(sdsiz);cli->println(") ");
              numTableHtml(cli,'i',(uint32_t*)&sdpos,"sd_pos____",9,0);cli->print("<input type=\"submit\" value=\"ok\">");cli->println("<br>");

          cli->println("<table>");
              cli->println("<tr>");
                cli->println(" ON=VRAI=1=HAUT=CLOSE=GPIO2=ROUGE");
                cli->println("<th></th><th><br>nom_periph</th><th><br>TH</th><th><br>  V </th><th><br>per</th><th><br>pth</th><th>nb<br>sd</th><th><br>pg</th><th>nb<br>sw</th><th><br>_O_I___</th><th>nb<br>dt</th><th></th><th>mac_addr<br>ip_addr</th><th>version<br>last out<br>last in</th><th></th><th>timer ON <br>timer OFF</th><th>num det _server timer<br>__en/HL  _en/HL en/HL </th>");
              cli->println("</tr>");

              for(i=1;i<=NBPERIF;i++){
                // !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!! pericur doit étre le premier de la liste !!!!!!!!!!!!!!!!!!!!!!!!!
                // car l'enregistrement de la table va étre rechargé avant mise à jour
                periInit();periCur=i;periLoad(i);if(*periIntNb>4){*periIntNb=4;periSave(i);}

/*                Serial.print(i);Serial.print(" ");Serial.print(periNamer);Serial.print(" ");Serial.print(*periPerRefr);Serial.print(" ");
                Serial.print(*periPitch);Serial.print(" ");Serial.print(*periLastVal);Serial.print(" ");Serial.print(*periLastDate);Serial.print(" ");
                Serial.print(*periIntNb);Serial.print(" ");Serial.print(*periDetNb);
                Serial.print(" ");serialPrintIp(periIpAddr);Serial.print(" ");serialPrintMac(periMacr);
*/
                cli->println("<tr>");
                  cli->println("<form method=\"POST\">");
                      numTableHtml(cli,'i',&periCur,"peri_cur__",2,1);
                      cli->print("<td><input type=\"text\" name=\"peri_nom__\" value=\"");
                         cli->print(periNamer);cli->print("\" size=\"12\" maxlength=\"");cli->print(PERINAMLEN-1);cli->print("\" ></td>");
                      cli->print("<td>");cli->print(*periLastVal);cli->print("</td>");
                      cli->print("<td>");cli->print(*periAlim);cli->println("</td>");
                      numTableHtml(cli,'l',(uint32_t*)periPerRefr,"peri_refr_",5,1);
                      numTableHtml(cli,'f',periPitch,"peri_pitch",5,1);
                      numTableHtml(cli,'b',periSondeNb,"peri_sonde",2,1);
                      checkboxTableHtml(cli,(uint8_t*)periProg,"peri_prog_",-1,1);
                      numTableHtml(cli,'b',periIntNb,"peri_intnb",1,1);
                      cli->println("<td>");
                      xradioTableHtml(cli,*periIntVal,"peri_intv\0",2,*periIntNb,3);
                      numTableHtml(cli,'b',periDetNb,"peri_detnb",1,1);
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
                      intModeTableHtml(cli,periIntMode,*periIntNb,4);

                  cli->print("</form>");
                cli->println("</tr>");
              }
          cli->println("</table>");
        cli->println("</form></body></html>");
Serial.println("fin péritable");
}




