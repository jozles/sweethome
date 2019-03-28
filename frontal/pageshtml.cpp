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

extern char* nomserver;
extern byte* mac;              // adresse server
extern char* userpass;         // mot de passe browser
extern char* modpass;          // mot de passe modif
extern char* peripass;         // mot de passe périphériques

extern char* chexa;

extern int   periCur;          // Numéro du périphérique courant

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

extern uint16_t perrefr;
extern File     fhisto;           // fichier histo sd card
extern long     sdpos;
extern char     strSD[RECCHAR];


extern char*    ssid;
extern char*    passssid;
extern char*    usrnames;
extern char*    usrpass;
extern long*    usrtime;
extern char*    thermonames;
extern int16_t* thermoperis;

extern int      usernum;

extern byte*     periSwVal;                    // ptr ds buffer peri : état/cde des inter 

File fimg;     // fichier image

extern struct swRemote remoteT[MAXREMLI];
extern struct Remote remoteN[NBREMOTE];

extern char*     periNamer;                    // ptr ds buffer : description périphérique




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

void sscfgt(EthernetClient* cli,char* nom,uint8_t nb,void* value,int len)
{
  cli->print("<td><input type=\"text\" name=\"");cli->print(nom);cli->print((char)(nb+PMFNCHAR));cli->print("\" value=\"");
  if(len<0){cli->print(*((int16_t*)value+nb));cli->println("\" size=\"1\" maxlength=\"2\" ></td>");}
  else {cli->print((char*)(((char*)value+(nb*(len+1)))));cli->print("\" size=\"");cli->print(len);cli->print("\" maxlength=\"");cli->print(len);cli->println("\" ></td>");}
}

void subcfgtable(EthernetClient* cli,char* titre,int nbl,char* nom1,char* value1,int len1,char* nom2,void* value2,int len2,char* titre2)
{
    cli->println("<table><col width=\"22\">");
    cli->println("<tr>");
    cli->print("<th></th><th>");cli->print(titre);cli->print("</th><th>");cli->print(titre2);cli->println("</th>");
    cli->println("</tr>");

    for(int nb=0;nb<nbl;nb++){
      cli->println("<tr>");
      cli->print("<td>");cli->print(nb);cli->print("</td>");

      sscfgt(cli,nom1,nb,value1,len1);
//                  cli->print("<td><input type=\"text\" name=\"");cli->print(nom1);cli->print((char)(nb+PMFNCHAR));cli->print("\" value=\"");    
//                      cli->print(value1+(nb*(len1+1)));cli->print("\" size=\"");cli->print(len1/2);cli->print("\" maxlength=\"");cli->print(len1);cli->println("\" ></td>");
                      
      sscfgt(cli,nom2,nb,value2,len2);
//                  cli->print("<td><input type=\"text\" name=\"");cli->print(nom2);cli->print((char)(nb+PMFNCHAR));cli->print("\" value=\"");
//                      cli->print((char*)(value2+(nb*(len2+1))));cli->print("\" size=\"");cli->print(len2/2);cli->print("\" maxlength=\"");cli->print(len2);cli->println("\" ></td>");
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
            cli->print(" serverMac <input type=\"text\" name=\"maccfg____\" value=\"");for(int k=0;k<6;k++){cli->print(chexa[mac[k]/16]);cli->print(chexa[mac[k]%16]);}cli->print("\" size=\"11\" maxlength=\"");cli->print(12);cli->println("\" >");                        

            subcfgtable(cli,"SSID",MAXSSID,"ssid_____",ssid,LENSSID,"passssid_",passssid,LPWSSID,"password");
            subcfgtable(cli,"USERNAME",NBUSR,"usrname__",usrnames,LENUSRNAME,"usrpass__",usrpass,LENUSRPASS,"password");
            subcfgtable(cli,"THERMO",NBTHERMO,"thername_",thermonames,LENTHNAME,"therperi_",thermoperis,-1,"peri");
            
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
            
            boutRetour(cli,"retour",0,0);
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
            
            cli->println("<p align=\"center\" ><input type=\"submit\" value=\"MàJ\" style=\"height:120px;width:400px;background-color:LightYellow;\"></p><br>");
            boutFonction(cli,"thermohtml","","thermohtml",0,0,7,1);                        
            
            cli->println("</form></body></html>");
}


void thermoHtml(EthernetClient* cli)
{  
            Serial.println("thermo show");
            htmlIntro(nomserver,cli);
            
            cli->print("<body>");cli->print(VERSION);cli->println("<br>");
            boutRetour(cli,"retour",0,1);

/* peritable températures */

         cli->println("<table>");
              cli->println("<tr>");
                cli->println("<th>thermo</th><th>peri</th><th></th><th>TH</th><th>min</th><th>max</th><th>last in</th>");
              cli->println("</tr>");

              for(int nuth=0;nuth<NBTHERMO;nuth++){
                int16_t nuper=*(thermoperis+nuth);
                if(nuper!=0){
                  periInitVar();periCur=nuper;periLoad(periCur);
                  if(periMacr[0]!=0x00){
                    cli->println("<tr>");
                      cli->print("<td>");cli->print(nuth+1);cli->print("</td>");
                      cli->print("<td>");cli->print(periCur);cli->println("</td>");
                      cli->print("<td>");cli->print(" <font size=\"7\">");cli->print(thermonames+nuth*(LENTHNAME+1));cli->println("</font>");cli->println("</td>");
                      cli->print("<td>");cli->print(" <font size=\"7\">");cli->print(*periLastVal+*periThOffset);cli->println("</font>");cli->println("</td>");
                      cli->print("<td>");cli->print(*periThmin);cli->println("</td>");
                      cli->print("<td>");cli->print(*periThmax);cli->println("</td>");
                      cli->print("<td>");printPeriDate(cli,periLastDateIn);cli->println("</td>");                      
                    cli->println("</tr>");
                  }
                }
              }
          cli->println("</table>");

        boutRetour(cli,"retour",0,0);
        boutFonction(cli,"remotehtml","","remote",0,0,7,1);                        

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
