#include <SPI.h>
#include <RF24.h>

//#define USART_DEBUG

#define BROADCAST_PIPE 0xB3B4B5B6CDLL
#define TX_PIPE        0xB3B4B5B6F1LL
#define RX_PIPE        0x7878787878LL

#define CHANNEL_NUM    (88)
#define PA_LEVEL       RF24_PA_LOW

#define RX_PIPE_NUM         (0)
#define BROADCAST_PIPE_NUM  (1)

//nano:     CE 9  CSN 10 (receiver)
//esp8266:  CE D4 CSN D3 (sender/broadcaster)
const int pinCE  = 9;//9 on nano D4 on esp //This pin is used to set the nRF24 to standby (0) or active mode (1)
const int pinCSN = 10;//10 on nano D3 on esp//This pin is used to tell the nRF24 whether the SPI communication is a command or message to send out

RF24 radio(pinCE, pinCSN); // Create your nRF24 object or wireless SPI connection


int listen_broadcast(uint16_t timeout = 500);
bool receive(int *val, uint16_t timeout = 500);

int listen_broadcast(uint16_t timeout)
{
  int val = 0;
  uint8_t pipe_num = 0;

  radio.startListening();

  unsigned long started_at = millis();
  while ( millis() - started_at < timeout )
  {
    if ( radio.available(&pipe_num) )
    {      
      if ( pipe_num == BROADCAST_PIPE_NUM )
      {
        radio.read(&val, sizeof(int));
        return val;
      }
    }
  }

#ifdef USART_DEBUG
    Serial.println("listen_broadcast() timeout.");
#endif
  return 0;
}

bool receive(int *val, uint16_t timeout)
{
  //radio.openReadingPipe(0, TX_PIPE); // Already opened in setup()
  radio.startListening();

  uint8_t pipe_num = 0;
  unsigned long started_at = millis();
  while ( millis() - started_at < timeout )
  {
    if ( radio.available(&pipe_num) )
    {
      if ( pipe_num == RX_PIPE_NUM )
      {
        radio.read(val, sizeof(int));
        radio.stopListening();
        radio.write(val, sizeof(int));
        return true;
      }
    }
  }

#ifdef USART_DEBUG
    Serial.println("receive() timeout.");
#endif

  return false;
}

void setup()
{
  Serial.begin(115200);
  Serial.println("*** Receiver initializing... ***");
  
  radio.begin();  //Start the nRF24 module
  radio.setPALevel(PA_LEVEL);  // "short range setting" - increase if you want more range AND have a good power supply
  radio.setChannel(CHANNEL_NUM);          // the higher channels tend to be more "open"

  // Open up to six pipes for PRX to receive data
  radio.openReadingPipe(RX_PIPE_NUM, TX_PIPE);
  radio.openReadingPipe(BROADCAST_PIPE_NUM, BROADCAST_PIPE);
  radio.openWritingPipe(RX_PIPE);
  //radio.printDetails();

  radio.startListening();                 // Start listening for messages
  delay(1000);
}

void loop()
{
  int receivedVal = 0;
  int broadcastVal = 0;
  bool bReceived = false;
  
  broadcastVal = listen_broadcast(200);
  bReceived = receive(&receivedVal, 200);

  if ( broadcastVal != 0 )
    Serial.println("Broadcasted value = " + String(broadcastVal));
  if ( bReceived )
    Serial.println("   Received value = " + String(receivedVal));
}
