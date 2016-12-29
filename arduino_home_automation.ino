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
#include <ToshibaDaiseikaiHeatpumpIR.h>
IRSenderPWM irSender(9);     // IR led on Duemilanove digital pin 3, using Arduino PWM
//IRSenderBlaster irSender(9); // IR led on Duemilanove digital pin 3, using IR Blaster (generates the 38 kHz carrier)

ToshibaDaiseikaiHeatpumpIR *heatpumpIR;

// DHT
#include "DHT.h"
#define DHTPIN 2     // what pin we're connected to
#define DHTTYPE DHT22   // DHT 22  (AM2302)
DHT dht(DHTPIN, DHTTYPE);

// Ethernet
#include <SPI.h>
//#include "avr/pgmspace.h"
#include <Ethernet.h>
/* you can change the authentication realm by defining
 * WEBDUINO_AUTH_REALM before including WebServer.h */
#define WEBDUINO_AUTH_REALM "Climate control requires god mode!"
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
P(wrapper_start) = "<div class='container'><div class='row'>";
P(wrapper_end) = "</div></div>";
P(page_title_main) = "<h1 class='text-center'>AC Control</h1>";
P(p_centered_start) = "<p class='col-xs-12 text-center'>";
P(p_end) = "</div>";

P(page_main) = ""
"        <div class='col-xs-12'>"
"          <form name='input' action='send_ir.html' method='get'>"
"            <input type='hidden' name='power' value='0' checked>"
"            <input class='btn btn-block btn-danger btn-lg' type='submit' value='Off'>"
"          </form>"
"        </div>"
"        <div class='col-xs-12'><hr></div>"
"        <div class='col-xs-12'>"
"        <form name='input' action='send_ir.html' method='get'>"
"            <input class='btn btn-block btn-primary btn-lg' type='submit' value='On'>"
"          </div>"
"          <div class='form-group col-xs-12'>"
"            <input type='hidden' name='power' value='1' checked>"
"          </div>"
"          <div class='form-group col-xs-12 col-sm-6'>"
"            <label for='mode'>Mode</label>"
"            <select id='mode' name='mode' class='form-control input-lg'>"
"              <option value='1' selected='selected'>AUTO</option>"
"              <option value='2'>HEAT</option>"
"              <option value='3'>COOL</option>"
"              <option value='4'>DRY</option>"
"              <option value='5'>FAN</option>"
"            </select> "
"          </div>"
"          <div class='form-group col-xs-12 col-sm-6'>"
"            <label for='temperature'>Temperature</label>"
"            <select id='temperature' name='temperature' class='form-control input-lg'>"
"              <option value='16'>16</option>"
"              <option value='17'>17</option>"
"              <option value='18'>18</option>"
"              <option value='19'>19</option>"
"              <option value='20'>20</option>"
"              <option value='21'>21</option>"
"              <option value='22'>22</option>"
"              <option value='23'>23</option>"
"              <option value='24' selected='selected'>24</option>"
"              <option value='25'>25</option>"
"              <option value='26'>26</option>"
"              <option value='27'>27</option>"
"              <option value='28'>28</option>"
"              <option value='29'>29</option>"
"              <option value='30'>30</option>"
"            </select> "
"          </div>"
"          <div class='form-group col-xs-12 col-sm-4'>"
"            <label for='fan'>Fan speed</label>"
"            <select id='fan' name='fan' class='form-control input-xs'>"
"              <option value='1' selected='selected'>AUTO</option>"
"              <option value='2'>1</option>"
"              <option value='3'>2</option>"
"              <option value='4'>3</option>"
"              <option value='5'>4</option>"
"              <option value='6'>5</option>"
"            </select> "
"          </div>"
"          <div class='form-group col-xs-12 col-sm-4'>"
"            <label for='vswing'>Vertical Swing</label>"
"            <select id='vswing' name='vswing' class='form-control input-xs'>"
"              <option value='1' selected='selected' >AUTO</option>"
"              <option value='2' >UP</option>"
"              <option value='3' >MIDDLE UP</option>"
"              <option value='4' >MIDDLE</option>"
"              <option value='5' >MIDDLE DOWN</option>"
"              <option value='6' >DOWN</option>"
"            </select>"
"          </div>"
"          <div class='form-group col-xs-12 col-sm-4'>"
"            <label for='hswing'>Horizontal swing</label>"
"            <select id='hswing' name='hswing' class='form-control input-xs'>"
"              <option value='1' selected='selected'>AUTO</option>"
"              <option value='2' >MIDDLE</option>"
"              <option value='3' >LEFT</option>"
"              <option value='4' >LEFT MIDDLE</option>"
"              <option value='5' >RIGHT</option>"
"              <option value='6' >RIGHT MIDDLE</option>"
"            </select>"
"          </form>"
"        </div>";

P(page_command) = "<a href='/' class='btn btn-info btn-block btn-lg'>back to controls</a>";


P(page_header) = "<!DOCTYPE html>"
"<html>"
"  <head>"
"    <link rel='stylesheet' href='http://maxcdn.bootstrapcdn.com/bootstrap/3.3.7/css/bootstrap.min.css' integrity='sha384-BVYiiSIFeK1dGmJRAkycuHAHRg32OmUcww7on3RYdg4Va+PmSTsz/K68vbdEjh4u' crossorigin='anonymous'>"
"  </head>"
"<body>";

P(page_footer) = "</body>"
"</html>";

// The index.html web page. This presents the HTML form
void indexCmd(WebServer &server, WebServer::ConnectionType type, char *url_tail, bool tail_complete)
{
  server.httpSuccess();
  server.printP(page_header);
  server.printP(wrapper_start);
  server.printP(page_title_main);

  server.printP(p_centered_start);
  float t = dht.readTemperature();
  float f = dht.readHumidity();
  server.print("Room Temperature: ");
  server.print(t,1);
  server.print("&deg;C<br>");
  server.print("Room Humidity: ");
  server.print(f,1);
  server.print("%");
  server.printP(p_end);

  server.printP(page_main);

  server.printP(wrapper_end);
  server.printP(page_footer);
}

// The send_ir.html. This handles the parameters submitted by the form
void ACCmd(WebServer &server, WebServer::ConnectionType type, char *url_tail, bool tail_complete)
{
  /* username = user
   * password = user
   * display a page saying "Hello User"
   *
   * the credentials have to be concatenated with a colon like
   * username:password
   * and encoded using Base64 - this should be done outside of your Arduino
   * to be easy on your resources
   *
   * in other words: "dXNlcjp1c2Vy" is the Base64 representation of "user:user"
   */
  if (server.checkCredentials("dXNlcjp1c2Vy"))
  {
    server.httpSuccess();
    if (type != WebServer::HEAD)
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
      server.printP(page_header);
      server.printP(page_command);

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

      server.printP(page_command);
      server.printP(page_footer);
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
  }
  else
  {
    /* send a 401 error back causing the web browser to prompt the user for credentials */
    server.httpUnauthorized();
  }
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

  dht.begin();
  delay(1000);

  heatpumpIR = new ToshibaDaiseikaiHeatpumpIR();

  Serial.println(F("Running"));
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


