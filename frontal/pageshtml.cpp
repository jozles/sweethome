#include <Arduino.h>
#include <SD.h>
#include "const.h"
#include <Wire.h>
#include "utilether.h"
#include "shconst.h"
#include "shutil.h"
#include "periph.h"
#include "peritable.h"

extern char* nomserver;
extern byte* mac;              // adresse server
extern char* usrpass;          // mot de passe browser
extern char* modpass;          // mot de passe modif
extern char* peripass;         // mot de passe périphériques

extern char* chexa;

extern uint16_t perrefr;
extern File     fhisto;           // fichier histo sd card
extern long     sdpos;
extern char     strSD[RECCHAR];
extern char*    ssid;
extern char*    passssid;

File fimg;     // fichier image

extern struct swRemote remoteT[MAXREMLI];
extern struct Remote remoteN[NBREMOTE];


void htmlIntro0(EthernetClient* cli)    // suffisant pour commande péripheriques
{
  cli->println("HTTP/1.1 200 OK");
  //cli->println("Location: http://82.64.32.56:1789/");
  //cli->println("Cache-Control: private");
  cli->println("CONTENT-Type: text/html; charset=UTF-8");
  cli->println("Connection: close\n");
  cli->println("<!DOCTYPE HTML ><html>");
}


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
          /*
          if(fimgSiz>MAXIMGLEN){Serial.print(" trop grand");return SDKO;}
          else{
            long ptr=0;
            char clibuf[fimgSiz];

            while(fimg.available())
            {
                clibuf[ptr]=fimg.read();
                ptr++;
            }
            cli->write(clibuf,ptr);
          }
          */
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

            cli->print(".button {background-color: #195B6A; border: none; color: white; padding: 16px 40px;");
            cli->println("text-decoration: none; font-size: 30px; margin: 2px; cursor: pointer;}");
            cli->println(".button2 {background-color: #77878A;}");

          cli->println("</style>");
  
  cli->println("</head>");
}

int dumpsd(EthernetClient* cli)                 // liste le fichier de la carte sd
{
  char inch=0;
  
  if(sdOpen(FILE_READ,&fhisto,"fdhisto.txt")==SDKO){return SDKO;}
  
  long sdsiz=fhisto.size();
  long pos=fhisto.position();
  fhisto.seek(sdpos);
  
  Serial.print("histoSD ");Serial.print(sdpos);Serial.print("/");Serial.print(sdsiz);

  htmlIntro(nomserver,cli);cli->println("<body>");
  cli->print("histoSD ");cli->print(sdpos);cli->print("/");cli->print(sdsiz);cli->println("<br>");

  long ptr=sdpos;
  
  while(ptr<sdsiz){
    inch=fhisto.read();ptr++;
    cli->print(inch);
  }
  cli->println("</body>");
  return sdOpen(FILE_WRITE,&fhisto,"fdhisto.txt");
}
/* 
  int cnt=0,strSDpt=0;
  bool ignore=FAUX; // ignore until next line
  long ptr=sdpos;
  while (ptr<sdsiz || inch==-1)
    {
    inch=fhisto.read();ptr++;
    if(!ignore)
      {        
      strSD[strSDpt]=inch;strSDpt++;strSD[strSDpt]='\0';
      if((strSDpt>=(RECCHAR-1)) || (strSD[strSDpt-2]=='\r' && strSD[strSDpt-1]=='\n')){
        cli->println(strSD);                                                                                                                                                                                                        
        memset(strSD,'\0',RECCHAR);strSDpt=0;
        }
      }
      else if (inch==10){ignore=FAUX;}
    }
  cli->println(strSD);

  cli->println("</body>");

  return sdOpen(FILE_WRITE,&fhisto,"fdhisto.txt");
}
*/

void accueilHtml(EthernetClient* cli)
{
            Serial.println(" saisie pwd");
            htmlIntro(nomserver,cli);

            cli->println("<body><form method=\"get\" >");
            cli->println(VERSION);cli->println("<br>");
            cli->println("<p>pass <input type=\"password\" name=\"password__\" value=\"\" size=\"6\" maxlength=\"8\" ></p>");
            cli->println("</form></body></html>");
}          

void cfgServerHtml(EthernetClient* cli)
{
            Serial.println(" config serveur");
            htmlIntro(nomserver,cli);
            
            cli->println("<body><form method=\"get\" >");
            cli->println(VERSION);cli->println("<br>");
            //char pwd[32]="password__=\0";strcat(pwd,usrpass);lnkTableHtml(cli,pwd,"retour");
            bouTableHtml(cli,"password__",modpass,"retour",0,0);  // génère peritable
            cli->println(" <input type=\"submit\" value=\"MàJ\"><br>");
            
            cli->print(" password <input type=\"text\" name=\"pwdcfg____\" value=\"");cli->print(usrpass);cli->print("\" size=\"5\" maxlength=\"");cli->print(LPWD);cli->println("\" >");
            cli->print("  modpass <input type=\"text\" name=\"modpcfg___\" value=\"");cli->print(modpass);cli->print("\" size=\"5\" maxlength=\"");cli->print(LPWD);cli->println("\" >");            
            cli->print(" peripass <input type=\"text\" name=\"peripcfg__\" value=\"");cli->print(peripass);cli->print("\" size=\"5\" maxlength=\"");cli->print(LPWD);cli->println("\" >");            
            cli->print(" serverMac <input type=\"text\" name=\"maccfg____\" value=\"");for(int k=0;k<6;k++){cli->print(chexa[mac[k]/16]);cli->print(chexa[mac[k]%16]);}cli->print("\" size=\"11\" maxlength=\"");cli->print(12);cli->println("\" >");                        
            
            
            cli->println("<table>");
              cli->println("<tr>");
              cli->println("<th>   </th><th>      SSID      </th><th>  password  </th>");
              cli->println("</tr>");

              for(int nb=0;nb<MAXSSID;nb++){
                cli->println("<tr>");
                  cli->print("<td>");cli->print(nb);cli->print("</td>");

                  cli->print("<td><input type=\"text\" name=\"ssid_____");cli->print((char)(nb+PMFNCHAR));cli->print("\" value=\"");    
                      cli->print(ssid+(nb*(LENSSID+1)));cli->print("\" size=\"12\" maxlength=\"");cli->print(LENSSID);cli->println("\" ></td>");
                      
                  cli->print("<td><input type=\"text\" name=\"passssid_");cli->print((char)(nb+PMFNCHAR));cli->print("\" value=\"");
                      cli->print(passssid+(nb*(LPWSSID+1)));cli->print("\" size=\"38\" maxlength=\"");cli->print(LPWSSID);cli->println("\" ></td>");
                      
                cli->println("</tr>");
              }
            cli->println("</table>");            
            cli->println("</form></body></html>");
}

void cfgRemoteHtml(EthernetClient* cli)
{
  char nf[LENNOM+1];nf[LENNOM]='\0';
  
            Serial.println(" config remote");
            htmlIntro(nomserver,cli);
            
            cli->println("<body><form method=\"get\" >");
            cli->println(VERSION);cli->println("<br>");
            
            bouTableHtml(cli,"password__",modpass,"retour",0,0);  // génère peritable
            cli->println(" <input type=\"submit\" value=\"MàJ\"><br>");

/* table remotes */

              cli->println("<table>");
              cli->println("<tr>");
              cli->println("<th>   </th><th>      Nom      </th><th>   </th>");
              cli->println("</tr>");

              for(int nb=0;nb<NBREMOTE;nb++){
                cli->println("<tr>");
                cli->print("<td>");cli->print(nb+1);cli->print("</td>");
                cli->print("<td><input type=\"text\" name=\"remote_no");cli->print((char)(nb+PMFNCHAR));cli->print("\" value=\"");
                        cli->print(remoteN[nb].nam);cli->print("\" size=\"12\" maxlength=\"");cli->print(LENREMNAM-1);cli->println("\" ></td>");
                memcpy(nf,"remote_en_",LENNOM);nf[LENNOM-1]=(char)(nb+PMFNCHAR);
                uint8_t ren=(uint8_t)remoteN[nb].enable;
                checkboxTableHtml(cli,&ren,nf,-1,1);         
                
                cli->println("</tr>");
              }
            cli->println("</table><br>");

/* table switchs */

            cli->println("<table>");
              cli->println("<tr>");
              cli->println("<th>  </th><th> rem </th><th> per </th><th> sw </th><th>   </th>");
              cli->println("</tr>");
              
              for(int nb=0;nb<MAXREMLI;nb++){
                cli->println("<tr>");
                cli->print("<td>");cli->print(nb+1);cli->print("</td>");
                memcpy(nf,"remote_un_",LENNOM);nf[LENNOM-1]=(char)(nb+PMFNCHAR);
                numTableHtml(cli,'b',&remoteT[nb].remnum,nf,1,1,0);
                memcpy(nf,"remote_pn_",LENNOM);nf[LENNOM-1]=(char)(nb+PMFNCHAR);
                numTableHtml(cli,'b',&remoteT[nb].pernum,nf,2,1,0);
                memcpy(nf,"remote_sw_",LENNOM);nf[LENNOM-1]=(char)(nb+PMFNCHAR);
                numTableHtml(cli,'b',&remoteT[nb].persw,nf,1,1,0);
                memcpy(nf,"remote_xe_",LENNOM);nf[LENNOM-1]=(char)(nb+PMFNCHAR);
                uint8_t ren=(uint8_t)remoteT[nb].enable;
                checkboxTableHtml(cli,&ren,nf,-1,1);         
                
                cli->println("</tr>");
              }
              
            cli->println("</table>");            
            cli->println("</form></body></html>");
}

void testHtml(EthernetClient* cli)
{
            Serial.println(" page d'essais");
 htmlImg(cli,"sweeth.jpg");
            
            
/*            
            
            htmlIntro(nomserver,cli);

            char pwd[32]="password__=\0";strcat(pwd,usrpass);
            
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

            bouTableHtml(cli,"password__",usrpass,"boutable",2,1);
            
            cli->print("<form><p hidden><input type=\"text\" name=\"password__\" value=\"");
            cli->print(usrpass);
            cli->print("\" ><p><input type=\"submit\" value=\"submit\"><br></form>");
            
            
            cli->print("<a href=page.html?password__=17515A:><input type=\"submit\" value=\"retour\"></a><br>");

            cli->println("</form></body></html>");            
*/
}           
