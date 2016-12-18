// Length parameters
// * name of parameter
// * value of parameter
// * overall lenght of the 'GET' type URL
#define NAMELEN 32
#define VALUELEN 32
#define URLLEN 128

#define DEFAULT_TEMPERATURE 24

// Daiseikai IR
#include <Arduino.h>
#include <CarrierHeatpumpIR.h>
IRSenderPWM irSender(9);     // IR led on Duemilanove digital pin 3, using Arduino PWM
//IRSenderBlaster irSender(9); // IR led on Duemilanove digital pin 3, using IR Blaster (generates the 38 kHz carrier)

DaiseikaiHeatpumpIR *heatpumpIR;

// Ethernet
#include <SPI.h>
//#include "avr/pgmspace.h"
#include <Ethernet.h>
#include <WebServer.h>
static uint8_t mac[] = {
  0x74, 0x69, 0x69, 0x2D, 0x30, 0x31 };  // W5100 MAC address
static uint8_t ip[] = { 
  192, 168, 2, 3 };  // Arduino IP
/* This creates an instance of the webserver.  By specifying a prefix
 * of "", all pages will be at the root of the server. */
#define PREFIX ""
WebServer webserver(PREFIX, 9093);

// flash-based web pages to save precious RAM
P(indexpage) = "<!DOCTYPE html>\n"
"<html>\n"
"<body>\n"
"\n"
"\n"
"<table border=\"1\">\n"
"<tr><td>\n"
"AC Control\n"
"<form name=\"input\" action=\"send_ir.html\" method=\"get\">\n"
"<table border=\"1\">\n"
"<tr><td>\n"
"<input type=\"radio\" name=\"power\" value=\"1\">ON\n"
"<input type=\"radio\" name=\"power\" value=\"0\" checked>OFF\n"
"</td><td>\n"
"Power state\n"
"</td></tr>\n"
"\n"
"<tr><td>\n"
"<select name=\"mode\">\n"
"  <option value=\"1\">AUTO</option>\n"
"  <option value=\"2\" selected=\"selected\">HEAT</option>\n"
"  <option value=\"3\">COOL</option>\n"
"  <option value=\"4\">DRY</option>\n"
"  <option value=\"5\">FAN</option>\n"
"</select> \n"
"</td><td>\n"
"Mode\n"
"</td></tr>\n"
"\n"
"<tr><td>\n"
"<select name=\"fan\">\n"
" <option value=\"1\" selected=\"selected\">AUTO</option>\n"
" <option value=\"2\">1</option>\n"
" <option value=\"3\">2</option>\n"
" <option value=\"4\">3</option>\n"
" <option value=\"5\">4</option>\n"
" <option value=\"6\">5</option>\n"
"</select> \n"
"</td><td>\n"
"Fan speed\n"
"</td></tr>\n"
"\n"
"<tr><td>\n"
"<select name=\"temperature\">\n"
"  <option value=\"16\">16</option>\n"
"  <option value=\"17\">17</option>\n"
"  <option value=\"18\">18</option>\n"
"  <option value=\"19\">19</option>\n"
"  <option value=\"20\">20</option>\n"
"  <option value=\"21\">21</option>\n"
"  <option value=\"22\">22</option>\n"
"  <option value=\"23\">23</option>\n"
"  <option value=\"24\" selected=\"selected\">24</option>\n"
"  <option value=\"25\">25</option>\n"
"  <option value=\"26\">26</option>\n"
"  <option value=\"27\">27</option>\n"
"  <option value=\"28\">28</option>\n"
"  <option value=\"29\">29</option>\n"
"  <option value=\"30\">30</option>\n"
"</select> \n"
"</td><td>\n"
"Temperature\n"
"</td></tr>\n"
"\n"
"<tr><td>\n"
"<select name=\"vswing\">\n"
"  <option value=\"1\" selected=\"selected\" >AUTO</option>\n"
"  <option value=\"2\" >UP</option>\n"
"  <option value=\"3\" >MIDDLE UP</option>\n"
"  <option value=\"4\" >MIDDLE</option>\n"
"  <option value=\"5\" >MIDDLE DOWN</option>\n"
"  <option value=\"6\" >DOWN</option>\n"
"</select>\n"
"</td><td>\n"
"Vertical swing\n"
"</td></tr>\n"
"\n"
"<tr><td>\n"
"<select name=\"hswing\">\n"
"  <option value=\"1\" selected=\"selected\">AUTO</option>\n"
"  <option value=\"2\" >MIDDLE</option>\n"
"  <option value=\"3\" >LEFT</option>\n"
"  <option value=\"4\" >LEFT MIDDLE</option>\n"
"  <option value=\"5\" >RIGHT</option>\n"
"  <option value=\"6\" >RIGHT MIDDLE</option>\n"
"</select>\n"
"</td><td>\n"
"Horizontal swing\n"
"</td></tr>\n"
"\n"
"</td></tr>\n"
"</table>\n"
"<input type=\"submit\" value=\"Submit\">\n"
"</form>\n"
"</td></tr>\n"
"\n"
"</table>\n"
"\n"
"</body>\n"
"</html>\n";

P(formactionpage_header) = "<!DOCTYPE html>"
"<html>"
"<body>"
"<h1>Parameters</h1>";

P(formactionpage_footer) = ""
"</body>"
"</html>";

// The index.html web page. This presents the HTML form
void indexCmd(WebServer &server, WebServer::ConnectionType type, char *url_tail, bool tail_complete)
{
  server.httpSuccess();
  server.printP(indexpage);
}

// The send_ir.html. This handles the parameters submitted by the form
void ACCmd(WebServer &server, WebServer::ConnectionType type, char *url_tail, bool tail_complete)
{
  URLPARAM_RESULT rc;
  char name[NAMELEN];
  char value[VALUELEN];
  int param = 0;

  // Sensible defaults for the heat pump mode

  byte powerState    = POWER_OFF;
  byte operatingMode = MODE_AUTO;
  byte fanSpeed      = FAN_AUTO;
  byte temperature   = DEFAULT_TEMPERATURE;
  byte swingV        = VDIR_AUTO;
  byte swingH        = HDIR_AUTO;

  server.httpSuccess();
  server.printP(formactionpage_header);

  if (strlen(url_tail))
    {
    while (strlen(url_tail))
      {
      rc = server.nextURLparam(&url_tail, name, NAMELEN, value, VALUELEN);
      if (rc != URLPARAM_EOS)
        {
          if (strcmp(name, "power") == 0 )
          {
            param = atoi(value);
 
            switch (param)
            {
              case 0:
                powerState = POWER_OFF;
                break;
              case 1:
                powerState = POWER_ON;
                break;
            }
          }
          else if (strcmp(name, "mode") == 0 )
          {
            param = atoi(value);
            
            switch (param)
            {
              case 1:
                operatingMode = MODE_AUTO;
                break;
              case 2:
                operatingMode = MODE_HEAT;
                break;
              case 3:
                operatingMode = MODE_COOL;
                break;
              case 4:
                operatingMode = MODE_DRY;
                break;
              case 5:
                operatingMode = MODE_FAN;
                break;
            }
          }
          else if (strcmp(name, "fan") == 0 )
          {
            param = atoi(value);
            
            switch (param)
            {
              case 1:
                fanSpeed = FAN_AUTO;
                break;
              case 2:
                fanSpeed = FAN_1;
                break;
              case 3:
                fanSpeed = FAN_2;
                break;
              case 4:
                fanSpeed = FAN_3;
                break;
              case 5:
                fanSpeed = FAN_4;
                break;
              case 6:
                fanSpeed = FAN_5;
                break;
            }
          }
          else if (strcmp(name, "temperature") == 0 )
          {
            param = atoi(value);
            if ( param >= 15 && param <= 31 && temperature == DEFAULT_TEMPERATURE)
            {
              temperature = param;
            }
          }
          else if (strcmp(name, "vswing") == 0 )
          {
            param = atoi(value);
        
            switch (param)
            {
              case 1:
                swingV = VDIR_AUTO;
                break;
              case 2:
                swingV = VDIR_UP;
                break;
              case 3:
                swingV = VDIR_MUP;
                break;
              case 4:
                swingV = VDIR_MIDDLE;
                break;
              case 5:
                swingV = VDIR_MDOWN;
                break;
              case 6:
                swingV = VDIR_DOWN;
                break;
            }
          }
          else if (strcmp(name, "hswing") == 0 )
          {
            param = atoi(value);

            switch (param)
            {
              case 1:
                swingH = HDIR_AUTO;
                break;
              case 2:
                swingH = HDIR_MIDDLE;
                break;
              case 3:
                swingH = HDIR_LEFT;
                break;
              case 4:
                swingH = HDIR_MLEFT;
                break;
              case 5:
                swingH = HDIR_RIGHT;
                break;
              case 6:
                swingH = HDIR_MRIGHT;
                break;
            }
          }          
        server.print(name);
        server.print(" = ");
        server.print(param);
        server.print("<br>");
        }
      }
    }

  server.printP(formactionpage_footer);
/*  
  Serial.println("Values:");
  Serial.println("Power: ");
  Serial.println(powerState);
  Serial.println("Mode: ");
  Serial.println(operatingMode);
  Serial.println("Fan: ");
  Serial.println(fanSpeed);
  Serial.println("Temp: ");
  Serial.println(temperature);
  Serial.println("SwingV: ");
  Serial.println(swingV);
  Serial.println("SwingH: ");
  Serial.println(swingH);
*/
  heatpumpIR->send(irSender, powerState, operatingMode, fanSpeed, temperature, swingV, swingH);
}

// The setup
void setup()
{
  Serial.begin(9600);

  Ethernet.begin(mac,ip);
  // Setup the default and the index page
  webserver.setDefaultCommand(&indexCmd);
  webserver.addCommand("index.html", &indexCmd);
  // The form handlers
  webserver.addCommand("send_ir.html", &ACCmd);
  // start the webserver
  webserver.begin();

  Serial.print("server is at ");
  Serial.println(Ethernet.localIP());

  heatpumpIR = new DaiseikaiHeatpumpIR();

  Serial.println(F("Starting"));
}


// The loop
void loop()
{
  // URL buffer for GET parameters
  char buff[URLLEN];
  int len = URLLEN;

  // process incoming connections one at a time forever
  webserver.processConnection(buff, &len);
}

