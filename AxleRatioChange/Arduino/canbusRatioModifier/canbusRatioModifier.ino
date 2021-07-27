

/*
 * canbus repeater/modifier using canbus modules and Arduino pro mini
 * 
 * Author: Chirpy
 * 
 * Repeats canbus messages between two buses
 * Intercepts ABS wheel speed messages and modifies them for changed axle ratio
 * Since most messages have 8 bytes of data, no shortcuts coded to check message length and expedite shorter message processing
 * Runs on Arduino Mini Pro with two canbus modules controlled via SPI
 */

/*
 * Note mini pro plus canbus modules use:
 * Module 1 - D2 for interrupts, D8 for CS (PORTB bit 0) (6 = 110)
 * Module 2 - D3 for interrupts, D9 for  CS (PORTB bit 1)(5 = 101)
 * Module 3 - A0 pin change for interrupts, D10 for CS (PORTB bit 2) (3 = 011)
 * Arduino has LED on pin 13 but not usable if using SPI, so can only be used after SPI is disabled
 * 
 * Module 1 - D2 int - INT0 - will be used to connect to TCM, Chip select is PORTB 0x06
 * Module 2 - D3 int - INT1 - will be used to connect to the ECU/ABS side of the bus, Chip select is PORTB 0x05
 * Module 3 not used in this application
 */

//definitions to allow fastest possible SPI transfers 

#define DELAY_CYCLES(n) __builtin_avr_delay_cycles(n)
 
 #include <SPI.h>
 #include <avr/wdt.h> //include watchdog library

ISR(INT0_vect) //interrupt service routine for D2 interrupt, which is TCM connection
{
  byte spiRx;
  byte canMessage[13];//

  //read canstat  register to see which buffer stored incoming data
  PORTB = 0x06; //CS
  SPDR = 0x03; // read
  DELAY_CYCLES(18); //send
  SPDR = 0x0e; //CANSTAT register
  DELAY_CYCLES(18); //send
  SPDR = 0x00; //read contents
  DELAY_CYCLES(18); //send
  PORTB = 0x07;

  spiRx = SPDR;

  //ICOD codes are bits 1-3, 110 is RXB0 interrupt, 111 is RXB1
  if ((spiRx & 0x0e) == 0x0c)  //0x0c indicates RXB0, 0x0e indicates RXB1
  {
    // grab data, re-transmit, reset flags, and return

    //read rx buffer 0 - message ID and length
    PORTB = 0x06; //CS
    SPDR = 0x90; // send read RX buffer 0 instruction, starting at SIDH
    DELAY_CYCLES(18); //send

    SPDR = 0x00; //initiate receive of first byte
    DELAY_CYCLES(18);  // pad out to 18 cycles

    //enter loop to read the 13 bytes of the message (maybe shorter, but read all anyway
     for (byte x = 0; x<12;x++) 
     {   
       canMessage[x] = SPDR; //store value
       SPDR = 0x00; //receive next byte
       DELAY_CYCLES(15);  // pad out to 18 cycles
     }

    //store the last byte
    DELAY_CYCLES(3); //delay to ensure SPI transfer completes
    canMessage[12] = SPDR; //receive RXB7
    PORTB = 0x07; //No CS
  }
  else
  {

    //ICOD codes are bits 1-3, 110 is RXB0 interrupt, 111 is RXB1
    if ((spiRx & 0x0e) == 0x0e)  //0x0e indicates RXB1
    //RXB0 has higher priority, so this will not execute if RXB0 triggered the interrupt, since the RXB0 flag needs to be cleared before the ICOD bits will update
    {
      // grab data, re-transmit, reset flags, and return

      //read rx buffer 0 - message ID and length
      PORTB = 0x06; //CS
      SPDR = 0x94; // send read RX buffer 1 instruction, starting at SIDH
      DELAY_CYCLES(18); //send

      SPDR = 0x00; //initiate receive of first byte
      DELAY_CYCLES(18);  // pad out to 18 cycles

      //enter loop to read the 13 bytes of the message (maybe shorter, but read all anyway
       for (byte x = 0; x<12;x++) 
       {   
         canMessage[x] = SPDR; //store value
         SPDR = 0x00; //receive next byte
         DELAY_CYCLES(15);  // pad out to 18 cycles
       }
      //store the last byte
      DELAY_CYCLES(3); //delay to ensure SPI transfer completes
      canMessage[12] = SPDR; //receive RXB7
      PORTB = 0x07; //No CS
    } 
  } 

  //modify target messages, if any


  //send data on ECU/ABS connection
  //send on TX0 if busy then TX1, if busy then TX2, else skip
  
  //read status of TXB 0
    PORTB = 0x05; 
    SPDR = 0x03; //read
    DELAY_CYCLES(18); //send
    SPDR = 0x30; //TXB0CTRL
    DELAY_CYCLES(18); //send
    SPDR = 0x00; //receive contents
    DELAY_CYCLES(18); //send
    PORTB = 0x07;

    spiRx = SPDR;

    if ((spiRx & 0x08) == 0)//check TX0 buffer is empty
    {
      //send data in TXB 0
      PORTB = 0x05;
      SPDR = 0x40; // load TX buffer 0 starting a SIDH
      DELAY_CYCLES(18); //send
      
      for (byte x = 0;x<13;x++) 
      {       
        SPDR = canMessage[x]; //load tx buffer with message
        DELAY_CYCLES(15);  // pad out to 18 cycles
      }

    DELAY_CYCLES(3); //complete send of last byte
    PORTB = 0x07; // No CS
    DELAY_CYCLES(2);

    //initiate message transmission by setting TXB0CTRL.TXREQ
    PORTB = 0x05; // No CS   
    SPDR = 0x81; // set TXB0CTRL.TXREQ
    DELAY_CYCLES(18); //send
    PORTB = 0x07; // No CS

    }
    else
    {
      //check status of TXB1
      PORTB = 0x05; 
      SPDR = 0x03; //read
      DELAY_CYCLES(18); //send
      SPDR = 0x40; //TXB1CTRL
      DELAY_CYCLES(18); //send
      SPDR = 0x00; //receive contents
      DELAY_CYCLES(18); //send
      PORTB = 0x07;
      
      spiRx = SPDR;

      if ((spiRx & 0x08) == 0)//check buffer is empty
      {
        //send data in TXB 1
        PORTB = 0x05;
        SPDR = 0x42; // load TX buffer 1 starting a SIDH
        DELAY_CYCLES(18); //send
      
      for (byte x =0;x<13;x++) 
      {       
        SPDR = canMessage[x]; //load tx buffer with message
        DELAY_CYCLES(15);  // pad out to 18 cycles
      }
      DELAY_CYCLES(3); //complete send of last byte
      PORTB = 0x07; // No CS
      DELAY_CYCLES(2);

      //initiate message transmission by setting TXB1CTRL.TXREQ
      PORTB = 0x05; // No CS   
      SPDR = 0x82; // write
      DELAY_CYCLES(18); //send
      
      PORTB = 0x07; // No CS

     }
      else
      {
        //check status of TXB2
        PORTB = 0x05; 
        SPDR = 0x03; //read
        DELAY_CYCLES(18); //send
        SPDR = 0x50; //TXB0CTRL
        DELAY_CYCLES(18); //send
        SPDR = 0x00; //receive contents
        DELAY_CYCLES(18); //send
        PORTB = 0x07;
        
        spiRx = SPDR;

        if ((spiRx & 0x08) == 0)
        {
          //send data in TXB 2
          PORTB = 0x05;
          SPDR = 0x44; // load TX buffer 2 starting a SIDH
          DELAY_CYCLES(18); //send
      
          for (byte x=0;x<13;x++) 
          {       
            SPDR = canMessage[x]; //load tx buffer with message
            DELAY_CYCLES(15);  // pad out to 18 cycles
          }

          DELAY_CYCLES(3); //complete send of last byte
          PORTB = 0x07; // No CS
          DELAY_CYCLES(2);

          //initiate message transmission by setting TXB2CTRL.TXREQ
          PORTB = 0x05; // No CS   
          SPDR = 0x84; // write
          DELAY_CYCLES(18); //send
        
          PORTB = 0x07; // No CS

        }
      }
    }
  }


ISR(INT1_vect) //interrupt service routine for D3 interrupt, which is ECU/ABS connection
{
  byte spiRx;
  byte canMessage[13];//
  boolean rx0Int = false;


  //read canstat  register to see which buffer stored incoming data
  PORTB = 0x05; //CS
  SPDR = 0x03; // read
  DELAY_CYCLES(18); //send
  SPDR = 0x0e; //CANSTAT register
  DELAY_CYCLES(18); //send
  SPDR = 0x00; //read contents
  DELAY_CYCLES(18); //send
  PORTB = 0x07;

  spiRx = SPDR;

  //ICOD codes are bits 1-3, 110 is RXB0 interrupt, 111 is RXB1
  if ((spiRx & 0x0e) == 0x0c)  //0x0c indicates RXB0, 0x0e indicates RXB1
  {
    rx0Int = true;
    // grab data, re-transmit, reset flags, and return

    //read rx buffer 0 - message ID and length
    PORTB = 0x05; //CS
    SPDR = 0x90; // send read RX buffer 0 instruction, starting at SIDH
    DELAY_CYCLES(18); //send

    SPDR = 0x00; //initiate receive of first byte
    DELAY_CYCLES(18);  // pad out to 18 cycles

    //enter loop to read the 13 bytes of the message (maybe shorter, but read all anyway
     for (byte x=0;x<12;x++) 
     {   
       canMessage[x] = SPDR; //store value
       SPDR = 0x00; //receive next byte
       DELAY_CYCLES(15);  // pad out to 18 cycles
     }
    //store the last byte
    DELAY_CYCLES(3); //delay to ensure SPI transfer completes
    canMessage[12] = SPDR; //receive RXB7
    PORTB = 0x07; //No CS
  }
  else
  {

    //ICOD codes are bits 1-3, 110 is RXB0 interrupt, 111 is RXB1
    if ((spiRx & 0x0e) == 0x0e)  //0x0e indicates RXB1
    //RXB0 has higher priority, so this will not execute if RXB0 triggered the interrupt, since the RXB0 flag needs to be cleared before the ICOD bits will update
    {
      // grab data, re-transmit, reset flags, and return

      //read rx buffer 0 - message ID and length
      PORTB = 0x05; //CS
      SPDR = 0x94; // send read RX buffer 1 instruction, starting at SIDH
      DELAY_CYCLES(18); //send

      SPDR = 0x00; //initiate receive of first byte
      DELAY_CYCLES(18);  // pad out to 18 cycles

      //enter loop to read the 13 bytes of the message (maybe shorter, but read all anyway
       for (byte x=0;x<12;x++)
       {   
         canMessage[x] = SPDR; //store value
         SPDR = 0x00; //receive next byte
         DELAY_CYCLES(15);  // pad out to 18 cycles
       }

      //store the last byte
      DELAY_CYCLES(3); //delay to ensure SPI transfer completes
      canMessage[12] = SPDR; //receive RXB7
      PORTB = 0x07; //No CS
    } 
  } 

  //modify target messages, if any
  if ((canMessage[0] == 0x96) && ((canMessage[1] & 0xe0) == 0x00)) // detect ABS message with SID 0x4b0
  {
    //3.06 TCM adjustment factors are
    // 3.06  32768 axle
    //3.27  35017
    //3.58  38336
    //3.77  40371

    //3.27 TCM adjustment factors are
    // 3.06  30664 axle
    //3.27  32768
    //3.58  35874
    //3.77  37778

    const unsigned int ratioAdj = 35017;
    
    //scale data in bytes 5/6, 7/8, 9/10, 11/12
    unsigned int msg = canMessage[5];
    msg = msg << 8;
    msg = msg + canMessage[6];
    
    unsigned long adjMsg = (unsigned long) msg * (unsigned long) ratioAdj;
    adjMsg = (adjMsg >> 15);
    if (adjMsg > 0xffff) //prevent overflow if adjusted speed is too high
    {
      adjMsg = 0xffff; // {future] ? setting a lower max ABS speed might also workaround speed limiter...?
    }
    canMessage[6] = adjMsg & 0xff;
    adjMsg = (adjMsg >> 8);
    canMessage[5] = adjMsg & 0xff;

    msg = canMessage[7];
    msg = msg << 8;
    msg = msg + canMessage[8];
    adjMsg = (unsigned long) msg * (unsigned long) ratioAdj;
    adjMsg = (adjMsg >> 15);
    if (adjMsg > 0xffff) //prevent overflow if adjusted speed is too high
    {
      adjMsg = 0xffff;
    }
    canMessage[8] = adjMsg & 0xff;
    adjMsg = (adjMsg >> 8);
    canMessage[7] = adjMsg & 0xff;

    msg = canMessage[9];
    msg = msg << 8;
    msg = msg + canMessage[10];
    adjMsg = (unsigned long) msg * (unsigned long) ratioAdj;
    adjMsg = (adjMsg >> 15);
    if (adjMsg > 0xffff) //prevent overflow if adjusted speed is too high
    {
      adjMsg = 0xffff;
    }
    canMessage[10] = adjMsg & 0xff;
    adjMsg = (adjMsg >> 8);
    canMessage[9] = adjMsg & 0xff;

    msg = canMessage[11];
    msg = msg << 8;
    msg = msg + canMessage[12];
    adjMsg = (unsigned long) msg * (unsigned long) ratioAdj;
    adjMsg = (adjMsg >> 15);
    if (adjMsg > 0xffff) //prevent overflow if adjusted speed is too high
    {
      adjMsg = 0xffff;
    }
    canMessage[12] = adjMsg & 0xff;
    adjMsg = (adjMsg >> 8);
    canMessage[11] = adjMsg & 0xff;

  }

  //send data on TCM connection
  //send on TX0 if busy then TX1, if busy then TX2, else skip
  
  //read status of TXB 0
    PORTB = 0x06; 
    SPDR = 0x03; //read
    DELAY_CYCLES(18); //send
    SPDR = 0x30; //TXB0CTRL
    DELAY_CYCLES(18); //send
    SPDR = 0x00; //receive contents
    DELAY_CYCLES(18); //send
    PORTB = 0x07;

    spiRx = SPDR;

    if ((spiRx & 0x08) == 0)//check TX0 buffer is empty
    {
      //send data in TXB 0
      PORTB = 0x06;
      SPDR = 0x40; // load TX buffer 0 starting a SIDH
      DELAY_CYCLES(18); //send
      
      for (byte x=0;x<13;x++) 
      {       
        SPDR = canMessage[x]; //load tx buffer with message
        DELAY_CYCLES(15);  // pad out to 18 cycles
      }

    DELAY_CYCLES(3); //complete send of last byte
    PORTB = 0x07; // No CS
    DELAY_CYCLES(2);

    //initiate message transmission by setting TXB0CTRL.TXREQ
    PORTB = 0x06; // No CS   
    SPDR = 0x81; // write
    DELAY_CYCLES(18); //send
      
    PORTB = 0x07; // No CS

    }
    else
    {
      //check status of TXB1
      PORTB = 0x06; 
      SPDR = 0x03; //read
      DELAY_CYCLES(18); //send
      SPDR = 0x40; //TXB1CTRL
      DELAY_CYCLES(18); //send
      SPDR = 0x00; //receive contents
      DELAY_CYCLES(18); //send
      PORTB = 0x07;
      
      spiRx = SPDR;

      if ((spiRx & 0x08) == 0)//check buffer is empty
      {
        //send data in TXB 1
        PORTB = 0x06;
        SPDR = 0x42; // load TX buffer 1 starting a SIDH
        DELAY_CYCLES(18); //send
      
      for (byte x=0;x<13;x++) 
      {       
        SPDR = canMessage[x]; //load tx buffer with message
        DELAY_CYCLES(15);  // pad out to 18 cycles
      };

      DELAY_CYCLES(3); //complete send of last byte
      PORTB = 0x07; // No CS
      DELAY_CYCLES(2);

      //initiate message transmission by setting TXB1CTRL.TXREQ
      PORTB = 0x06; // No CS   
      SPDR = 0x82; // write
      DELAY_CYCLES(18); //send
        
      PORTB = 0x07; // No CS

     }
      else
      {
        //check status of TXB2
        PORTB = 0x06; 
        SPDR = 0x03; //read
        DELAY_CYCLES(18); //send
        SPDR = 0x50; //TXB0CTRL
        DELAY_CYCLES(18); //send
        SPDR = 0x00; //receive contents
        DELAY_CYCLES(18); //send
        PORTB = 0x07;
        
        spiRx = SPDR;

        if ((spiRx & 0x08) == 0)
        {
          //send data in TXB 2
          PORTB = 0x06;
          SPDR = 0x44; // load TX buffer 2 starting a SIDH
          DELAY_CYCLES(18); //send
      
          for (byte x=0;x<13;x++) 
          {       
            SPDR = canMessage[x]; //load tx buffer with message
            DELAY_CYCLES(15);  // pad out to 18 cycles
          };

          DELAY_CYCLES(3); //complete send of last byte
          PORTB = 0x07; // No CS
          DELAY_CYCLES(2);

          //initiate message transmission by setting TXB2CTRL.TXREQ
          PORTB = 0x06; // No CS   
          SPDR = 0x84; // write
          DELAY_CYCLES(18); //send
   
          PORTB = 0x07; // No CS

        }
      }
    }        
  }




void setup() {
  
  wdt_disable();

  //setup pins and modes

  pinMode(8, OUTPUT);  //chip select for TCM facing module
  digitalWrite(8, LOW);

  pinMode(9, OUTPUT);  //chip select ECU facing module
  digitalWrite(9, HIGH);

  pinMode(10, OUTPUT);  //chip select third module
  digitalWrite(10, HIGH);

  pinMode(11, OUTPUT);  //MOSI for SPI
  pinMode(13, OUTPUT); //SCK for SPI

  //configure arduino SPI interface
  SPCR = 0x50; //SPI no interrupt, mode 0, MSB first, 8 MHz clock
  SPSR = 0x01; //SPI interface 8 MHz clock setting SPI2X bit 0
  // SPDR is the SPI data register for byte input/output
  //SPSR bit 7 is the SPI flag which is set for completion of data read/write (collision flag is bit 6)

  DELAY_CYCLES(2);

  //initiaitize first canbus module

   //reset MCP2515 via SPI
  PORTB = 0x06; //write D8 low to chip select MCP2515
  SPDR = 0xc0; //reset MCP2515
  DELAY_CYCLES(18); //receive
  PORTB = 0x07; //raise chip select after SPI transfers
  
  DELAY_CYCLES(16);
  
  //configure MCP2515
  
  //configure can bit rate to 500 kbps, and receive interrupts on buffer 0
  PORTB = 0x06; //CS
  SPDR = 0x02; // send write register instruction
  DELAY_CYCLES(18); //send
  SPDR = 0x28;// start at CNF3 register
  DELAY_CYCLES(18); //send
  SPDR = 0x02; //CNF3 PHSEG2 = 2, which is 3 Time Quanta (TQ). Note SyncSeg defaults to 1 TQ
   //note usually use 0x02, but can use 0x82 to enable SOF signal output for debugging
  DELAY_CYCLES(18); //send
  SPDR = 0x90; //CNF2 PHSEG1 = 2, which is 3 Time Quanta (TQ), and PSEG = 0 which is 1 TQ.
  DELAY_CYCLES(18); //send
  SPDR = 0x00; //CNF1 Baud Rate Prescaler = 0, = 500 kHz with 8 MHz clock
  DELAY_CYCLES(18); //send
  PORTB = 0x07; // No CS

  DELAY_CYCLES(2);

  //turn off receive filters and masks
  PORTB = 0x06; //CS
  SPDR = 0x02; // write
  DELAY_CYCLES(18); //send
  SPDR = 0x60;// RXB0CTRL register
  DELAY_CYCLES(18); //send
  SPDR = 0x64; // turn masks and filters off, enable BUKT
  DELAY_CYCLES(18); //send
  PORTB = 0x07; // No CS
  DELAY_CYCLES(2); //send

  //clear error flags and interrupt flags
  
  PORTB = 0x06; //CS
  SPDR = 0x02; // write
  DELAY_CYCLES(18); //send
  SPDR = 0x2c;// CANINTF register
  DELAY_CYCLES(18); //send
  SPDR = 0x00; // 0x00 to CANINTF
  DELAY_CYCLES(18); //send
  SPDR = 0x00; // 0x00 to EFLG
  DELAY_CYCLES(18); //send
  PORTB = 0x07; // No CS
  DELAY_CYCLES(2); //send

 //set normal  mode  CANCTRL = 00000000 = 0x00
  PORTB = 0x06; //CS
  SPDR = 0x02; // write spi
  DELAY_CYCLES(18); //send
  SPDR = 0x0f; // CANCTRL register
  DELAY_CYCLES(18); //send
  SPDR = 0x00; // set nomal mode
   //note usually use 0x00, but can use 0x04 to enable SOF signal output for debugging
  DELAY_CYCLES(20); //send
  PORTB = 0x07; // No CS
  DELAY_CYCLES(2); //


    //initiaitize second canbus module

   //reset MCP2515 via SPI
  PORTB = 0x05; //write D9 low to chip select MCP2515
  SPDR = 0xc0; //reset MCP2515
  DELAY_CYCLES(18); //receive
  PORTB = 0x07; //raise chip select after SPI transfers
  DELAY_CYCLES(2);

  
  //configure MCP2515
  
  //configure can bit rate to 500 kbps, and receive interrupts on buffer 0
  PORTB = 0x05; //CS
  SPDR = 0x02; // send write register instruction
  DELAY_CYCLES(18); //send
  SPDR = 0x28;// start at CNF3 register
  DELAY_CYCLES(18); //send
  SPDR = 0x02; //CNF3 PHSEG2 = 2, which is 3 Time Quanta (TQ). Note SyncSeg defaults to 1 TQ
   //note usually use 0x02, but can use 0x82 to enable SOF signal output for debugging
  DELAY_CYCLES(18); //send
  SPDR = 0x90; //CNF2 PHSEG1 = 2, which is 3 Time Quanta (TQ), and PSEG = 0 which is 1 TQ.
  DELAY_CYCLES(18); //send
  SPDR = 0x00; //CNF1 Baud Rate Prescaler = 0, = 500 kHz with 8 MHz clock
  DELAY_CYCLES(18); //send
  PORTB = 0x07; // No CS

  DELAY_CYCLES(2);

  //turn off receive filters and masks
  PORTB = 0x05; //CS
  SPDR = 0x02; // write
  DELAY_CYCLES(18); //send
  SPDR = 0x60;// RXB0CTRL register
  DELAY_CYCLES(18); //send
  SPDR = 0x64; // turn masks and filters off, enable BUKT
  DELAY_CYCLES(18); //send
  PORTB = 0x07; // No CS
  DELAY_CYCLES(2); //send

  //clear error flags and interrupt flags
  
  PORTB = 0x05; //CS
  SPDR = 0x02; // write
  DELAY_CYCLES(18); //send
  SPDR = 0x2c;// CANINTF register
  DELAY_CYCLES(18); //send
  SPDR = 0x00; // 0x00 to CANINTF
  DELAY_CYCLES(18); //send
  SPDR = 0x00; // 0x00 to EFLG
  DELAY_CYCLES(18); //send
  PORTB = 0x07; // No CS
  DELAY_CYCLES(2); //send

 //set normal  mode  CANCTRL = 00000000 = 0x00
  PORTB = 0x05; //CS
  SPDR = 0x02; // write spi
  DELAY_CYCLES(18); //send
  SPDR = 0x0f; // CANCTRL register
  DELAY_CYCLES(18); //send
  SPDR = 0x00; // set nomal mode
   //note usually use 0x00, but can use 0x04 to enable SOF signal output for debugging
  DELAY_CYCLES(20); //send
  PORTB = 0x07; // No CS
  DELAY_CYCLES(2); //

  noInterrupts();
  
  //configure Arduino for interrupts
  EICRA = 0x00; //set INT0 and INT1 to trigger on low
  EIMSK = 0x03; //enable interrupts on D2 and D3 (= INT0, INT1)

  //enable receive interrupts on first MCP2515
  PORTB = 0x06; //CS
  SPDR = 0x02; // write spi
  DELAY_CYCLES(18); //send
  SPDR = 0x2b; // write CANINTE
  DELAY_CYCLES(18); //send
  SPDR = 0x03; //CANINTE enable receive interrupts on receive buffers 0 and 1
  DELAY_CYCLES(20); //send
  PORTB = 0x07; // No CS
  DELAY_CYCLES(2); //

  //enable receive interrupts on second MCP2515
  PORTB = 0x05; //CS
  SPDR = 0x02; // write spi
  DELAY_CYCLES(18); //send
  SPDR = 0x2b; // write CANINTE
  DELAY_CYCLES(18); //send
  SPDR = 0x03; //CANINTE enable receive interrupts on receive buffers 0 and 1
  DELAY_CYCLES(20); //send
  PORTB = 0x07; // No CS
  DELAY_CYCLES(2); //

  wdt_enable(WDTO_30MS); //30 ms timeout on watchdog

  interrupts();

}

void loop() {
  // put your main code here, to run repeatedly:

 wdt_reset(); //reset watchdog

}
