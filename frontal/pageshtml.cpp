#include <Arduino.h>
#include <SD.h>
#include "const.h"
#include <Wire.h>
#include "utilether.h"
#include "utilhtml.h"
#include "shconst.h"
#include "shutil.h"
#include "periph.h"
#include "pageshtml.h"

extern char*     nomserver;
extern byte*     mac;              // adresse server
extern byte*     localIp;
extern uint16_t* portserver;
extern char*     userpass;         // mot de passe browser
extern char*     modpass;          // mot de passe modif
extern char*     peripass;         // mot de passe périphériques

extern char*     chexa;
extern byte      maskbit[];

extern int       periCur;          // Numéro du périphérique courant

extern byte*     periMacr;                     // ptr ds buffer : mac address 
extern char*     periNamer;                    // ptr ds buffer : description périphérique
extern float*    periLastVal;                  // ptr ds buffer : dernière valeur de température  
extern float*    periThmin;                    // ptr ds buffer : alarme mini th
extern float*    periThmax;                    // ptr ds buffer : alarme maxi th
extern float*    periThOffset;                 // ptr ds buffer : offset correctif sur mesure température
extern char*     periLastDateIn;               // ptr ds buffer : date/heure de dernière réception
extern char*     periLastDateOut;              // ptr ds buffer : date/heure de dernier envoi  
extern char*     periVers;                     // ptr ds buffer : version logiciel du périphérique
extern char*     periModel;                    // ptr ds buffer : model du périphérique

extern byte      periMacBuf[6]; 

extern uint16_t  perrefr;
extern File      fhisto;           // fichier histo sd card
extern long      sdpos;
extern char      strSD[RECCHAR];


extern char*     ssid;
extern char*     passssid;
extern char*     usrnames;
extern char*     usrpass;
extern long*     usrtime;
extern char*     thermonames;
extern int16_t*  thermoperis;
extern uint16_t* toPassword;

extern int       usernum;

extern byte*     periSwVal;                    // ptr ds buffer peri : état/cde des inter 

File fimg;     // fichier image

extern struct SwRemote remoteT[MAXREMLI];
extern struct Remote remoteN[NBREMOTE];

extern struct Timers timersN[NBTIMERS];

extern char*     periNamer;                    // ptr ds buffer : description périphérique

extern int       fdatasave;

extern byte      memDetServ;  // image mémoire NBDSRV détecteurs (8)

int htmlImg(EthernetClient* cli,char* fimgname)    // suffisant pour commande péripheriques
{
        Serial.print(fimgname);
        File fimg; // = SD.open(fimgname,FILE_READ);
        if(sdOpen(FILE_READ,&fimg,fimgname)==SDKO){return SDKO;}
        else {
  
          cli->println("HTTP/1.1 200 OK");
          cli->println("CONTENT-Type: image/jpg");
          //cli->println("<link rel="icon" type="image/png" href="favicon.png\" sizes=\"64x64\">");
          cli->println();

          long fimgSiz=fimg.size();
          byte c;
          Serial.print(" size=");Serial.print(fimgSiz);
          while (fimgSiz>0){c=fimg.read();cli->write(&c,1);fimgSiz--;}
          fimg.close();
          delay(1);   
        }
        Serial.println(" terminé");
        cli->stop();
        return SDOK;
}

void htmlFavicon(EthernetClient* cli)
{
  htmlImg(cli,"sweeth.png");
}

void dumpsd(EthernetClient* cli)
{ 
  htmlIntro(nomserver,cli);

  cli->print("<body>");cli->print(VERSION);cli->println("<br>");
  boutRetour(cli,"retour",0,1);
  if(dumpsd0(cli)!=SDOK){cli->println("SDKO");}
  boutRetour(cli,"retour",0,1);
  
  cli->println("</body></html>");
}

int dumpsd0(EthernetClient* cli)                 // liste le fichier de la carte sd
{
  char inch=0;
  
  if(sdOpen(FILE_READ,&fhisto,"fdhisto.txt")==SDKO){return SDKO;}
  
  long sdsiz=fhisto.size();
  long pos=fhisto.position();
  fhisto.seek(sdpos);
  
  Serial.print("histoSD ");Serial.print(sdpos);Serial.print("/");Serial.print(sdsiz);

  cli->print("histoSD ");cli->print(sdpos);cli->print("/");cli->print(sdsiz);cli->println("<br>");

  long ptr=sdpos;
  
  while(ptr<sdsiz){
    inch=fhisto.read();ptr++;
    cli->print(inch);
  }

  return sdOpen(FILE_WRITE,&fhisto,"fdhisto.txt");
}

void accueilHtml(EthernetClient* cli)
{
            Serial.println(" saisie pwd");
            htmlIntro(nomserver,cli);

            cli->println("<body><form method=\"get\" >");
            cli->println(VERSION);cli->println("<br>");

            cli->println("<p>user <input type=\"username\" placeholder=\"Username\" name=\"username__\" value=\"\" size=\"6\" maxlength=\"8\" ></p>");            
            cli->println("<p>pass <input type=\"password\" placeholder=\"Password\" name=\"password__\" value=\"\" size=\"6\" maxlength=\"8\" ></p>");
                        cli->println(" <input type=\"submit\" value=\"login\"><br>");
            cli->println("</form></body></html>");
}          

void sscb(EthernetClient* cli,bool val,char* nomfonct,int nuf,int etat,uint8_t td,uint8_t nb)
{                                                                               // saisie checkbox ; 
                                                                                // le nom de fonction reçoit 2 caractères
  char nf[LENNOM+1];
  memcpy(nf,nomfonct,LENNOM);
  nf[LENNOM]='\0';
  nf[LENNOM-1]=(char)(nb+PMFNCHAR);
  nf[LENNOM-2]=(char)(nuf+PMFNCHAR);
  checkboxTableHtml(cli,(uint8_t*)&val,nf,etat,td);
}

void sscfgt(EthernetClient* cli,char* nom,uint8_t nb,void* value,int len,uint8_t type)  // type=0 value ok ; type =1 (char)value modulo len*nt ; type =2 (uint16_t)value modulo nb
{
  cli->print("<td><input type=\"text\" name=\"");cli->print(nom);cli->print((char)(nb+PMFNCHAR));cli->print("\" value=\"");
  if(type==0){cli->print((char*)value);cli->print("\" size=\"");cli->print(len);cli->print("\" maxlength=\"");cli->print(len);cli->println("\" ></td>");}
  if(type==2){cli->print(*((int16_t*)value+nb));cli->println("\" size=\"1\" maxlength=\"2\" ></td>");}
  if(type==1){cli->print((char*)(((char*)value+(nb*(len+1)))));cli->print("\" size=\"");cli->print(len);cli->print("\" maxlength=\"");cli->print(len);cli->println("\" ></td>");}
  if(type==3){cli->print(*((int8_t*)value));cli->println("\" size=\"1\" maxlength=\"2\" ></td>");}
}

void subcfgtable(EthernetClient* cli,char* titre,int nbl,char* nom1,char* value1,int len1,uint8_t type1,char* nom2,void* value2,int len2,char* titre2,uint8_t type2)
{
    cli->println("<table><col width=\"22\">");
    cli->println("<tr>");
    cli->print("<th></th><th>");cli->print(titre);cli->print("</th><th>");cli->print(titre2);cli->println("</th>");
    cli->println("</tr>");

    for(int nb=0;nb<nbl;nb++){
      cli->println("<tr>");
      cli->print("<td>");cli->print(nb);cli->print("</td>");

      sscfgt(cli,nom1,nb,value1,len1,type1);                      
      sscfgt(cli,nom2,nb,value2,len2,type2);

      if(len2==-1){
        int16_t peri=*((int16_t*)value2+nb);
        if(peri>0){Serial.print(peri);periLoad(peri);cli->println("<td>");cli->println(periNamer);cli->println("</td>");}
        if(nb==nbl-1){Serial.println();}
      }
      cli->println("</tr>");
    }
    cli->println("</table>");          
}

void cfgServerHtml(EthernetClient* cli)
{
            Serial.println(" config serveur");
            htmlIntro(nomserver,cli);
            
            cli->println("<body><form method=\"get\" >");
            cli->println(VERSION);cli->println("<br>");

            usrFormHtml(cli,1);
            
            boutRetour(cli,"retour",0,0);
            
            cli->println(" <input type=\"submit\" value=\"MàJ\"><br>");
            
            cli->print(" password <input type=\"text\" name=\"pwdcfg____\" value=\"");cli->print(userpass);cli->print("\" size=\"5\" maxlength=\"");cli->print(LPWD);cli->println("\" >");
            cli->print("  modpass <input type=\"text\" name=\"modpcfg___\" value=\"");cli->print(modpass);cli->print("\" size=\"5\" maxlength=\"");cli->print(LPWD);cli->println("\" >");            
            cli->print(" peripass <input type=\"text\" name=\"peripcfg__\" value=\"");cli->print(peripass);cli->print("\" size=\"5\" maxlength=\"");cli->print(LPWD);cli->println("\" >");            
            cli->print(" to password");numTableHtml(cli,'d',toPassword,"to_passwd_",6,0,0);cli->println("<br>");
            cli->print(" serverMac <input type=\"text\" name=\"ethcfg___m\" value=\"");for(int k=0;k<6;k++){cli->print(chexa[mac[k]/16]);cli->print(chexa[mac[k]%16]);}cli->print("\" size=\"11\" maxlength=\"");cli->print(12);cli->println("\" >");                        
            cli->print(" localIp <input type=\"text\" name=\"ethcfg___i\" value=\"");for(int k=0;k<4;k++){cli->print(localIp[k]);if(k!=3){cli->print(".");}}cli->print("\" size=\"11\" maxlength=\"");cli->print(15);cli->println("\" >");                        
            cli->print(" portserver ");numTableHtml(cli,'d',portserver,"ethcfg___p",4,0,0);cli->println("<br>");

            subcfgtable(cli,"SSID",MAXSSID,"ssid_____",ssid,LENSSID,1,"passssid_",passssid,LPWSSID,"password",1);
            subcfgtable(cli,"USERNAME",NBUSR,"usrname__",usrnames,LENUSRNAME,1,"usrpass__",usrpass,LENUSRPASS,"password",1);
            subcfgtable(cli,"THERMO",NBTHERMO,"thername_",thermonames,LENTHNAME,1,"therperi_",thermoperis,-1,"peri",2);
            
            cli->println("</form></body></html>");
}

void cfgRemoteHtml(EthernetClient* cli)
{
  char nf[LENNOM+1];nf[LENNOM]='\0';
  uint8_t val;
  
            Serial.println(" config remote");
            htmlIntro(nomserver,cli);
            
            cli->println("<body><form method=\"get\" >");
            cli->println(VERSION);cli->println("<br>");
            
            usrFormHtml(cli,1);
            boutRetour(cli,"retour",0,0);

            cli->println(" <input type=\"submit\" value=\"MàJ\"><br>");

/* table remotes */

              cli->println("<table>");
              cli->println("<tr>");
              cli->println("<th>   </th><th>      Nom      </th><th> on/off </th><th> en </th>");
              cli->println("</tr>");

              for(int nb=0;nb<NBREMOTE;nb++){
                cli->println("<tr>");
                
                cli->print("<td>");cli->print(nb+1);cli->print("</td>");
                cli->print("<td><input type=\"text\" name=\"remotecfn");cli->print((char)(nb+PMFNCHAR));cli->print("\" value=\"");
                        cli->print(remoteN[nb].nam);cli->print("\" size=\"12\" maxlength=\"");cli->print(LENREMNAM-1);cli->println("\" ></td>");

                //sliderHtml(cli,(uint8_t*)(&remoteN[nb].onoff),"remotecfo_",nb,0,1);

                memcpy(nf,"remotecfo_",LENNOM);nf[LENNOM-1]=(char)(nb+PMFNCHAR);
                val=(uint8_t)remoteN[nb].onoff;
                checkboxTableHtml(cli,&val,nf,-1,1);         
                
                memcpy(nf,"remotecfe_",LENNOM);nf[LENNOM-1]=(char)(nb+PMFNCHAR);
                val=(uint8_t)remoteN[nb].enable;
                checkboxTableHtml(cli,&val,nf,-1,1);         
                   
                cli->println("</tr>");
              }
            cli->println("</table><br>");

/* table switchs */

            cli->println("<table>");
              cli->println("<tr>");
              cli->println("<th>  </th><th> rem </th><th>  </th><th> per </th><th>  </th><th>I/O</th><th> sw </th><th>   </th>");
              cli->println("</tr>");
              
              for(int nb=0;nb<MAXREMLI;nb++){
                cli->println("<tr>");
                cli->print("<td>");cli->print(nb+1);cli->print("</td>");
                
                memcpy(nf,"remotecfu_",LENNOM);nf[LENNOM-1]=(char)(nb+PMFNCHAR);
                numTableHtml(cli,'b',&remoteT[nb].num,nf,1,1,0);
                
                cli->print("<td>");if(remoteT[nb].num!=0){cli->print(remoteN[remoteT[nb].num-1].nam);}else {cli->print(" ");}cli->print("</td>");
                memcpy(nf,"remotecfp_",LENNOM);nf[LENNOM-1]=(char)(nb+PMFNCHAR);
                numTableHtml(cli,'b',&remoteT[nb].pernum,nf,2,1,0);
                
                cli->print("<td>");
                if(remoteT[nb].pernum!=0){periLoad(remoteT[nb].pernum);cli->print(periNamer);cli->print("</td><td>");if((*periSwVal>>(2*remoteT[nb].persw)+1)&0x01!=0){cli->print("I");}else{cli->print("O");}}
                else {cli->print(" </td><td>");}
                cli->print("</td>");

                memcpy(nf,"remotecfs_",LENNOM);nf[LENNOM-1]=(char)(nb+PMFNCHAR);
                numTableHtml(cli,'b',&remoteT[nb].persw,nf,1,1,0);
                
                memcpy(nf,"remotecfx_",LENNOM);nf[LENNOM-1]=(char)(nb+PMFNCHAR);
                uint8_t ren=(uint8_t)remoteT[nb].enable;
                checkboxTableHtml(cli,&ren,nf,-1,1);         
                
                cli->println("</tr>");
              }
              
            cli->println("</table>");            
            cli->println("</form></body></html>");
}


void remoteHtml(EthernetClient* cli)
{  
            Serial.println("remote control");
            htmlIntro(nomserver,cli);
            
            cli->println("<body><form method=\"get\" >");
            cli->println(VERSION);cli->println(" ");

            usrFormHtml(cli,1);
            
            boutRetour(cli,"retour",0,0);cli->print(" ");          
            cli->println("<br>");

/* table remotes */

            cli->println("<table>");

            for(int nb=0;nb<NBREMOTE;nb++){
              if(remoteN[nb].nam[0]!='\0'){
                cli->println("<tr>");
                
                cli->print("<td>");cli->print(nb+1);cli->println("</td>");
                cli->print("<td>");
                // l'input hidden assure que toutes les lignes génèrent une fonction dans 'GET /' pour assurer l'effacement des cb
                cli->print("<input type=\"hidden\" name=\"remote_cn");cli->print((char)(nb+PMFNCHAR));cli->println("\">");
                cli->print(" <font size=\"7\">");cli->print(remoteN[nb].nam);cli->println("</font></td>");

                sliderHtml(cli,(uint8_t*)(&remoteN[nb].onoff),"remote_ctl",nb,0,1);
                
                uint8_t ren=(uint8_t)remoteN[nb].enable;
                char nf[LENNOM+1]="remote_xe_";nf[LENNOM-1]=(char)(nb+PMFNCHAR);
                checkboxTableHtml(cli,&ren,nf,-1,1);
                            
                cli->println("</tr>");
              }
            }
            cli->println("</table>");
            
            cli->println("<p align=\"center\" ><input type=\"submit\" value=\"MàJ\" style=\"height:120px;width:400px;background-color:LightYellow;font-size:40px;font-family:Courier,sans-serif;\"><br>");
            boutFonction(cli,"thermohtml","","températures",0,0,7,0);cli->print(" ");
            boutFonction(cli,"remotehtml","","refresh",0,0,7,0);
            cli->print("</p>");
            
            cli->println("</form></body></html>");
}

int scalcTh(int bd)           // maj temp min/max des périphériques sur les bd derniers jours
{
/* --- calcul date début --- */
  
  int   ldate=15;

  int   yy,mm,dd,js,hh,mi,ss;
  byte  yb,mb,db,dsb,hb,ib,sb;
  readDS3231time(&sb,&ib,&hb,&dsb,&db,&mb,&yb);           // get date(now)
  yy=yb+2000;mm=mb;dd=db;hh=hb;mi=ib;ss=sb;
  calcDate(bd,&yy,&mm,&dd,&js,&hh,&mi,&ss);               // get new date
  uint32_t amj=yy*10000L+mm*100+dd;
  uint32_t hms=hh*10000L+mi*100+ss;
  char     dhasc[ldate+1];
  sprintf(dhasc,"%.8lu",amj);strcat(dhasc," ");
  sprintf(dhasc+9,"%.6lu",hms);dhasc[15]='\0';            // dhasc date/heure recherchée
  Serial.print("dhasc=");Serial.println(dhasc);
  
  if(sdOpen(FILE_READ,&fhisto,"fdhisto.txt")==SDKO){return SDKO;}
  
  long sdsiz=fhisto.size();
  long searchStep=100000;
  long ptr,curpos=sdsiz;
  fhisto.seek(curpos);
  long pos=fhisto.position();  
  
  Serial.print("--- start search date at ");Serial.print(curpos-searchStep);Serial.print(" sdsiz=");Serial.print(sdsiz);Serial.print(" pos=");Serial.print(pos);Serial.print(" (millis=");Serial.print(millis());Serial.println(")");

/* --- recherche 1ère ligne --- */

  char inch1=0,inch2=0;
  char buf[RECCHAR];
  bool fini=FAUX;
  int pt;
    
  while(curpos>0 && !fini){
    curpos-=searchStep;if(curpos<0){curpos=0;}ptr=curpos;
    fhisto.seek(curpos);
    while(ptr<curpos+searchStep && inch1!='\n'){inch1=fhisto.read();ptr++;}
    for(pt=0;pt<ldate;pt++){buf[pt]=fhisto.read();ptr++;}                    // '\n' trouvé : get date
    if(memcmp(buf,dhasc,ldate)>0){ptr=curpos+searchStep;}                    // si la date trouvée est > reculer
    else {                                                                   // sinon chercher >
      
      while(!fini){
        while(ptr<pos && inch1!='\n'){inch1=fhisto.read();ptr++;}
        for(pt=0;pt<ldate;pt++){buf[pt]=fhisto.read();ptr++;}                // '\n' trouvé : get date
        if(memcmp(buf,dhasc,ldate)>0){                                       // si la date trouvé est > ok sinon continuer
          fini=VRAI;                                                         // ptr ok ; pt ok commencer l'acquisition
        }
      }
    }
  }
  long t0=millis();
  Serial.print("--- fin recherche ptr=");Serial.print(ptr);Serial.print(" millis=");Serial.print(millis());Serial.println("");

/* --- balayage et màj --- */
  
  char strfds[3];memset(strfds,0x00,3);
  if(convIntToString(strfds,fdatasave)>2){
    Serial.print("fdatasave>99!! ");Serial.print("fdatasave=");Serial.print(fdatasave);Serial.print(" strfds=");Serial.println(strfds);ledblink(BCODESYSERR);
  }
  char* pc;
  float th,np;
  int lnp=0,nbli=0,nbth=0;

  for(int pp=1;pp<=NBPERIF;pp++){periLoad(pp);if(periMacr[0]!=0x00){*periThmin=99;*periThmax=-99;periSave(pp,PERISAVELOCAL);}}
                                                                             
                                                                         // acquisition
  fhisto.seek(ptr-ldate);                                                // sur début enregistrement
  fini=FAUX;
  while(ptr<pos){
    pt=0;
    inch1='\0';      
    while(ptr<pos && inch1!='\n'){inch1=fhisto.read();buf[pt]=inch1;pt++;ptr++;}   // get record
    buf[pt]='\0';
    nbli++;
    pc=strchr(buf,';');
    if(memcmp(buf+ldate+1,"ip",2)==0 && memcmp(pc+1,strfds,2)==0){       // datasave (après ';' soit '\n' soit'<' soit num fonction)
      np=convStrToNum(pc+SDPOSNUMPER,&lnp);                              // num périphérique
      th=convStrToNum(pc+SDPOSTEMP,&lnp);                                // temp périphérique
      periLoad((int)np);
//delay(20);Serial.print(buf);Serial.print(" per=");Serial.print(np);Serial.print(" th=");Serial.print(th);Serial.print(" - ");periPrint(np);
      packMac(periMacBuf,pc+SDPOSMAC);                       
      if(compMac(periMacBuf,periMacr)){                                  // contrôle mac
        if(*periThmin>th){*periThmin=th;periSave((int)np,PERISAVELOCAL);nbth++;}
        if(*periThmax<th){*periThmax=th;periSave((int)np,PERISAVELOCAL);nbth++;} 
      }      
    }
  }
  for(uint16_t pp=1;pp<=NBPERIF;pp++){periLoad(pp);if(periMacr[0]!=0x00){periSave(pp,PERISAVESD);}}   // écriture SD
  
  Serial.print("--- fin balayage ");Serial.print(nbli);Serial.print(" lignes ; ");Serial.print(nbth);Serial.print(" màj ; millis=");Serial.print(millis()-t0);Serial.println("");
 
  fhisto.seek(pos);
  return sdOpen(FILE_WRITE,&fhisto,"fdhisto.txt");
}

void intro(EthernetClient* cli)
{
  htmlIntro0(cli);
  cli->println("<head><title>sweet hdev</title>");
  
  cli->println("<style>");
  cli->println("table {");
  cli->println("font-family: Courier, sans-serif;");
  cli->println("border-collapse: collapse;");
  cli->println("overflow: auto;");
  cli->println("white-space:nowrap;");
  cli->println("}");
  cli->println("td, th {");
  cli->println("border: 1px solid #dddddd;");
  cli->println("text-align: left;");
  cli->println("}");
  cli->println("</style>");
  
  cli->println("</head>");
}

void thermoHtml(EthernetClient* cli)
{  
            scalcTh(1);          // update periphériques
            
            Serial.println("thermo show");
            //htmlIntro(nomserver,cli);
            intro(cli);
            
            cli->print("<body>");cli->print(VERSION);cli->print(" ");
            boutRetour(cli,"retour",0,0);cli->print(" ");


/* peritable températures */

         cli->println("<table>");
              cli->println("<tr>");
                cli->println("<th>peri</th><th></th><th>TH</th><th>min</th><th>max</th><th>last in</th>");
              cli->println("</tr>");

              for(int nuth=0;nuth<NBTHERMO;nuth++){
                int16_t nuper=*(thermoperis+nuth);
                if(nuper!=0){
                  periInitVar();periCur=nuper;periLoad(periCur);
                  if(periMacr[0]!=0x00){
                    cli->println("<tr>");
                      //cli->print("<td>");cli->print(nuth+1);cli->print("</td>");
                      cli->print("<td>");cli->print(periCur);cli->println("</td>");
                      cli->print("<td> <font size=\"7\">");cli->print(thermonames+nuth*(LENTHNAME+1));cli->println("</font></td>");
                      cli->print("<td> <font size=\"7\">");cli->print(*periLastVal+*periThOffset);cli->println("</font></td>");
                      cli->print("<td> <div style='text-align:right; font-size:30px;'>");cli->print(*periThmin);cli->println("</div></td>");
                      cli->print("<td> <div style='text-align:right; font-size:30px;'>");cli->print(*periThmax);cli->println("</div></td>");
                      cli->print("<td>");printPeriDate(cli,periLastDateIn);cli->println("</td>");                      
                    cli->println("</tr>");
                  }
                }
              }
          cli->println("</table>");

        cli->print("<p align=\"center\">");
        boutFonction(cli,"remotehtml","","remote",0,0,7,0);cli->print(" ");                        
        boutFonction(cli,"thermohtml","","refresh",0,0,7,0);
        cli->print("</p>");
        cli->print("<br><br><br>");for(int d=0;d<NBDSRV;d++){cli->print((char)(((memDetServ>>d)&0x01)+48));cli->print(" ");}
        cli->println("/n</body></html>");
}

void timersHtml(EthernetClient* cli)
{
  int nucb;           
  char a;
  char bufdate[LNOW];alphaNow(bufdate);
  
            Serial.println("saisie timers");
            htmlIntro(nomserver,cli);
            
            cli->print("<body>");cli->print(VERSION);cli->print(" ");
            
            boutRetour(cli,"retour",0,0);cli->print(" ");
            boutFonction(cli,"timershtml","","refresh",0,0,1,0);cli->print(" ");

            for(int zz=0;zz<14;zz++){cli->print(bufdate[zz]);if(zz==7){cli->print("-");}}cli->print("-");cli->print((char)(bufdate[14]+48));
            cli->println(" GMT");

         cli->println("<table>");
              cli->println("<tr>");
                cli->println("<th></th><th>nom</th><th>det</th><th>h_beg</th><th>h_end</th><th></th><th>e_p_c_f_</th><th>7_d_l_m_m_j_v_s</th><th>dh_beg_cycle</th><th>dh_end_cycle</th>");
              cli->println("</tr>");

              for(int nt=0;nt<NBTIMERS;nt++){

                    cli->println("<tr>");
                    cli->println("<form method=\"GET \">");
                      usrFormHtml(cli,1);
                      cli->print("<td>");cli->print(nt+1);cli->println("</td>");
                      //Serial.print(nt);Serial.print(" ");Serial.print(timersN[nt].nom);Serial.print(" ");Serial.println(timersN[nt].detec);
                      
                      sscfgt(cli,"tim_name_",nt,timersN[nt].nom,LENTIMNAM,0);
                      sscfgt(cli,"tim_det__",nt,&timersN[nt].detec,1,3);                                            
                      sscfgt(cli,"tim_hdf_d",nt,timersN[nt].hdeb,6,0);                                            
                      sscfgt(cli,"tim_hdf_f",nt,timersN[nt].hfin,6,0);                   
                      
                      char oi[]="OI",md=memDetServ>>timersN[nt].detec;
                      cli->print("<td>");cli->print(oi[timersN[nt].curstate]);cli->print(md/16,HEX);cli->print(md%16,HEX);
                      cli->println("</td><td>");
                      
                      nucb=0;sscb(cli,timersN[nt].enable,"tim_chkb__",nucb,-1,0,nt);
                      nucb++;sscb(cli,timersN[nt].perm,"tim_chkb__",nucb,-1,0,nt);
                      nucb++;sscb(cli,timersN[nt].cyclic,"tim_chkb__",nucb,-1,0,nt);   
                      nucb++;sscb(cli,timersN[nt].forceonoff,"tim_chkb__",nucb,-1,0,nt);
                     
                      cli->println("</td><td>");
                      for(int nj=7;nj>=0;nj--){
                        bool vnj; 
                        vnj=(timersN[nt].dw>>nj)&0x01;
                        nucb++;sscb(cli,vnj,"tim_chkb__",nucb,-1,0,nt);
                      }
                      cli->println("</td>");
                      sscfgt(cli,"tim_hdf_b",nt,&timersN[nt].dhdebcycle,14,0);
                      sscfgt(cli,"tim_hdf_e",nt,&timersN[nt].dhfincycle,14,0); 
                      cli->print("<td>");cli->print(" <input type=\"submit\" value=\"MàJ\"><br>");cli->println("</td>");
                    cli->print("<td>");cli->print(timersN[nt].dw,HEX);cli->print(" ");cli->print(maskbit[1+bufdate[14]*2],HEX);cli->println("</td>");
                    cli->println("</form>");
                    cli->println("</tr>");
                  }

        cli->println("</table>");
        cli->println("</body></html>");
}


void testHtml(EthernetClient* cli)
{
            Serial.println(" page d'essais");
 htmlImg(cli,"sweeth.jpg");
            
            
/*            
            
            htmlIntro(nomserver,cli);

            char pwd[32]="password__=\0";strcat(pwd,userpass);
            
            cli->println("<body><form method=\"get\" action=\"page.html\" >");

            cli->println("<a href=page.html?");cli->print(pwd);cli->print(":>retourner</a><br>");
            cli->println("<a href=page.html?password__=17515A:>retourner</a><br>");

            //cli->print(" <p hidden> ce que je ne veux plus voir <input type=\"text\" name=\"ma_fonct_1\" value=\"\"><br><p>");
            //cli->println(" ce que je veux toujour voir <input type=\"text\" name=\"ma_fonct_2\" value=\"\"><br>");

            //cli->print("<img id=\"img1\" alt=\"BOUTON\" fp-style=\"fp-btn: Border Left 1\" fp-title=\"BOUTON\" height=\"20\"  style=\"border: 0\" width=\"100\"> <br>");

            //cli->println(" <input type=\"submit\" value=\"MàJ\"><br>");

            cli->println("<a href=\"page.html?password__=17515A:\"><input type=\"button\" value=\"href+input button\"></a><br>"); 
            
            cli->println("<a href=page.html?");cli->print(pwd);cli->print(":><input type=\"submit\" value=\"retour\"></a><br>");
            cli->println("<a><input type=\"submit\" formaction=\"page.html?password__=17515A:\" value=\"formaction\"></a><br>");

            bouTableHtml(cli,"password__",userpass,"boutable",2,1);
            
            cli->print("<form><p hidden><input type=\"text\" name=\"password__\" value=\"");
            cli->print(userpass);
            cli->print("\" ><p><input type=\"submit\" value=\"submit\"><br></form>");
            
            
            cli->print("<a href=page.html?password__=17515A:><input type=\"submit\" value=\"retour\"></a><br>");

            cli->println("</form></body></html>");            
*/
}           
