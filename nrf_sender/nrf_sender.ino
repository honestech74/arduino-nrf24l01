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
#define RETRANSMIT_COUNT    (3)

//nano:     CE 9  CSN 10 (receiver)
//esp8266:  CE D4 CSN D3 (sender/broadcaster)
const int pinCE  = D4;//9 on nano //This pin is used to set the nRF24 to standby (0) or active mode (1)
const int pinCSN = D3;//10 on nano //This pin is used to tell the nRF24 whether the SPI communication is a command or message to send out

RF24 radio(pinCE, pinCSN); // Create your nRF24 object or wireless SPI connection

void broadcast(int val);
bool transmit(int val, int timeout=500);

void broadcast(int val)
{
  radio.stopListening();
  radio.openWritingPipe(BROADCAST_PIPE);
  bool success = radio.write(&val, sizeof(int), true); // multicast=true
  
  #ifdef USART_DEBUG
  if (success) Serial.println("broadcast() : Write success");
  else         Serial.println("broadcast() : Write failed");
  #endif
}

bool transmit(int val, int timeout)
{
  radio.openWritingPipe(TX_PIPE);        //open writing or transmit pipe
    
  uint8_t retryCount = RETRANSMIT_COUNT;
  while ( retryCount )
  {
    // Stop listening for transmitting
    radio.stopListening();
    
    // Write
    bool success = radio.write(&val, sizeof(int));
    
    #ifdef USART_DEBUG
    if ( success )  Serial.println("transmit() : Write success");
    else            Serial.println("transmit() : Write failed");
    #endif
  
    // Now, start listening for getting a reply
    radio.startListening();
  
    // Wait here until we get a response, or timeout
    unsigned long started_at = millis();
    uint8_t pipe_num = 0;
    while ( millis() - started_at < timeout )
    {
      if ( radio.available(&pipe_num) )
      {
        if ( pipe_num == RX_PIPE_NUM )
        {
          int received_val = 0;
          radio.read(&received_val, sizeof(int));
                    
          if ( received_val == val )
          {            
            #ifdef USART_DEBUG
            Serial.println("transmit() : Transmit success");
            #endif
            return true;
          }
        }
      }
    }
    
    retryCount--;
  }
  return false;
}
void setup()  
{
  Serial.begin(115200);   //start serial to communicate process
  Serial.println("*** Transmitter initializing... ***");

  radio.begin();            //Start the nRF24 module
  radio.setPALevel(PA_LEVEL);  // "short range setting" - increase if you want more range AND have a good power supply
  radio.setChannel(CHANNEL_NUM);          // the higher channels tend to be more "open"
  radio.openWritingPipe(TX_PIPE);
  radio.openReadingPipe(RX_PIPE_NUM, RX_PIPE);  //open reading or receive pipe
  radio.stopListening(); //go into transmit mode
  //radio.printDetails(); //output radio details
  delay(1000);
}

int broadcastVal = 0;
int transmitVal = 100;

void loop()
{
  bool bSuccess = false;
  broadcast(broadcastVal++);
  delay(500);

  Serial.println("Broadcasted value : " + String(broadcastVal));
  bSuccess = transmit(transmitVal++);
  if ( bSuccess )
    Serial.println("Transmitted value : " + String(transmitVal));
  delay(500);
}
