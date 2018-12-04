/*
  Copyright (C) 2011 J. Coliz <maniacbug@ymail.com>

  This program is free software; you can redistribute it and/or
  modify it under the terms of the GNU General Public License
  version 2 as published by the Free Software Foundation.
*/

/**
   Example for Getting Started with nRF24L01+ radios.

   This is an example of how to use the RF24 class.  Write this sketch to two
   different nodes.  Put one of the nodes into 'transmit' mode by connecting
   with the serial monitor and sending a 'T'.  The ping node sends the current
   time to the pong node, which responds by sending the value back.  The ping
   node can then see how long the whole cycle took.
*/

#include <SPI.h>
#include "nRF24L01.h"
#include "RF24.h"
#include "printf.h"

//
// Hardware configuration
//

// Set up nRF24L01 radio on SPI bus plus pins 9 & 10

RF24 radio(9, 10);

//
// Topology
//

// Radio pipe addresses for the 2 nodes to communicate.
const uint64_t pipes[2] = { 0x000000002ALL, 0x000000002BLL };



void setup(void)
{

  Serial.begin(9600);
  printf_begin();

  //
  // Setup and configure rf radio
  //

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



  radio.openWritingPipe(pipes[1]);
  radio.openReadingPipe(1, pipes[0]);


  //
  // Start listening
  //

  radio.startListening();

  //
  // Dump the configuration of the rf unit for debugging
  //

  //radio.printDetails();
}

void loop(void)
{



  // if there is data ready
  if ( radio.available() )
  {
    // Dump the payloads until we've gotten everything
    unsigned int data, x, y, WallWest, WallEast, WallNorth, WallSouth, OppRobot, Treasure;
    bool done = false;
    while (!done)
    {
      // Fetch the payload, and see if this was the last one.
      done = radio.read( &data, sizeof(unsigned int) );

      // Spew it
      //printf("data = %u...",data);


      x = (data >> 4) & 0x000F;
      Serial.print(x);


      y = data & 0x000F;
      Serial.print(",");
      Serial.print(y);



      WallWest = (data >> 8) & 0x0001;
      if (WallWest) {
        Serial.print(",");
        Serial.print("west=true");
      }

      WallNorth = (data >> 9) & 0x0001;
      if (WallNorth) {
        Serial.print(",");
        Serial.print("north=true");
      }

      WallEast = (data >> 10) & 0x0001;
      if (WallEast) {
        Serial.print(",");
        Serial.print("east=true");
      }

      WallSouth = (data >> 11) & 0x0001;
      if (WallSouth) {
        Serial.print(",");
        Serial.print("south=true");
      }



      OppRobot = (data >> 12) & 0x0008;
      if (OppRobot) {
        Serial.print(",");
        Serial.print("robot=true");
      }

      Treasure = (data >> 12) & 0x0007;
      /*
        No Treasure = 0;
        Blue Triangle = 1;
        Blue Square = 2;
        Blue Diamond = 3;
        Red Triangle = 4;
        Red Square = 5;
        Red Diamond = 6;

      */
      switch (Treasure) {
        case 1:
          Serial.print(",");
          Serial.print("tcolor=blue");
          Serial.print(",");
          Serial.print("tshape=triangle");

        case 2:
          Serial.print(",");
          Serial.print("tcolor=blue");
          Serial.print(",");
          Serial.print("tshape=square");

        case 3:
          Serial.print(",");
          Serial.print("tcolor=blue");
          Serial.print(",");
          Serial.print("tshape=diamond");

        case 4:
          Serial.print(",");
          Serial.print("tcolor=red");
          Serial.print(",");
          Serial.print("tshape=triangle");

        case 5:
          Serial.print(",");
          Serial.print("tcolor=red");
          Serial.print(",");
          Serial.print("tshape=square");

        case 6:
          Serial.print(",");
          Serial.print("tcolor=red");
          Serial.print(",");
          Serial.print("tshape=diamond");

      }





      Serial.println();

      // Delay just a little bit to let the other unit
      // make the transition to receiver
      delay(20);

    }

    /* // First, stop listening so we can talk
      radio.stopListening();

      // Send the final one back.
      radio.write( &data, sizeof(unsigned int) );
      printf("Sent response.\n\r");

      // Now, resume listening so we catch the next packets.
      radio.startListening(); */
  }


  //
  // Change roles
  //


}
// vim:cin:ai:sts=2 sw=2 ft=cpp
