
/* Serial Switch Code
   Cleverthings 2016
   This code selects a channel and sets to on/off through commands issued from serial port.
   Designed for ATmega2560. 
   Digital Outputs: 32 (array output[] )
   PWM outputs....: 12 (D2 to D13)
   Analog inputs..: 16 (array analog[] )
   For more information, visit www.cleverthings.pt/serialswitch
*/

#include <EEPROM.h>
const int MEM_POS_ID =0; 
const int MEM_POS_INIT= 17;

String command;
const byte output[]= {53,51,49,47,45,43,41,39,37,35,33,31,29,27,25,23,52,50,48,46,44,42,40,38,36,34,32,30,28,26,24,22};      //pins Out digital in order of "SW_seq"
const byte analog[16] = {A0,A1,A2,A3,A4,A5,A6,A7,A8,A9,A10,A11,A12,A13,A14,A15};                                             //pins Input analog
boolean blinkFlag = false;          //global flag for blink outputs
long blinkStamp;                    //global timestamp for last blink
byte blinkValue = 1;                //global initial value for blink in single output (HIGH or LOW)
int blinkInterval = 100;            //global interval for blink
boolean test = false;               //global flag for cyclical board test
int ti = 100;                       //global value of test interval
String blinkString;                 //global string of blink
String outstring;                   //global result of reading output port


void setup() {

  for (int p = 0; p<sizeof(output); p++){       //initialize output ports
    pinMode(output[p],OUTPUT);
    digitalWrite(output[p],LOW);
  }


  pinMode(13, OUTPUT); //Debug led
  Serial.begin(9600); 
  
  Serial.println("Firmware    : SSW3216-USB");
  Serial.println("Release     : 1.0");
  Serial.println("Manufacturer: Cleverthings");
  Serial.print("Equipment ID: ");Serial.println(readMem(MEM_POS_ID));
  String initCommand = readMem(MEM_POS_INIT);
  Serial.print("Init command: ");Serial.println(initCommand);
  Serial.println("");

  if(initCommand != "NOT DEFINED"){
    parseCommand(initCommand, initCommand.length());
  }
}

void loop() {


 //EVALUATE INCOMING SERIAL DATA
  if (Serial.available()){
    
    //disable test if it is running  
     if (test){ test = false; clear();}
    
    char c = Serial.read();
    if (c== 13 || c==10){
      parseCommand(command, command.length());
      command = "";
     }
     else {
      if(c=='[' || c==']' ){
        //discard
      } else {
        command += c;
      }
     }
  }



  //EVALUATE FLAGs

    if (blinkFlag){
      if ( (millis() - blinkStamp) >= blinkInterval){
        blink();
        
      }
    }
 
  if (test){
    testBoard();
  }
}

void parseCommand(String cmd, int len) {

  //getting command header
  char ctype = cmd.charAt(0);
  char com = cmd.charAt(1);
  len = len - 2;

  //evaluate command type and comand itself
    if (ctype == '!' && com == 'S')
    {  
        setMultiple(cmd);
    } 
    
    else if (ctype == '!' && (com == 'B' || com == 'b') )
    {
      blinkString = cmd;
      blink();
      blinkFlag = true;
    }

    else if (ctype == '!' && com == 's')
    {
      byte psep1 = cmd.indexOf(';');                               //position of 1st separator          
      byte psep2 = cmd.indexOf(';',psep1+1);                       //position of 2nd separator
      int port = cmd.substring(psep1+1,psep2).toInt();             //pin 
      int portval = cmd.substring( (psep2+1), (psep2+2)).toInt();  //value
      digitalWrite(port,portval);
    
    }

    else if (ctype == '!' && com == 'p')
    {
      byte psep1 = cmd.indexOf(';');                               //position of 1st separator          
      byte psep2 = cmd.indexOf(';',psep1+1);                       //position of 2nd separator
      int port = cmd.substring(psep1+1,psep2).toInt();             //pin 
      int portval = cmd.substring( (psep2+1), (psep2+4)).toInt();  //value
      if(portval>255 || portval < 0)portval=0;
      analogWrite(port,portval);
      Serial.print("#");Serial.println(mapfloat(portval,0.0,255.0,0.0,5.0));
    
    }

    else if (ctype == '?' && com == '@')
    {
      Serial.print("#");
      for (int a=0; a<16; a++){
       int value = analogRead(analog[a]);
       delay(10);
      Serial.print(value); if( a<15 )Serial.print(";");
      }
      Serial.println();
    }

    else if (ctype == '?' && com == 'a')
    {
        byte psep1 = cmd.indexOf(';');                               //position of 1st separator          
        byte psep2 = cmd.indexOf(';',psep1+1);                       //position of 2nd separator
        int port = cmd.substring(psep1+1,psep2).toInt();             //pin 
        int portval = cmd.substring( (psep2+1), (psep2+4)).toInt();  //amount of samples
        readAnalog(port,portval);
    }

    else if (ctype == '!' && com == 'T')
    {
      blinkFlag = false;
      blinkString = "";
      test = true;
 
    }

    else if (ctype == '!' && com == 'R')
    {
      randomSeed(analogRead(0));
      clear();
      
      int max = output[0];
      int min = output[0];
      for (byte r=1; r<sizeof(output);r++){                      //extract each element of array and compare
        if(output[r] > max){                                     //to find the HIGHEST and the LOWEST value
          max = output[r];
        }
        if(output[r] < min){
          min = output[r];  
        }
      }
      byte randNumber = random(min,max);       
      Serial.print("#");Serial.println(randNumber);
      String cmdrandom = "!b;"+String(randNumber)+";100";
      blinkString= cmdrandom;
      Serial.println(blinkString);
      blink();
      blinkFlag = true;
 
    }

    else if (ctype == '?' && (com == 'S' || com == 's') )
    {
      Serial.print("#");Serial.println(readPort(cmd));
      outstring = "";
    }

    else if (ctype == '!' && com == 'D')
    {
      if(cmd.length() < 18){
      writeMem(cmd.substring(2), 16,MEM_POS_ID);}
    }

    else if (ctype == '!' && com == 'i')
    {
      writeMem(cmd.substring(2), len, MEM_POS_INIT);
    }

    else if (ctype == '?' && com == 'i')
    {
      String initCommand = readMem(MEM_POS_INIT);
      Serial.print("#");Serial.println(initCommand);
    }

    else if (ctype == '?' && com == 'O')
    {
      Serial.print("#");
      for (int o = 0; o<sizeof(output);o++){
        Serial.print(output[o]); if( (sizeof(output)- o >1))Serial.print(";");
      }
      Serial.println();
    }

    else if (ctype == '!' && com == 'C')
    {
      clear();
      Serial.println("Clear");
    }

    else {
      Serial.println("COMMAND NOT RECOGNIZED");
    }
  Serial.println(cmd); 
  
}


void blink(){
   
   char blinkcmd = blinkString.charAt(1);    //get the type of blink command (multiple or single)
   static boolean blinkInvert;               //define a static variable to invert values
    
   //Multiple outputs
   if (blinkcmd == 'B'){
    byte lencmd = blinkString.length();                                 //check the size of blink string command
    lencmd = lencmd - 2;                                                //remove !B from string
    blinkInterval = 100;                                                //redefine blink interval for default value
    for (byte i = 0; i<lencmd; i++){
      int cmdvalue = blinkString.substring(i+2,i+3).toInt();            //1's and 0's strings to int
      if(cmdvalue==1){                                                  //is a blink mapped bit?
              digitalWrite(output[i],blinkInvert);
      }
      blinkStamp = millis();
     }
     (blinkInvert==true)?(blinkInvert=false):(blinkInvert=true);
   }
  
  //Single output
  if (blinkcmd == 'b'){
    byte psep1 = blinkString.indexOf(';');                               //position of 1st separator          
    byte psep2 = blinkString.indexOf(';',psep1+1);                       //position of 2nd separator
    int port = blinkString.substring(psep1+1,psep2).toInt();             //pin 
    blinkInterval = blinkString.substring( (psep2+1), (psep2+4)).toInt();  //delay in ms (max 999)
    digitalWrite(port,blinkValue);
    (blinkValue == 1)?(blinkValue=0):(blinkValue=1);
    blinkStamp = millis();
    
  }
}

void testBoard(){

  for (int p = 22; p<54; p++){
       digitalWrite(p,LOW);
       delay(ti);
  }

  for (int q = 22; q<54; q++){
       digitalWrite(q,HIGH);
       delay(ti);
  }
  
}

String readPort(String cmd){

  byte OutStatus[][3] = { {PORTB, PB0,53},
                          {PORTB, PB2,51},
                          {PORTL, PL0,49},
                          {PORTL, PL2,47},
                          {PORTL, PL4,45},
                          {PORTL, PL6,46},
                          {PORTG, PG0,41},
                          {PORTG, PG2,39},
                          {PORTC, PC0,37},
                          {PORTC, PC2,35},
                          {PORTC, PC4,33},
                          {PORTC, PC6,31},
                          {PORTA, PA7,29},
                          {PORTA, PA5,27},
                          {PORTA, PA3,25},
                          {PORTA, PA1,23},
                          {PORTB, PB1,52},
                          {PORTB, PB3,50},
                          {PORTL, PL1,48},
                          {PORTL, PL3,46},
                          {PORTL, PL5,44},
                          {PORTL, PL7,42},
                          {PORTG, PG1,40},
                          {PORTD, PD7,38},
                          {PORTC, PC1,36},
                          {PORTC, PC3,34},
                          {PORTC, PC5,32},
                          {PORTC, PC7,30},
                          {PORTA, PA6,28},
                          {PORTA, PA4,26},
                          {PORTA, PA2,24},
                          {PORTA, PA0,22}
                        };
                          

 if (cmd.charAt(1) == 'S'){
   for (int l = 0; l < 32; l++){                      
      byte port = OutStatus[l][0];
      byte pin = OutStatus[l][1];
      byte value = port & (1<<pin);
      if (value > 0){
        value = 1;
       }else{ 
        value = 0;
        }
     outstring += value;
     
    }
   return(outstring);
   
 }
 if (cmd.charAt(1) == 's'){
      byte psep1 = cmd.indexOf(';');                               //position of 1st separator          
      byte psep2 = cmd.indexOf(';',psep1+1);                       //position of 2nd separator
      int digpin = cmd.substring(psep1+1,psep2).toInt();             //pin 
     
     for (int l = 0; l < 32; l++){                     
      byte port = OutStatus[l][0];
      byte pin = OutStatus[l][1];
      byte dig = OutStatus[l][2];
      if (digpin == dig){
        byte value = port & (1<<pin);
        if (value > 0){
          value = 1;
        }else{ 
          value = 0;
        }
        return(String(value));
        
       }
     }
  }
}

void writeMem (String data, byte maxlen, int pos) {

  if (data.length() > maxlen){
    Serial.println("EXCEEDS MAXIMUM LENGTH");
    return;
  }

  if (pos > 100){
    Serial.println("MEMORY ADDRESS INVALID");
  }
  
  for (int i = 0 ; i < data.length(); i++){
    EEPROM.write(( pos+i),data.charAt(i));
  }
    EEPROM.write((pos + data.length()),':');
}

String readMem (int pos) {

  if (pos > 100){
    Serial.println("MEMORY ADDRESS INVALID");
  }
  String MemStoreValue;
  
  for (int i = 0; i < 32; i++){
    char databyte = EEPROM.read(pos +i);
    if (databyte == ':'){
      return(MemStoreValue);
    }
    MemStoreValue += databyte;
  }
  return("NOT DEFINED");
}

void clear(){
  
      blinkString = "";
      blinkFlag = false;
      
      for (int o = 0; o<sizeof(output);o++){
        digitalWrite(output[o],LOW); 
      }
}

void readAnalog(byte port, int samples){

if (samples < 10){
  for (byte i = 0; i < samples; i++){
    int value = analogRead(port);
    delay(10);
    Serial.print("#");Serial.println(value);
  }
} 

}

void lastState(String last){
  setMultiple(last);
}

void setMultiple(String bitmap){
        
        String cmd = bitmap;
        byte len = cmd.length() - 2;
 
        for (byte i = 0; i<len ;i++){
          int cmdvalue = cmd.substring(i+2,i+3).toInt();  //1's and 0's strings to int
          digitalWrite(output[i],cmdvalue);
          blinkFlag = false;
          blinkString = "";
        }  
}

float mapfloat(long x, long in_min, long in_max, long out_min, long out_max){
 return (float)(x - in_min) * (out_max - out_min) / (float)(in_max - in_min) + out_min;
}
