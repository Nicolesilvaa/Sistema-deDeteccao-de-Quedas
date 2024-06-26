

#include <SPI.h>                                 //the lora device is SPI based so load the SPI library
#include <SX128XLT.h>                            //include the appropriate library   
#include "Settings.h"                            //include the setiings file, frequencies, LoRa settings etc   

SX128XLT LT;                                     //create a library class instance called LT

uint32_t RXpacketCount;
uint32_t errors;

uint8_t RXBUFFER[RXBUFFER_SIZE];                 //create the buffer that received packets are copied into

uint8_t RXPacketL;                               //stores length of packet received
int16_t  PacketRSSI;                             //stores RSSI of received packet
int8_t  PacketSNR;                               //stores signal to noise ratio of received packet

void setup(){

  pinMode(LED1, OUTPUT);                        //setup pin as output for indicator LED
  led_Flash(2, 125);                            //two quick LED flashes to indicate program start

  Serial.begin(9600);
  Serial.println();
  Serial.print(F(__TIME__));
  Serial.print(F(" "));
  Serial.println(F(__DATE__));
  Serial.println(F(Program_Version));
  Serial.println();
  Serial.println(F("Iniciando comunicação - Receptor"));
  Serial.println(F("Lasid UFBA"));
  Serial.println();

  if (BUZZER > 0){

    pinMode(BUZZER, OUTPUT);
    digitalWrite(BUZZER, HIGH);
    delay(50);
    digitalWrite(BUZZER, LOW);
  }

  SPI.begin();

  //SPI beginTranscation is normally part of library routines, but if it is disabled in the library
  //a single instance is needed here, so uncomment the program line below
  //SPI.beginTransaction(SPISettings(8000000, MSBFIRST, SPI_MODE0));

  //setup hardware pins used by device, then check if device is found
  if (LT.begin(NSS, NRESET, RFBUSY, DIO1, DIO2, DIO3, RX_EN, TX_EN, LORA_DEVICE)){

    Serial.println(F("LoRa Device found"));
    //led_Flash(2, 1);
    delay(20);
  }

  else{

    Serial.println(F("No device responding"));
    while (1){

      led_Flash(50, 50);                                       //long fast speed LED flash indicates device error
    }
  }

  //The function call list below shows the complete setup for the LoRa device using the information defined in the
  //Settings.h file.
  //The 'Setup LoRa device' list below can be replaced with a single function call;
  //LT.setupLoRa(Frequency, Offset, SF , Bandwidth, CodeRate);

  //***************************************************************************************************
  //Setup LoRa device
  //***************************************************************************************************
  LT.setMode(MODE_STDBY_RC);
  LT.setRegulatorMode(USE_LDO);
  LT.setPacketType(PACKET_TYPE_LORA);
  LT.setRfFrequency(Frequency, Offset);
  LT.setBufferBaseAddress(0, 0);
  LT.setModulationParams(SF, Bandwidth, CodeRate);
  LT.setPacketParams(12, LORA_PACKET_VARIABLE_LENGTH, 255, LORA_CRC_ON, LORA_IQ_NORMAL, 0, 0);
  LT.setDioIrqParams(IRQ_RADIO_ALL, (IRQ_TX_DONE + IRQ_RX_TX_TIMEOUT), 0, 0);
  LT.setHighSensitivity();
  //LT.setLowPowerRX();
  //***************************************************************************************************

  Serial.print(F("Dados Recebdidos - RXBUFFER_SIZE "));
  Serial.println(RXBUFFER_SIZE);
  Serial.println();

}

void loop(){

  RXPacketL = LT.receive(RXBUFFER, RXBUFFER_SIZE, 60000, WAIT_RX); //wait for a packet to arrive with 60seconds (60000mS) timeout

  digitalWrite(LED1, HIGH);                      //something has happened

  if (BUZZER > 0){                             //turn buzzer on
    digitalWrite(BUZZER, HIGH);
  }

  PacketRSSI = LT.readPacketRSSI();              //read the recived RSSI value
  PacketSNR = LT.readPacketSNR();                //read the received SNR value

  if(RXPacketL == 0){  
    
    packet_is_Error();
  
  }                        //if the LT.receive() function detects an error, RXpacketL == 0
  else{
    
    packet_is_OK();
  
  }

  if (BUZZER > 0){
    
    digitalWrite(BUZZER, LOW);
  
  }                    //buzzer off

  digitalWrite(LED1, LOW);                                        //LED off
  Serial.println();

}


void packet_is_OK(){

  int16_t IRQStatus, localCRC;

  IRQStatus = LT.readIrqStatus();                  //read the LoRa device IRQ status register
  RXpacketCount++;

  printElapsedTime();                              //print elapsed time to Serial Monitor
  Serial.print(F(",RSSI,"));
  Serial.print(PacketRSSI);
  Serial.print(F("dBm,SNR,"));
  Serial.print(PacketSNR);
  Serial.print(F("dB,Length,"));
  Serial.print(RXPacketL);
  Serial.print(F(",Packets,"));
  Serial.print(RXpacketCount);
  Serial.print(F(",Errors,"));
  Serial.print(errors);
  Serial.print(F(",IRQreg,"));
  Serial.print(IRQStatus, HEX);

  led_Flash(2, 20);  
  rssiMedio(RXpacketCount,PacketRSSI);  
     
}

void packet_is_Error(){

  uint16_t IRQStatus;
  IRQStatus = LT.readIrqStatus();                   //read the LoRa device IRQ status register
  printElapsedTime();                               //print elapsed time to Serial Monitor

  if (IRQStatus & IRQ_RX_TIMEOUT){                 //check for an RX timeout

    Serial.print(F(" RXTimeout"));
  }

  else{
    errors++;
    Serial.print(F(" PacketError"));
    Serial.print(F(",RSSI,"));
    Serial.print(PacketRSSI);
    Serial.print(F("dBm,SNR,"));
    Serial.print(PacketSNR);
    Serial.print(F("dB,Length,"));
    Serial.print(LT.readRXPacketL());               //get the real packet length
    Serial.print(F(",Packets,"));
    Serial.print(RXpacketCount);
    Serial.print(F(",Errors,"));
    Serial.print(errors);
    Serial.print(F(",IRQreg,"));
    Serial.print(IRQStatus, HEX);
    LT.printIrqStatus();                            //print the names of the IRQ registers set
  }

  delay(250);                                       //gives a longer buzzer and LED flash for error

}

// Calcula o RSSI médio de envio de pacotes para cada intervalo de 200 envios.

void rssiMedio(uint32_t &RXpacketCount, int16_t  &PacketRSSI){

  uint16_t somaRSSI;
  somaRSSI += PacketRSSI;

  if(RXpacketCount % 200 == 0){

    uint16_t  mediaRSSI = somaRSSI/200;

    Serial.println();
    Serial.print("Intensidade total da recepção de 200 mensagens = ");
    Serial.print(somaRSSI);
    Serial.println(" dBm");
    Serial.print("Intensidadde média de transmissão de 200 mensagens = "); // RSSI -> indicador de intensidade de sinal recebido 
    Serial.print((mediaRSSI));
    Serial.print(" dBm");
    Serial.println();    

  }
}

void printElapsedTime(){

  float seconds;
  seconds = millis() / 1000;
  Serial.print(seconds, 0);
  Serial.print(F("s"));
}

void led_Flash(uint16_t flashes, uint16_t delaymS){

  uint16_t index;

  for (index = 1; index <= flashes; index++){
    
    digitalWrite(LED1, HIGH);
    delay(delaymS);
    digitalWrite(LED1, LOW);
    delay(delaymS);
  }
}

