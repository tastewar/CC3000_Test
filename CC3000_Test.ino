//#define ENABLE_CC3K_PRINTER
//#define DEBUG_MODE
#include <Adafruit_CC3000.h>
#include <ccspi.h>
#include <SPI.h>
#include <string.h>
#include <TinyXML.h>
#include "utility/debug.h"

// These are the interrupt and control pins
#define ADAFRUIT_CC3000_IRQ   3  // MUST be an interrupt pin!
// These can be any two pins
#define ADAFRUIT_CC3000_VBAT  5
#define ADAFRUIT_CC3000_CS    10

#define XMLBUFLEN             150

Adafruit_CC3000 cc3000 = Adafruit_CC3000(ADAFRUIT_CC3000_CS, 
ADAFRUIT_CC3000_IRQ, 
ADAFRUIT_CC3000_VBAT,
SPI_CLOCK_DIVIDER);

Adafruit_CC3000_Client client;
TinyXML xml;
unsigned char  xmlbuf[XMLBUFLEN];

const unsigned long
dhcpTimeout     = 60L * 1000L, // Max time to wait for address from DHCP
connectTimeout  = 15L * 1000L, // Max time to wait for server connection
responseTimeout = 15L * 1000L; // Max time to wait for data from server

void setup(void)
{
  uint32_t ip = 0L, t;
  xml.init(xmlbuf,sizeof(xmlbuf),&XML_callback);
  Serial.begin(115200);
  while ( !Serial.available() );
  Serial.println(F("Hello, CC3000!\n")); 

  /* Try to reconnect using the details from SmartConfig          */
  /* This basically just resets the CC3000, and the auto connect  */
  /* tries to do it's magic if connections details are found      */
  Serial.println(F("Trying to reconnect using SmartConfig values ..."));

  /* !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!! */
  /* !!! Note the additional arguments in .begin that tell the   !!! */
  /* !!! app NOT to deleted previously stored connection details !!! */
  /* !!! and reconnected using the connection details in memory! !!! */
  /* !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!! */
  if (!cc3000.begin(false, true))
  {
    Serial.println(F("Unable to re-connect!? Did you run the SmartConfigCreate"));
    Serial.println(F("sketch to store your connection details?"));
    while(1);
  }

  /* Round of applause! */
  Serial.println(F("Reconnected!"));

  /* Wait for DHCP to complete */
  Serial.println(F("\nRequesting DHCP"));
  while (!cc3000.checkDHCP()) {
    delay(100); // ToDo: Insert a DHCP timeout!
  }

  /* Display the IP address DNS, Gateway, etc. */
  while (! displayConnectionDetails()) {
    delay(1000);
  }
  // Look up server's IP address
  Serial.println(F("\r\nGetting server IP address."));
  t = millis();
  Serial.print("t=");
  Serial.println(t);
  Serial.print("connectTimeout=");
  Serial.println(connectTimeout);
  while((0L == ip) && ((millis() - t) < connectTimeout))
  {
    Serial.print ( "!" );
    if(cc3000.getHostByName("freegeoip.net", &ip)) break;
    Serial.print ( "." );
    delay(1000);
  }
  Serial.println ( );
  cc3000.printIPdotsRev(ip);
  Serial.println ( );
  if(0L == ip)
  {
    Serial.println(F("failed"));
    return;
  }
  cc3000.printIPdotsRev(ip);
  Serial.println();

  // Request XML-formatted data from server (port 80)
  Serial.print(F("Connecting to geo server..."));
  client = cc3000.connectTCP(ip, 80);
  if(client.connected())
  {
    Serial.print(F("connected.\r\nRequesting data..."));
    client.print(F("GET /xml/ HTTP/1.0\r\nConnection: close\r\n\r\n"));
  }
  else
  {
    Serial.println(F("failed connecting to host."));
    return;
  }

  Serial.print("\r\nReading response...");
  while (true)
  {
    if (client.available())
    {
      char c = client.read();
      //Serial.print(c);
      if (c=='<')
      {
        xml.processChar('<');
        break;
      }
    }
  }
  Serial.println("Waiting for response");
  while (true)
  {
    if (client.available())
    {
      char c = client.read();
      //Serial.print(c);
      xml.processChar(c);
    }
  }
  client.close();
  Serial.println(F("OK"));

  /* You need to make sure to clean up after yourself or the CC3000 can freak out */
  /* the next time your try to connect ... */
  Serial.println(F("\nDisconnecting"));
  cc3000.disconnect();
}

void loop ( )
{
}

bool displayConnectionDetails(void) {
  uint32_t ipAddress, netmask, gateway, dhcpserv, dnsserv;

  if(!cc3000.getIPAddress(&ipAddress, &netmask, &gateway, &dhcpserv, &dnsserv)) {
    Serial.println(F("Unable to retrieve the IP Address!\r\n"));
    return false;
  } 
  else {
    Serial.print(F("\nIP Addr: ")); 
    cc3000.printIPdotsRev(ipAddress);
    Serial.print(F("\nNetmask: ")); 
    cc3000.printIPdotsRev(netmask);
    Serial.print(F("\nGateway: ")); 
    cc3000.printIPdotsRev(gateway);
    Serial.print(F("\nDHCPsrv: ")); 
    cc3000.printIPdotsRev(dhcpserv);
    Serial.print(F("\nDNSserv: ")); 
    cc3000.printIPdotsRev(dnsserv);
    Serial.println();
    return true;
  }
}

void XML_callback( uint8_t statusflags, char* tagName,  uint16_t tagNameLen,  char* data,  uint16_t dataLen )
{
  if (statusflags & STATUS_START_TAG)
  {
    if ( tagNameLen )
    {
      Serial.print("Start tag ");
      Serial.println(tagName);
    }
  }
  else if  (statusflags & STATUS_END_TAG)
  {
    Serial.print("End tag ");
    Serial.println(tagName);
  }
  else if  (statusflags & STATUS_TAG_TEXT)
  {
    Serial.print("Tag:");
    Serial.print(tagName);
    Serial.print(" text:");
    Serial.println(data);
  }
  else if  (statusflags & STATUS_ATTR_TEXT)
  {
    Serial.print("Attribute:");
    Serial.print(tagName);
    Serial.print(" text:");
    Serial.println(data);
  }
  else if  (statusflags & STATUS_ERROR)
  {
    Serial.print("XML Parsing error  Tag:");
    Serial.print(tagName);
    Serial.print(" text:");
    Serial.println(data);
  }
}


