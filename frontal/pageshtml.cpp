#include <Arduino.h>
#include <SD.h>
#include "const.h"
#include <Wire.h>
#include "utilether.h"
#include "shconst.h"
#include "shutil.h"
#include "periph.h"
#include "httphtml.h"

extern char* nomserver;

extern int  perrefr;
extern File fhisto;      // fichier histo sd card
extern long sdpos;
extern char strSD[RECCHAR];
extern char ssid[MAXSSID*(LENSSID+1)];   
extern char passssid[MAXSSID*(LENSSID+1)];


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

            cli->print(".button { background-color: #195B6A; border: none; color: white; padding: 16px 40px;");
            cli->println("text-decoration: none; font-size: 30px; margin: 2px; cursor: pointer;}");
            cli->println(".button2 {background-color: #77878A;");

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
  
  Serial.print("histoSD ");Serial.println(sdsiz);

  htmlIntro(nomserver,cli);cli->println("<body>");

  cli->print("histoSD ");cli->print(sdsiz);cli->print(" ");cli->print(pos);cli->print("/");cli->print(sdpos);cli->print("<br>\n");
  
  int cnt=0,strSDpt=0;
  int ignore=VRAI; // ignore until next line
  long ptr=sdpos;
  while (ptr<sdsiz || inch==-1)
    {
    inch=fhisto.read();ptr++;
#ifndef WEMOS
//    cnt++;if(cnt>5000){wdt_reset();cnt=0;}  
#endif ndef WEMOS
    if(!ignore)
      {
        
      strSD[strSDpt]=inch;strSDpt++;strSD[strSDpt]='\0';
      if((strSDpt>=(RECCHAR-1)) || (strSD[strSDpt-2]=='\r' && strSD[strSDpt-1]=='\n')){
        cli->println(strSD);                                                                                                                                                                                                        
        memset(strSD,'\0',RECCHAR);strSDpt=0;
        Serial.print(inch);
        }
      }
      else if (inch==10){ignore=FAUX;}
    }
  cli->println(strSD);

  cli->println("</body>");

  return sdOpen(FILE_WRITE,&fhisto,"fdhisto.txt");
  return SDOK;
}

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
            lnkTableHtml(cli,"password__=17515A","retour");cli->println("<br>");
            
            cli->println("<table>");
              cli->println("<tr>");
              cli->println("<th>   </th><th>      SSID      </th><th>  password  </th>");
              cli->println("</tr>");

              for(int nb=0;nb<MAXSSID;nb++){
                cli->println("<tr>");
                  cli->print("<td>");cli->print(nb);cli->print("</td>");
                  cli->print("<td><input type=\"text\" name=\"ssid_____");cli->print((char)(nb+PMFNCHAR));cli->print("\" value=\"");    
                      cli->print((ssid+nb*(LENSSID+1)));cli->print("\" size=\"12\" maxlength=\"");cli->print(LENSSID);cli->println("\" ></td><td>");
                  cli->println("<p>pass <input type=\"password\" name=\"passssid_");cli->print((char)(nb+PMFNCHAR));cli->print("\" value=\"");
                      cli->print((passssid+nb*(LPWSSID+1)));cli->print("\" size=\"8\" maxlength=\"");cli->print(LPWSSID);cli->println("\" ></p></td>");
                  cli->println("<td><input type=\"submit\" value=\"MàJ\"></td>");
                cli->println("</tr>");
              }
            cli->println("</table>");            
            cli->println("</form></body></html>");

}

