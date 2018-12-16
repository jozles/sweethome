#define _HTML_WK_
#ifdef _HTML_WK_

#include <Ethernet.h>
#include "utilether.h"


extern EthernetClient cli;
extern File fhtml;
//char lineBuff[128];

void htmlTest()
{

// **************** page passwd.html_
      //SD.remove("passw.txt");
      //strcpy(lineBuff,"<body><form method=\"get\" ><p><label for=\"password\">password</label> : <input type=\"password\"");htmlSave(&fhtml,"passw.txt",lineBuff);cli.println(lineBuff);
      //strcpy(lineBuff," name=\"password__\" id=\"password\" value=\"\" size=\"6\" maxlength=\"8\" ></p></form></body></html>");htmlSave(&fhtml,"passw.txt",lineBuff);cli.println(lineBuff);
      //htmlPrint(&cli,&fhtml,"passw.txt");

      // *** un espace avant target (fin de la liste)
// **************** page tech1a.html
      //SD.remove("tech1a.txt");
      //strcpy(lineBuff,"<a href=page.html?reset_____: target=_self>reset</a>  <br><br>\n");htmlSave(&fhtml,"tech1a.txt",lineBuff);cli.println(lineBuff);
      //strcpy(lineBuff,"<a href=page.html?stop_wr___: target=_self>start/stop write</a>  <br>\n");htmlSave(&fhtml,"tech1a.txt",lineBuff);cli.println(lineBuff);
      //strcpy(lineBuff,"<a href=page.html?sd_da_seul: target=_self>data_save seul</a>  <br>\n");htmlSave(&fhtml,"tech1a.txt",lineBuff);cli.println(lineBuff);
      //strcpy(lineBuff,"<a href=page.html?dump_sd___: target=_self>dump SDcard</a> (");htmlSave(&fhtml,"tech1a.txt",lineBuff);cli.println(lineBuff);
      
      //cli.println("<a href=page.html?reset_____: target=_self>reset</a>  <br><br>");
      //cli.println("<a href=page.html?stop_wr___: target=_self>stop write</a>  <br>");
      //cli.println("<a href=page.html?start_wr__: target=_self>start write</a>  <br>");
      //cli.println("<a href=page.html?dump_sd___: target=_self>dump SDcard</a> (");
      //htmlPrint(&cli,&fhtml,"tech1a.txt");
      //sdsiz=fhisto.size();cli.print(sdsiz);
// **************** page tech1b.html
      //SD.remove("tech1b.txt");
      //strcpy(lineBuff,") <br>\n<form method=\"get\" >");htmlSave(&fhtml,"tech1b.txt",lineBuff);cli.println(lineBuff);
      //strcpy(lineBuff,"<p><label for=\"pos_SD\">position SD</label> : <input type=\"text\" name=\"sd_pos____\" id=\"pos_SD\" value=\"");htmlSave(&fhtml,"tech1b.txt",lineBuff);cli.println(lineBuff);
      //cli.print(") <br>");
      //cli.println("<form method=\"get\" ");
      //cli.print("<p>");cli.print("<label for=\"pos_SD\">position SD</label> : <input type=\"text\" name=\"sd_pos____\" id=\"pos_SD\" value=\"");
      //htmlPrint(&cli,&fhtml,"tech1b.txt");
      //cli.print(sdpos);
// **************** page tech1c.html
      //SD.remove("tech1c.txt");
      //strcpy(lineBuff,"\" size=\"3\" maxlength=\"6\"  /><br></p></form>\n");htmlSave(&fhtml,"tech1c.txt",lineBuff);cli.println(lineBuff);
      //strcpy(lineBuff,"<a href=page.html?stop_aff__: target=_self>stop aff</a> <br>\n<form method=\"get\" >");htmlSave(&fhtml,"tech1c.txt",lineBuff);cli.println(lineBuff);
      //strcpy(lineBuff,"<p>\n<label for=\"af_flags\">flags affic</label> : <input type=\"text\" name=\"aff_flags_\" id=\"af_flags\" value=\"");htmlSave(&fhtml,"tech1c.txt",lineBuff);cli.println(lineBuff);
      //cli.println("\" size=\"3\" maxlength=\"6\"  /><br></p></form>");
      //cli.println("<a href=page.html?stop_aff__: target=_self>stop aff</a>   <br>");
      //cli.println("<form method=\"get\" ");
      //cli.print("<p><label for=\"af_flags\">flags affic</label> : <input type=\"text\" name=\"aff_flags_\" id=\"af_flags\" value=\"");
      //htmlPrint(&cli,&fhtml,"tech1c.txt");
      //char mask[4];sprintf(mask,"%d",affmask);
      //cli.print(mask);
      //cli.print("\" size=\"1\" maxlength=\"3\"  />");
//****************** page tech1d.html
      //strcpy(lineBuff,"\" size=\"1\" maxlength=\"3\"  /> ");htmlSave(&fhtml,"tech1d.txt",lineBuff);cli.println(lineBuff);
      //htmlPrint(&cli,&fhtml,"tech1d.txt");
      //strcpy(valeur,"ON");if(afficser==FAUX){strcpy(valeur,"OFF");}cli.print("  ");cli.print(valeur);
// ***************** page tech1e.html
      //SD.remove("tech1e.txt");
      //strcpy(lineBuff,"<br>Serial  (1 date, 2 getnv, 4 t/h, 8 dumpSD, 16 SDstore, 32 SDerr, 64 SDhtml)<br></p></form>\n");htmlSave(&fhtml,"tech1e.txt",lineBuff);cli.println(lineBuff);
      //strcpy(lineBuff,"<a href=page.html?stop_refr_: target=_self>stop refr</a> <br>\n<form method=\"get\" >");htmlSave(&fhtml,"tech1e.txt",lineBuff);cli.println(lineBuff);
      //strcpy(lineBuff,"<p><label for=\"per_refr\">per. affich</label> : <input type=\"text\" name=\"per_refr__\" id=\"per_refr\" value=\"");htmlSave(&fhtml,"tech1e.txt",lineBuff);cli.println(lineBuff);
      //cli.println("<br>Serial  (1 date, 2 getnv, 4 t/h, 8 dumpSD, 16 SDstore, 32 SDerr, 64 SDhtml)<br></p></form>");
      //cli.println("<a href=page.html?stop_refr_: target=_self>stop refr</a> <br><form method=\"get\" ");
      //cli.print("<p><label for=\"per_refr\">per. affich</label> : <input type=\"text\" name=\"per_refr__\" id=\"per_refr\" value=\"");
      //htmlPrint(&cli,&fhtml,"tech1e.txt");
      //cli.print(perrefr);
// ******************** page texh1f.html
      //SD.remove("tech1f.txt");
      //strcpy(lineBuff,"\" size=\"1\" maxlength=\"3\"  />");htmlSave(&fhtml,"tech1f.txt",lineBuff);cli.println(lineBuff);
      //cli.print("\" size=\"1\" maxlength=\"3\"  />");
      //htmlPrint(&cli,&fhtml,"tech1f.txt");
      //strcpy(valeur,"ON");if(perrefr==0){strcpy(valeur,"OFF");}cli.print("  ");
      //cli.print(valeur);
// ********************* page tech1g.html
      //SD.remove("tech1g.txt");
      //strcpy(lineBuff,"<br></p></form>\n<form method=\"get\" ><p><label for=\"per_temp\">temp. per.</label> : <input type=\"text\" name=\"per_temp__\" id=\"per_temp\" value=\"");htmlSave(&fhtml,"tech1g.txt",lineBuff);cli.println(lineBuff);
      //cli.println("<br></p></form>\n<form method=\"get\" <p><label for=\"per_temp\">temp. per.</label> : <input type=\"text\" name=\"per_temp__\" id=\"per_temp\" value=\"");
      //htmlPrint(&cli,&fhtml,"tech1g.txt");
      //cli.print(pertemp);
// ********************* page tech1h.html
      //SD.remove("tech1h.txt");
      //strcpy(lineBuff,"\" size=\"1\" maxlength=\"4\"  /> sec<br></p></form>\n<form method=\"get\" >");htmlSave(&fhtml,"tech1h.txt",lineBuff);cli.println(lineBuff);
      //strcpy(lineBuff,"<p><label for=\"per_date\">heure per.</label> : <input type=\"text\" name=\"per_date__\" id=\"per_date\" value=\"");htmlSave(&fhtml,"tech1h.txt",lineBuff);cli.println(lineBuff);
      //cli.println("\" size=\"1\" maxlength=\"4\"  /> sec<br></p></form>\n<form method=\"get\" ");
      //cli.print("<p><label for=\"per_date\">heure per.</label> : <input type=\"text\" name=\"per_date__\" id=\"per_date\" value=\"");
      //htmlPrint(&cli,&fhtml,"tech1h.txt");
      //cli.print(perdate);
// ********************** page tech1j.html
      //SD.remove("tech1j.txt");
      //strcpy(lineBuff,"\" size=\"1\" maxlength=\"4\"  /> min<br></p></form>\n</body></html>");htmlSave(&fhtml,"tech1j.txt",lineBuff);cli.println(lineBuff);
      //cli.print("\" size=\"1\" maxlength=\"4\"  /> min<br></p></form>\n</body></html>");
      //htmlPrint(&cli,&fhtml,"tech1j.txt");
// *********************** page err501.html
      //SD.remove("err501.txt");
      //strcpy(lineBuff,"<body><br><br> err. 501 Not Implemented <br><br></body></html>");htmlSave(&fhtml,"err501.txt",lineBuff);cli.println(lineBuff);
      //htmlPrint(&cli,&fhtml,"err501.txt");
// *********************** page err400.html
      //SD.remove("err400.txt");
      //strcpy(lineBuff,"<body><br><br> err. 400 Bad Request <br><br></body></html>");htmlSave(&fhtml,"err400.txt",lineBuff);cli.println(lineBuff);
      //htmlPrint(&cli,&fhtml,"err400.txt");



}
#endif // _HTML_WK_
