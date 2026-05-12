/****************************************************/
/* This is only one example of code structure       */
/* OFFCOURSE this code can be optimized, but        */
/* the idea is let it so simple to be easy catch    */
/* where can do changes and look to the results     */
/* The code is not clean and not well ordered this  */
/* is exagerated in comments to help on Debug       */
/****************************************************/

/*This panel use the micro-controller HOLTEK, ref: HT48R05A  *
* and the driver of VFD from PCT, ref: PT6312                *
*/
#define VFD_in 8// If 0 write LCD, if 1 read of LCD
#define VFD_clk 9 // Must be pulsed to LCD fetch data of bus
#define VFD_stb 10 // if 0 is a command, if 1 is a data0

#define CTL 5 // Pin comming from panel named as CTL
#define A5STB 11 //Pin to return the panel to standby mode providing a pulse of High.

//#define AdjustPins  PIND // before is C, but I'm use port C to VFC Controle signals

#define BUTTON_PIN2 2 //Att check wich pins accept interrupts... Uno is 2 & 3
#define BUTTON_PIN3 3 //Att check wich pins accept interrupts... Uno is 2 & 3

volatile byte buttonReleased2 = false;
volatile byte buttonReleased3 = false;

uint8_t a=0x33;   //0x33
uint8_t b=0x01;   //0x01
uint8_t numberGrids = 0x00;  //Here we define the number of grids used as present on the below table of 6312 datasheet:
/*Display mode settings: 
 000: 4 digits, 16 segments 
 001: 5 digits, 16 segments 
 010: 6 digits, 16 segments 
 011: 7 digits, 15 segments 
 100: 8 digits, 14 segments 
 101: 9 digits, 13 segments 
 110: 10 digits, 12 segments 
 111: 11 digits, 11 segments
*/
unsigned char DigitTo7SegEncoder(unsigned char digit, unsigned char common);
/*Global Variables Declarations*/
unsigned char day = 7;  
unsigned char hours = 0;
unsigned char minutes = 0;
unsigned char seconds = 0;
unsigned char milisec = 0;
unsigned char points = 0;
unsigned char secs;
unsigned char digit;
unsigned char number;
unsigned char numberA;
unsigned char numberB;
unsigned char numberC;
unsigned char numberD;
unsigned char numberE;
unsigned char numberF;
unsigned char grid;
unsigned char wordA = 0;
unsigned char wordB = 0;
unsigned int k=0;

unsigned int segments[] ={
  // Here I'm forced to use the "0" as 10, because the 7 segments sart in "1"
  // This table is inverted
  // This not respect the table for 7segm like "abcdefgh"  // 
     0b11101110, //0 
     0b01001000, //1 
     0b11010110, //2 
     0b11011010, //3  
     0b01111000, //4 
     0b10111010, //5 
     0b10111110, //6  
     0b11001000, //7 
     0b11111110, //8 
     0b11111010, //9 
     0b00000000, //10 // empty display
  };
unsigned long grids[] ={
  //font data
  //  The grid on this display count from left to right  // 
    0b00010000, //  Grid 1
    0b00001000, //  Grid 2
    0b00000100, //  Grid 3
    0b00000010, //  Grid 4
    0b00000001, //  Grid 5
  };
void pt6312_init(void){
  delayMicroseconds(200); //power_up delay
  // Note: Allways the first byte in the input data after the STB go to LOW is interpret as command!!!
  
  // Configure VFD display (numberGrids)
  cmd_with_stb(numberGrids);//  (0b01000000)    cmd1 4 grids 16 segm
  delayMicroseconds(5);

  // set DIMM/PWM to value and switch ON the display.
  cmd_with_stb((0b10001000) | 7);//0 min - 7 max  )(0b01010000)
  delayMicroseconds(5);

  // Write to memory display, increment address, normal operation
  cmd_with_stb(0b01000000);//(BIN(01000000));
  delayMicroseconds(1);
  // Address 00H - 15H ( total of 11*2Bytes=176 Bits)
  cmd_with_stb(0b11000000);//(BIN(01100110)); 
  delayMicroseconds(1);
}
void cmd_without_stb(unsigned char a){
  // send without stb
  unsigned char data = 170; //value to transmit, binary 10101010
  unsigned char mask = 1; //our bitmask
  
  data=a;
  //This don't send the strobe signal, to be used in burst data send
          for (mask = 00000001; mask>0; mask <<= 1) { //iterate through bit mask
            digitalWrite(VFD_clk, LOW);
            if (data & mask){ // if bitwise AND resolves to true
                digitalWrite(VFD_in, HIGH);
            }
            else{ //if bitwise and resolves to false
              digitalWrite(VFD_in, LOW);
            }
    delayMicroseconds(1);
    digitalWrite(VFD_clk, HIGH);
    delayMicroseconds(5);
   }
   //digitalWrite(VFD_clk, LOW);
}
void cmd_with_stb(unsigned char a){
  // send with stb
  unsigned char data = 170; //value to transmit, binary 10101010
  unsigned char mask = 1; //our bitmask
  
  data=a;
  
  //This send the strobe signal
  //Note: The first byte input at in after the STB go LOW is interpreted as a command!!!
  digitalWrite(VFD_stb, LOW);
  delayMicroseconds(1);
          for (mask = 0b00000001; mask>0; mask <<= 1) { //iterate through bit mask
            digitalWrite(VFD_clk, LOW);
            delayMicroseconds(1);
            if (data & mask){ // if bitwise AND resolves to true
                digitalWrite(VFD_in, HIGH);
            }
            else{ //if bitwise and resolves to false
              digitalWrite(VFD_in, LOW);
            }
            digitalWrite(VFD_clk, HIGH);
            delayMicroseconds(1);
          }
      delayMicroseconds(1);      
      digitalWrite(VFD_stb, HIGH);
      delayMicroseconds(1);
}
void test_VFD(void){
  /* 
  Here do a test for all segments of 5 grids
  each grid is controlled by a group of 2 bytes
  by these reason I'm send a burst of 2 bytes of
  data.
  */
  // to test 6 grids is 4*2=8, the 8 gird result in 8*2=16 bytes.
      cmd_with_stb(numberGrids); // cmd 1 // 5 Grids & 16 Segments
      cmd_with_stb(0b01000000); // cmd 2 //Normal operation; Set pulse as 1/16
      
        digitalWrite(VFD_stb, LOW);
        delayMicroseconds(1);
        cmd_without_stb((0b11000000)); //cmd 3 wich define the start address (00H to 15H)
        
            for (int i = 0; i < 5 ; i++){ // test base to 16 segm and 4grids, if write more not relevant, they will not showed.
            cmd_without_stb(0b11111111); // Data to fill table 4*16 
            cmd_without_stb(0b11111111); // Data to fill table 4*16 
            }
      digitalWrite(VFD_stb, HIGH);
      cmd_with_stb((0b10001000) | 7); //cmd 4
      delay(1);
      //invertLED();
}
void findSegments (void){
  uint8_t m = 0x00;
  uint8_t grid = 0x00;
  //       
  for(uint8_t g = 0x00; g < 0x0C; g = g+2){
    m=0x00;
    grid = g;
                for (int s = 0x00; s < 16; s++){
                    // This cycle while is used to controle button advance segments test!
                      while(1){
                            if(!buttonReleased2){
                              delay(200);
                            }
                            else{
                              delay(15);
                               buttonReleased2 = false;
                               break;
                            }
                      }
                  cmd_with_stb(numberGrids); // cmd 1 // 6 Grids & 16 Segments
                  cmd_with_stb(0b01000000); // cmd 2 //Normal operation; Set pulse as 1/16
                  
                  digitalWrite(VFD_stb, LOW);
                  delayMicroseconds(1);
                  cmd_without_stb((0b11000000) | g); //cmd 3 wich define the start address (00H to 15H)
                  //
                      if(s < 8){
                        cmd_without_stb(0b00000001 << s); // Data to fill table 5*16 = 80 bits
                        cmd_without_stb(0b00000000); // Data to fill table 5*16 = 80 bits
                         Serial.print("s: "); Serial.println(s, HEX);
                      }
                      else{
                        cmd_without_stb(0b00000000); // Data to fill table 5*16 = 80 bits
                        cmd_without_stb(0b00000001 << m); // Data to fill table 5*16 = 80 bits
                        m++;
                          Serial.print("m: "); Serial.println(m, HEX);
                      }
                  digitalWrite(VFD_stb, HIGH);
                  delayMicroseconds(5);
                  //
                  Serial.print("grid:  ");
                  Serial.print(grid, DEC);
                  //
                  Serial.print(";   Segment:  ");
                  Serial.println(s, DEC);
                  delay(80);
                }
          //  invertLED();     
          //  clear_VFD();     
  }           
}
void clear_VFD(void){
  /*
  Here I clean all registers 
  Could be done only on the number of grid
  to be more fast. The 12 * 2 bytes = 24 registers
  */
      for (uint8_t n=0x00; n < 0x08; n = n+2){  // important be 10, if not, bright the half of wells./this on the VFD of 4 grids)
        cmd_with_stb(numberGrids);//       cmd 1 // 4 Grids & 16 Segments
        cmd_with_stb(0b01000000); //       cmd 2 //Normal operation; Set pulse as 1/16
        digitalWrite(VFD_stb, LOW);
        delayMicroseconds(1);
            cmd_without_stb((0b11000000) | n); // cmd 3 //wich define the start address (00H to 15H)
            cmd_without_stb(0b00000000); // Data to fill table of 4 grids * 16 segm 
            cmd_without_stb(0b00000000); // Data to fill table of 4 grids * 16 segm 
            //
            digitalWrite(VFD_stb, HIGH);
            delayMicroseconds(5);
            cmd_with_stb((0b10001000) | 7); //cmd 4
            digitalWrite(VFD_stb, HIGH);
            delayMicroseconds(5);
     }
}
void ctrlLEDon(uint8_t portLED){
        cmd_with_stb(numberGrids); // cmd 1 // 5 Grids & 16 Segments
        digitalWrite(VFD_stb, LOW);
        delayMicroseconds(1);
        cmd_without_stb(0b01000001); // cmd 2 //write to LED ports
      
        cmd_without_stb(~(portLED)); // Data to fill table 5*16 = 80 bits
     
      digitalWrite(VFD_stb, HIGH);
      delayMicroseconds(4);
}
void ctrlLEDoff(void){
        cmd_with_stb(0b00000010); // cmd 1 // 5 Grids & 16 Segments
        digitalWrite(VFD_stb, LOW);
        delayMicroseconds(1);
        cmd_without_stb(0b01000001); // cmd 2 //write to LED ports
      
        cmd_without_stb(0b00000000); // Data to fill table 5*16 = 80 bits
     
      digitalWrite(VFD_stb, HIGH);
      delayMicroseconds(4);
}
void writeLED(){
      for(unsigned int led=0; led < 4; led++){
                  cmd_with_stb(numberGrids); // cmd 1 // 6 Grids & 16 Segments
                  digitalWrite(VFD_stb, LOW);
                  delayMicroseconds(1);
                  cmd_without_stb(0b01000001); //Write to LED ports (16312 have 4 LED's vailable at pins 
                        switch (led){
                          case 0: cmd_without_stb(0b00001110); break; //LED's 0,1,2,3 is reverse mode 0=On 1=Off
                          case 1: cmd_without_stb(0b00001101); break;
                          case 2: cmd_without_stb(0b00001011); break;
                          case 3: cmd_without_stb(0b00000111); break;
                        }
                  //
                  digitalWrite(VFD_stb, HIGH);
                  delayMicroseconds(4);
                  delay(750);
      }
}
void segBit(void){
  /*
  Here I clean all registers 
  Could be done only on the number of grid
  to be more fast. The 12 * 2 bytes = 24 registers
  */
      
        cmd_with_stb(numberGrids); //       cmd 1 // 4 Grids & 16 Segments
        cmd_with_stb(0b01000000); //       cmd 2 //Normal operation; Set pulse as 1/16
        digitalWrite(VFD_stb, LOW);
        delayMicroseconds(5);
            cmd_without_stb(0b11000000); // cmd 3 //wich define the start address (00H to 15H)

            cmd_without_stb(0b00000000); // Data to fill table of 0 grid * 16 segm 
            cmd_without_stb(0b00000000); // Data to fill table of 0 grid * 16 segm 

            cmd_without_stb(0b00000000); // Data to fill table of 1 grid * 16 segm 
            cmd_without_stb(0b00000000); // Data to fill table of 1 grid * 16 segm 

            cmd_without_stb(0b00000000); // Data to fill table of 2 grid * 16 segm 
            cmd_without_stb(0b00000000); // Data to fill table of 2 grid * 16 segm 

            cmd_without_stb(0b00000000); // Data to fill table of 3 grid * 16 segm 
            cmd_without_stb(0b00000000); // Data to fill table of 3 grid * 16 segm 

            digitalWrite(VFD_stb, HIGH);
            delayMicroseconds(5);
            cmd_with_stb((0b10001000) | 7); //cmd 4
            delayMicroseconds(5);  
}
void send7segm(){
  // This block is very important, it solve the difference 
  // between segments from grid 1 and grid 2(start 8 or 9)
      cmd_with_stb(numberGrids);// cmd 1 // 4 Grids & 16 Segments
      cmd_with_stb(0b01000000); // cmd 2 //Normal operation; Set pulse as 1/16
      
        digitalWrite(VFD_stb, LOW);
        delayMicroseconds(1);
        cmd_without_stb((0b11000000)); //cmd 3 wich define the start address (00H to 15H)
        
          cmd_without_stb(segments[k]); // 
          cmd_without_stb(segments[k]); // 
          cmd_without_stb(segments[k]); //  
          cmd_without_stb(segments[k]); // 
          cmd_without_stb(segments[k]); //
          cmd_without_stb(segments[k]); //
          cmd_without_stb(segments[k]); //
          cmd_without_stb(segments[k]); //

      digitalWrite(VFD_stb, HIGH);
      cmd_with_stb((0b10001000) | 7); //cmd 4
      delay(1);
      delay(1000);  
}
void PT6312BLQ_RunWheels(){
  uint8_t word = 0x00; //To be used as temp manipulated char and sended!
  int j, n;
  char x;
  short v = 0b0000001000000000;  // The short have a size of 16 bits(2 bytes)
        for (n=1; n < 2; n++){  //Note: only want write the position 0 & 1 of memory map (4 grids X 2 bytes)
          //clear_VFD();
            for(j = 1; j < 7; j++) {  // execute cycle and use the bit shift to control the bit of second byte belongs to whell
              //cmd1 Configure VFD display (numberGrids) 
              cmd_with_stb(numberGrids);//  4 grids
              delay(1);  // 
              
              //cmd2 Write to memory display, increment address, normal operation 
              cmd_with_stb(0b01000000);//Teste mode setting to normal, Address increment Fixed, Write data to display memory...
              
              digitalWrite(VFD_stb, LOW);
              delay(1);
              //cmd3 Address 00H - 15H ( total of 11*2Bytes=176 Bits)
              cmd_without_stb((0b11000000) | n);//Increment active, then test all segments!
              delay(1); 
              //Decide if write byte Low or byte High
                      if (n < 9 ){
                          //Serial.println(n, DEC); // Only to debug, comment it please!
                          ((x=v >> 8) & 0x00FF);
                          word = ((x << j) & 0xFF);
                          cmd_without_stb(word | 0x02);
                          //Serial.println(word, BIN); //Only to debug, comment it please!
                          //cmd_without_stb(0b00000001 << j);
                        }
                        // else if ((n > 8 ) and (j < 8)){
                        //   //Serial.println(n, DEC); // Only to debug
                        //   //((x=v << 8) & 0xFF00);
                        //   x=0b00000001;
                        //   cmd_without_stb((x << j) & 0xFF); // 
                        //   //cmd_without_stb(0b00000001 << j);
                        // } 
              delay(1);
              digitalWrite(VFD_stb, HIGH);
              //cmd4 set DIMM/PWM to value
              cmd_with_stb((0b10001000) | 7);//0 min - 7 max  )(0b01010000)//0 min - 7 max  )(0b01010000)
              delay(80);
            }
        }
}
/******************************************************************/
/************************** Update Clock **************************/
/******************************************************************/
void send_update_clock(void){
  //This function is responsable to do the update of clock
  //and make assign the digits numbers of sec units, dozen...
  if (secs >=60){
    secs =0;
    minutes++;
  }
  if (minutes >=60){
    minutes =0;
    hours++;
  }
  if (hours >=24){
    hours =0;
  }
    //*************************************************************
    DigitTo7SegEncoder(secs%10);
    //Serial.println(secs, DEC);
    numberA=number;
    DigitTo7SegEncoder(secs/10);
    //Serial.println(secs, DEC);
    numberB=number;
    SegTo32Bits();
    //*************************************************************
    DigitTo7SegEncoder(minutes%10);
    numberC=segments[number];
    DigitTo7SegEncoder(minutes/10);
    numberD=segments[number];
    SegTo32Bits();
    //**************************************************************
    DigitTo7SegEncoder(hours%10);
    numberE=segments[number];
    DigitTo7SegEncoder(hours/10);
    numberF=segments[number];
    SegTo32Bits();
    //**************************************************************
}
void SegTo32Bits(){
  //Serial.println(number,HEX);
  digitalWrite(VFD_stb, LOW);
  delayMicroseconds(10);
      cmd_with_stb(numberGrids);  // cmd 1 // 4 Grids & 16 Segments
      cmd_with_stb(0b01000000);   // cmd 2 // Normal operation; Set pulse as 1/16
      
        digitalWrite(VFD_stb, LOW);
        delayMicroseconds(10);
        cmd_without_stb((0b11000000) | grid); //cmd 3 wich define the start address (00H to 15H)
        wordB=segments[numberB];
        wordA=segments[numberA];
              
          cmd_without_stb(numberF); //Address 0x00, 0º grid at this panel belongs to wheel (Bit 0 to bit 7)
          cmd_without_stb(0x02);    //Address 0x01, 0º grid //The value of 0x20 is to active the bit equivalent to 9º bit of circle on center wheel
          
          cmd_without_stb(numberE); //Address 0x02, 1º grid  Note: The distribution of grids 0 and 1 support the digits belongs to the same group!!!
          cmd_without_stb(0x00);    //Address 0x03, 1º grid This is symbols predefineds, symbos of micro, speak, bar, etc...

          cmd_without_stb(numberC); //Address 0x04, 2º grid
          cmd_without_stb(numberD); //Address 0x05, 2º grid

          cmd_without_stb(wordA); //Address 0x06, 3º grid// seconds unit
          cmd_without_stb(wordB); //Address 0x07, 3º grid// seconds dozens  // Only this grid got a shift because the second  //  dozens
           
      digitalWrite(VFD_stb, HIGH);
      delayMicroseconds(10);
      cmd_with_stb((0b10001000) | 7); //cmd 4
      delay(5);
       
}
void DigitTo7SegEncoder( unsigned char digit){
      switch(digit)
      {
        case 0:   number=0;     break;  // if remove the LongX, need put here the segments[x]
        case 1:   number=1;     break;
        case 2:   number=2;     break;
        case 3:   number=3;     break;
        case 4:   number=4;     break;
        case 5:   number=5;     break;
        case 6:   number=6;     break;
        case 7:   number=7;     break;
        case 8:   number=8;     break;
        case 9:   number=9;     break;
      }
} 
void buttonsAdjustHMS(){
 // Important is necessary put a pull-up resistor to the VCC(+5VDC) to this pins (3, 4, 5)
 // if dont want adjust of the time comment the call of function on the loop
  /* Reset Seconds to 00 Pin number 3 Switch to GND*/
    // if((AdjustPins & 0x08) == 0 )
    // {
    //   _delay_ms(200);
    //   secs=00;
    // }
    
    // /* Set Minutes when SegCntrl Pin 4 Switch is Pressed*/
    // if((AdjustPins & 0x10) == 0 )
    // {
    //   _delay_ms(200);
    //   if(minutes < 59)
    //   minutes++;
    //   else
    //   minutes = 0;
    // }
    // /* Set Hours when SegCntrl Pin 5 Switch is Pressed*/
    // if((AdjustPins & 0x20) == 0 )
    // {
    //   _delay_ms(200);
    //   if(hours < 23)
    //   hours++;
    //   else
    //   hours = 0;
    // }
}
void readButtons(){
    //Take special attention to the initialize digital pin LED_BUILTIN as an output.
    int val = 0;       // variable to store the read value
    int dataIn=0;
    byte array[8] = {0,0,0,0,0,0,0,0};
    byte together = 0;
    unsigned char data = 0; //value to transmit, binary 10101010
    unsigned char mask = 1; //our bitmask
    array[0] = 0;
    unsigned char btn1 = 0x41;
          digitalWrite(VFD_stb, LOW);
          delayMicroseconds(2);
          cmd_without_stb(0b01000010); // cmd 2 //Read Keys;Normal operation; Set pulse as 1/16
          // cmd_without_stb((0b11000000)); //cmd 3 wich define the start address (00H to 15H)
          // send without stb
      
      pinMode(VFD_in, INPUT_PULLUP);  // Important this point! Here I'm changing the direction of the pin to INPUT data.
      delayMicroseconds(2);
      //PORTD != B01010100; // this will set only the pins you want and leave the rest alone at
      //their current value (0 or 1), be careful setting an input pin though as you may turn 
      //on or off the pull up resistor  
      //This don't send the strobe signal, to be used in burst data send
            for (int z = 0; z < 3; z++){
                //for (mask=0B00000001; mask > 0; mask <<= 1) { //iterate through bit mask
                      for (int h =7; h >= 0; h--) {
                          digitalWrite(VFD_clk, HIGH);  // Remember wich the read data happen when the clk go from LOW to HIGH! Reverse from write data to out.
                          delayMicroseconds(2);
                        val = digitalRead(VFD_in);
                          //digitalWrite(ledPin, val);    // sets the LED to the button's value
                              if (val & mask){ // if bitwise AND resolves to true
                                //Serial.print(val);
                                //data =data | (1 << mask);
                                array[h] = 1;
                              }
                              else{ //if bitwise and resolves to false
                              //Serial.print(val);
                              // data = data | (1 << mask);
                              array[h] = 0;
                              }
                        digitalWrite(VFD_clk, LOW);
                        delayMicroseconds(2);                        
                      } 
                
                  Serial.print(z);
                  Serial.print(" - " );
                            
                                      for (int bits = 7 ; bits >= 0; bits--) {
                                          Serial.print(array[bits]);
                                      }
        //On this area you can modify the function assigned to each key!
        //Value of "z" tell us the number of byte: (0, 1, 2), after the
        //second digit is positon of bit inside of byte like this: 
        //"array[0] == 1", byte 0, position 0.
                            if (z==0){
                              if(array[0] == 1){
                              hours++;
                              }
                            }         
                            if (z==0){
                              if(array[4] == 1){
                              hours--;
                              }
                            }  
                            if (z==1){
                              if(array[6] == 1){
                              
                              }
                            }
                              
                            if (z==1){
                              if(array[3] == 1){
                              minutes++;
                              }
                            }
                            if (z==1){
                              if(array[7] == 1){
                              minutes--;
                              }
                            }
                            if (z==0){
                              if(array[3] == 1){
                              secs++;
                              }
                            }
                            if (z==0){
                              if(array[3] == 1){
                                  hours = 0;
                                  minutes = 0;
                                secs=0;  // Set count of secs to zero to be more easy to adjust with other clocl.
                                }
                            }
                            
                      Serial.println();
              }  // End of "for" of "z"
          Serial.println();
    digitalWrite(VFD_stb, HIGH);
    delayMicroseconds(2);
    cmd_with_stb((0b10001000) | 7); //cmd 4
    delayMicroseconds(2);
    pinMode(VFD_in, OUTPUT);  // Important this point!  // Important this point! Here I'm changing the direction of the pin to OUTPUT data.
    delay(1); 
}
void invertLED(){
  //This function is only to delay process and debug!
    digitalWrite(LED_BUILTIN, (!digitalRead(LED_BUILTIN)));
   writeLED();
              for(uint8_t s = 0x00; s < 5; s++){
                switch (s){
                   case 0: ctrlLEDon(0x01); break;
                   case 1: ctrlLEDon(0x02); break;
                   case 2: ctrlLEDon(0x04); break;
                   case 3: ctrlLEDon(0x08); break;
                   case 4: ctrlLEDon(0x10); break;
                   default: 
                   for(uint8_t i = 0x00; i < 4; i++){
                     digitalWrite(13, HIGH); delay(100); digitalWrite(13, LOW); 
                   }
                   break;
                 }
                 Serial.println(s, DEC);
                 delay(500);
               ctrlLEDoff();
               delay(500);
               }

}
void setup() {
  pinMode(LED_BUILTIN, OUTPUT);

  pinMode(VFD_in, OUTPUT);
  pinMode(VFD_clk, OUTPUT);
  pinMode(VFD_stb, OUTPUT);

  pinMode(CTL, INPUT_PULLUP); //Pin come from pantel and go to LOW when we press button of On/Off
  pinMode(A5STB, OUTPUT); // Pin to return the panel to standby state.

  pinMode(BUTTON_PIN2, INPUT_PULLUP);
  pinMode(BUTTON_PIN3, INPUT_PULLUP);

  Serial.begin(115200);

    //The next button trigger event is to find segments!
    attachInterrupt(digitalPinToInterrupt(BUTTON_PIN2),
                      buttonReleasedInterrupt2,
                      FALLING);
    //The next button trigger event is to swithc ON/OFF of panel!
    attachInterrupt(digitalPinToInterrupt(BUTTON_PIN3),
                      buttonReleasedInterrupt3,
                      FALLING);

  seconds = 0x00;
  minutes = 0x00;
  hours = 0x00;
  /*CS12  CS11 CS10 DESCRIPTION
  0        0     0  Timer/Counter1 Disabled 
  0        0     1  No Prescaling
  0        1     0  Clock / 8
  0        1     1  Clock / 64
  1        0     0  Clock / 256
  1        0     1  Clock / 1024
  1        1     0  External clock source on T1 pin, Clock on Falling edge
  1        1     1  External clock source on T1 pin, Clock on rising edge
 */
  // initialize timer1 
  cli();           // disable all interrupts
  //initialize timer1 
  //noInterrupts();           // disable all interrupts
  TCCR1A = 0;
  TCCR1B = 0;// This initialisations is very important, to have sure the trigger take place!!!
  TCNT1  = 0;
  // Use 62499 to generate a cycle of 1 sex 2 X 0.5 Secs (16MHz / (2*256*(1+62449) = 0.5
  OCR1A = 62498;      //OCR1A = 62498; I used this value, adjust it!      // compare match register 16MHz/256/2Hz
  TCCR1B |= (1 << WGM12);   // CTC mode
  TCCR1B |= ((1 << CS12) | (0 << CS11) | (0 << CS10));    // 256 prescaler 
  TIMSK1 |= (1 << OCIE1A);  // enable timer compare interrupt
 
    // Note: this counts is done to a Arduino 1 with Atmega 328... Is possible you need adjust
    // a little the value 62499 upper or lower if the clock have a delay or advnce on hours.
      
   
    CLKPR=(0x80);
    // //Set PORT see lines below, they are samples of configuration of PORTD
    //DDRD - The Port D Data Direction Register - read/write
    //PORTD - The Port D Data Register - read/write
    //PIND - The Port D Input Pins Register - read only
    DDRD = 0xFB;  // IMPORTANT: from pin 0 to 7 is port D, from pin 8 to 13 already is port B Here we define pin 2 of Arduino.
    PORTD=0x04;   // Important: Here we define finish of port of pin 2 of Arduino
    DDRB =0xFF;
    PORTB =0x00;
    pt6312_init();
    
    //only here I active the enable of interrupts to allow run the test of VFD
    //interrupts();             // enable all interrupts
    sei();
}
void loop(){
  // You can comment untill while cycle to avoid the tests of running.
      test_VFD();
      delay(500);
      clear_VFD();
      delay(500);
      // segBit();  //Only to test bit by bit
      // delay(500);
      // invertLED(); //Only to debug, if necessary!
      // findSegments(); // Uncomment and you can use this cycle to test all segments of VFD
      //
       for(int h=0; h < 10; h++){ // Print digits from 0 to 9 1 by 1 to all grids.
       k=h;
       send7segm();  //Presentation of all numbers at all digits
       }
    //
    clear_VFD();
    PT6312BLQ_RunWheels();
          while(1){
              send_update_clock();
              delay(200);
              readButtons();
              delay(200);
              PT6312BLQ_RunWheels();  //Instead of running wheels you can apply to two point separator 
          }
}
void buttonReleasedInterrupt2() {
  buttonReleased2 = true; // This is the line of interrupt button to advance one step on the search of segments!
}
void buttonReleasedInterrupt3() {
  buttonReleased3 = true; // This is the line of interrupt button to switch ON/OFF the panel trough the pin CTL.
  digitalWrite(11, !(digitalRead(11)));//This will result as a swap on pin 11.
}
ISR(TIMER1_COMPA_vect)   {  //This is the interrupt request
      secs++;
} 
