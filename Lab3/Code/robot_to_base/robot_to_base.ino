#include <SPI.h>
#include "nRF24L01.h"
#include "RF24.h"
#include "printf.h"

//
// Hardware configuration
//

// Set up nRF24L01 radio on SPI bus plus pins 9 & 10

RF24 radio(9, 10);

// Radio pipe addresses for the 2 nodes to communicate.
const uint64_t pipes[2] = { 0x000000002ALL, 0x000000002BLL };

void setup(void)
{
  Serial.begin(9600);
  printf_begin();

  // Setup and configure rf radio
  radio.begin();

  // optionally, increase the delay between retries & # of retries
  radio.setRetries(15, 15);
  radio.setAutoAck(true);
  // set the channel
  radio.setChannel(0x50);
  // set the power
  // RF24_PA_MIN=-18dBm, RF24_PA_LOW=-12dBm, RF24_PA_MED=-6dBM, and RF24_PA_HIGH=0dBm.
  radio.setPALevel(RF24_PA_MAX);
  //RF24_250KBPS for 250kbs, RF24_1MBPS for 1Mbps, or RF24_2MBPS for 2Mbps
  radio.setDataRate(RF24_2MBPS);

  // optionally, reduce the payload size.  seems to
  // improve reliability
  radio.setPayloadSize(8);

  radio.openWritingPipe(pipes[0]);
  radio.openReadingPipe(1, pipes[1]);

  // Dump the configuration of the rf unit for debugging
  //  radio.printDetails();
//  delay(3000);
}

void loop(void)
{
  radio.stopListening();
  //  unsigned int data;
  unsigned int data[] = {0x0D22, 0x0D22, 0x0512, 0x0602, 0x0201, 0x0411, 0x0D21, 0x0411, 0x0910, 0x0300};
  // printf("Now sending %u...", data);
  for (int i = 0; i < 10; i++) {
    radio.stopListening();
    printf("Now sending %u...", data[i]);
    bool ok = radio.write( &data[i], sizeof(unsigned int) );

    if (ok)
      printf("ok...\n");
    else
      printf("failed.\n\r");

    // Now, continue listening
    radio.startListening();

    // Try again 1s later
    delay(1000);
  }
}
// vim:cin:ai:sts=2 sw=2 ft=cpp

