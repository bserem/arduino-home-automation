// Daiseikai IR
#include <Arduino.h>
#include <CarrierHeatpumpIR.h>
IRSenderPWM irSender(9);     // IR led on Duemilanove digital pin 3, using Arduino PWM
//IRSenderBlaster irSender(9); // IR led on Duemilanove digital pin 3, using IR Blaster (generates the 38 kHz carrier)

DaiseikaiHeatpumpIR *heatpumpIR;

// Ethernet
#include <SPI.h>
#include <Ethernet.h>
byte mac[] = {
  0x74,0x69,0x69,0x2D,0x30,0x31 };  // W5100 MAC address
IPAddress ip(192,168,2,3);  // Arduino IP
EthernetClient webclient;
// Initialize the Ethernet server library
// with the IP address and port you want to use 
// (port 80 is default for HTTP):
EthernetServer server(80);
String httpRequest;

//#include <SD.h>

// HUE
const char hueHubIP[] = "192.168.2.2";  // Hue hub IP
const char hueUsername[] = "";  // Hue username
const int hueHubPort = 80;
//  Hue variables
boolean hueOn;  // on/off
int hueBri;  // brightness value
long hueHue;  // hue value
String hueCmd;  // Hue command

// DHT
#include "DHT.h"
#define DHTPIN 40     // what pin we're connected to
#define DHTTYPE DHT22   // DHT 22  (AM2302)
DHT dht(DHTPIN, DHTTYPE);

unsigned long buffer=0;  //buffer for received data storage
unsigned long addr;

// Visuals
int redLED = 6; // heat
int orangeLED = 5;  // ir is transmitting
int greenLED = 4; // on (auto if no other led)
int blueLED = 3; // cool

void setup()
{
  Serial.begin(9600);

  Ethernet.begin(mac,ip);
  server.begin();
  Serial.print("server is at ");
  Serial.println(Ethernet.localIP());

  // initialize SD card
  /*
  Serial.println("Initializing SD card...");
  if (!SD.begin(4)) {
    Serial.println("ERROR - SD card initialization failed!");
    return;    // init failed
  }
  Serial.println("SUCCESS - SD card initialized.");
  // check for index.htm file
  if (!SD.exists("index.htm")) {
    Serial.println("ERROR - Can't find index.htm file!");
    return;  // can't find index file
  }
  Serial.println("SUCCESS - Found index.htm file.");  
  */

  dht.begin();

  pinMode(redLED, OUTPUT);
  pinMode(orangeLED, OUTPUT);
  pinMode(greenLED, OUTPUT);
  pinMode(blueLED, OUTPUT);
  delay(500);

  heatpumpIR = new DaiseikaiHeatpumpIR();

  Serial.println(F("Starting"));
}

int freeRam () 
{
  extern int __heap_start, *__brkval; 
  int v; 
  return (int) &v - (__brkval == 0 ? (int) &__heap_start : (int) __brkval); 
}

void(* resetFunc) (void) = 0; //declare reset function @ address 0

void parseRequest(String httpRequest) {
  Serial.println("In parseRequest");
  Serial.println(httpRequest);
}

void acControl(int power, int mode, int temp) {
  const char* buf;

  Serial.print(F("Sending IR to "));
  // Print the model
  buf = heatpumpIR->model();
  // 'model' is a PROGMEM pointer, so need to write a byte at a time
  while (char modelChar = pgm_read_byte(buf++))
  {
    Serial.print(modelChar);
  }
  Serial.print(F(", info: "));
  // Print the info
  buf = heatpumpIR->info();  
  // 'info' is a PROGMEM pointer, so need to write a byte at a time
  while (char infoChar = pgm_read_byte(buf++))
  {
    Serial.print(infoChar);
  }
  Serial.println();

  digitalWrite(orangeLED,HIGH);
  digitalWrite(redLED, LOW);
  delay(50);
  heatpumpIR->send(irSender, power, mode, FAN_AUTO, temp, VDIR_AUTO, HDIR_AUTO);
  digitalWrite(orangeLED,LOW);
  digitalWrite(redLED, HIGH);
}

void loop()
{

  // DHT Initialize
  float h = dht.readHumidity();
  float t = dht.readTemperature();
  // check if returns are valid, if they are NaN (not a number) then something went wrong!
  if (isnan(t) || isnan(h)) {
    Serial.println("Failed to read from DHT");
  }

  // listen for incoming clients
  EthernetClient client = server.available();
  if (client) {
    Serial.println("new client");
    // an http request ends with a blank line
    boolean currentLineIsBlank = true;
    while (client.connected()) {
      if (client.available()) {
        char c = client.read();

        //read char by char HTTP request
        if (httpRequest.length() < 100) {
          //store characters to string
          httpRequest += c;
          //Serial.write(c);
        }

        // if you've gotten to the end of the line (received a newline
        // character) and the line is blank, the http request has ended,
        // so you can send a reply
        if (c == '\n' && currentLineIsBlank) {
          // send a standard http response header
          client.println("HTTP/1.1 200 OK");
          client.println("Content-Type: text/html");
          client.println("Connection: close");  // the connection will be closed after completion of the response
          // CAREFULL: YOU COULD BE SENDING IR COMMANDS TO YOUR AC CONSTANTLY
          //client.println("Refresh: 15");  // refresh the page automatically every 15 sec
          client.println();
          client.println("<!DOCTYPE HTML>");
          client.println("<html>");
          client.println("<head>");
          client.println("<title>Peggy</title>");
          client.println("<link rel='stylesheet' href='https://maxcdn.bootstrapcdn.com/bootstrap/3.3.7/css/bootstrap.min.css' integrity='sha384-BVYiiSIFeK1dGmJRAkycuHAHRg32OmUcww7on3RYdg4Va+PmSTsz/K68vbdEjh4u' crossorigin='anonymous'>");
          client.println("<script src='https://maxcdn.bootstrapcdn.com/bootstrap/3.3.7/js/bootstrap.min.js' integrity='sha384-Tc5IQib027qvyjSMfHjOMaLkfuWVxZxUPnCJA7l2mCWNIpG9mGCD8wGNIcPD7Txa' crossorigin='anonymous'></script>");
          client.println("</head>");
          client.println("<body class='container'>");
          client.println("<div class='row'>");
          client.println("<div class='col-xs-12'>");
          client.println("<h1 class='text-center'><a href='/'>Peggy</a></h1>");
          client.println("<hr>");
          client.println("</div>");
/*
          client.println("<div class='col-xs-12 col-md-6'>");
          client.println("<h2>AC Control</h2>");

          client.println("<form method=get>");
          client.print("<input type='radio' name=ac value='1'>On");
          client.print("<input type='radio' name=ac value='0' checked>Off<br>");
          client.print("<input type='radio' name=mode value='auto' checked>Auto");
          client.print("<input type='radio' name=mode value='cool'>Cool");
          client.print("<input type='radio' name=mode value='heat'>Heat<br>");
          //         client.print("<input type='radio' name=mode value='fan' checked>Fan<br>");
          client.print("Set Temperature to: <input type='text' name=temp value='24'><br>");
          client.print("<input type=submit value=submit>");
          client.println("</form>");

          client.println("</div>");
*/          
          client.println("<div class='col-xs-12 col-md-6'>");
          client.println("<H2>AC (24&deg;C)</H2>");
          client.println("<a class='btn btn-block btn-success btn-lg' href='/?acOnAuto'>Auto</a><br>");
          client.println("<a class='btn btn-block btn-warning btn-lg' href='/?acOff'>Off</a><br>");
          client.println("<a class='btn btn-block btn-primary' href='/?acOnCool'>Cool</a><br>");
          client.println("<a class='btn btn-block btn-danger' href='/?acOnHeat'>Heat</a><br>");
          client.println("</div>");
          
          client.println("<div class='col-xs-12 col-md-6'>");
          client.println("<H2>Lights</H2>");
          client.println("<a class='btn btn-block btn-info btn-lg' href='/?hueFloorOn'>Floor On</a><br>");
          client.println("<a class='btn btn-block btn-default btn-lg' href='/?hueFloorOff'>Floor Off</a><br>");
          client.println("<a class='btn btn-block btn-info' href='/?hueOn'>Ceiling On</a><br>");
          client.println("<a class='btn btn-block btn-default' href='/?hueOff'>Ceiling Off</a><br>");
          client.println("</div>");
          
          // Reading temperature or humidity takes about 250 milliseconds!
          // Sensor readings may also be up to 2 seconds 'old' (its a very slow sensor)
          client.println("<div class='col-xs-12 col-md-4 text-center'>");
          client.println("<H2>Conditions</H2>");
          client.print("Room Temperature: <strong>"); 
          client.print(t,1);
          client.println("&deg;C</strong>");
          client.println("<br />");
          client.print("Room Humidity: <strong>"); 
          client.print(h,1);
          client.println("%</strong>");
          client.println("</div>");

          client.println("<div class='col-xs-12 col-md-4 text-center'>");
          client.println("<H2>System</H2>");
          client.print("Available RAM: <strong>");
          client.print(freeRam());
          client.println("b</strong><br>");
          long days=0;
          long hours=0;
          long mins=0;
          long secs=0;
          secs = millis()/1000; //convect milliseconds to seconds
          mins=secs/60; //convert seconds to minutes
          hours=mins/60; //convert minutes to hours
          days=hours/24; //convert hours to days
          secs=secs-(mins*60); //subtract the coverted seconds to minutes in order to display 59 secs max
          mins=mins-(hours*60); //subtract the coverted minutes to hours in order to display 59 minutes max
          hours=hours-(days*24); //subtract the coverted hours to days in order to display 23 hours max
          //Display results
          client.println("Uptime:");
          if (days>0) // days will be displayed only if value is greater than zero
          {
            client.print(days);
            client.print(" days and :");
          }
          if (hours<10) client.print('0');
          client.print(hours);
          client.print(":");
          if (mins<10) client.print('0');
          client.print(mins);
          client.print(":");
          if (secs<10) client.print('0');
          client.println(secs);
          client.println("</div>");


          client.println("<div class='col-xs-12 col-md-4 text-center'>");
          client.println("<H2>Network Status</H2>");
          client.println("Public IP: ");
          client.println("<iframe src='https://api.ipify.org' height='20' width='100' style='border: none'></iframe>");
          client.println("<br>");
          client.println("Private IP: ");
          client.println(ip);
          client.println("</div>");

          client.println("<div class='col-xs-12 text-right'><hr>Compiled: ");
          client.println(__DATE__ ", " __TIME__);
          client.println("</div>");
          client.println("</div>");
          client.println("</body>");
          client.println("</html>");
          client.println("</body>");
          client.println("</html>");

          //controls the Arduino if you press the buttons
          if (httpRequest.indexOf("?acOnAuto") > 0){
            digitalWrite(greenLED, HIGH);
            digitalWrite(blueLED, LOW);
            digitalWrite(redLED, LOW);
            digitalWrite(orangeLED,HIGH);
            delay(50);
            heatpumpIR->send(irSender, POWER_ON, MODE_AUTO, FAN_AUTO, 24, VDIR_AUTO, HDIR_AUTO);
            digitalWrite(orangeLED,LOW);
          }

          if (httpRequest.indexOf("?acOnCool") > 0){
            digitalWrite(greenLED, HIGH);
            digitalWrite(blueLED, HIGH);
            digitalWrite(redLED, LOW);
            digitalWrite(orangeLED,HIGH);
            delay(50);
            heatpumpIR->send(irSender, POWER_ON, MODE_COOL, FAN_AUTO, 24, VDIR_AUTO, HDIR_AUTO);
            digitalWrite(orangeLED,LOW);
          }

          if (httpRequest.indexOf("?acOnHeat") > 0){
            digitalWrite(greenLED, HIGH);
            digitalWrite(redLED, HIGH);
            digitalWrite(blueLED, LOW);
            digitalWrite(orangeLED,HIGH);
            delay(50);
            heatpumpIR->send(irSender, POWER_ON, MODE_HEAT, FAN_AUTO, 24, VDIR_AUTO, HDIR_AUTO);
            digitalWrite(orangeLED,LOW);
          }

          if (httpRequest.indexOf("?acOff") > 0){
            digitalWrite(greenLED, LOW);
            digitalWrite(blueLED, LOW);
            digitalWrite(redLED, LOW);
            digitalWrite(orangeLED,HIGH);
            delay(50);
            heatpumpIR->send(irSender, POWER_OFF, MODE_AUTO, FAN_AUTO, 24, VDIR_AUTO, HDIR_AUTO);
            digitalWrite(orangeLED,LOW);
          }


          //clearing string for next read
          httpRequest="";

          break;
        }
        if (c == '\n') {
          // you're starting a new line
          currentLineIsBlank = true;
        } 
        else if (c != '\r') {
          // you've gotten a character on the current line
          currentLineIsBlank = false;
        }
      }
    }
    // give the web browser time to receive the data
    delay(1);
    // close the connection:
    client.stop();
    Serial.println("client disonnected");

  } 
}







