//Sensors station by dMb(2015)
//part of myMeteostation
//
//  Tested on atmega328p au
//  Written in Arduino 1.0.6
//default serial speed 9600
//Connections:
//D2    -DHT22 sensor
//D3    -DS temp sensor bus OneWire
//D7    -CFG A pin
//D8    -CFG B pin
//Short CFG pins A+B to config-terminal mode
//
#include <Arduino.h>

// - Arduino EEPROMex library 
// http://thijs.elenbaas.net/2012/07/extended-eeprom-library-for-arduino
#include <EEPROMex.h>
#include <EEPROMVar.h> 

struct sensdata {
  byte index;
  byte mac[8];
};
#define SENSORSMAX  10
sensdata Sensors[SENSORSMAX];
byte  SensorsCount;

void ClearSensors();
void LoadSensors();
void SaveSensors();
void PrintSensors();
void PrintSensorData(byte data, byte sensno);
#define  PRINTDATAR  0
#define  PRINTDATAV  1
#define  PRINTDATAM  2
void WriteSensor(byte pos);
void ReadSensor(byte pos, byte infos);
void UpdateDS();
void UpdateDHT();
void PrintHelp();
void ScanOW();
byte ReadVCC();

#define PrintH(h)    if (h <= 0x0F) {Serial.print("0");Serial.print(h, HEX);} else {Serial.print(h, HEX);}
#define PrintF(x)    Serial.print(F(x))
#define PrintFV(f,v) Serial.print(F(f)); Serial.print(v)
#define PrintFH(f,v) Serial.print(F(f)); PrintH(v, HEX)
#define Print(x)     Serial.print(x)
#define PrintD(x)    Serial.print(x/10);Serial.print('.');Serial.print(x%10)


#define  RM_REMOTE    0  //as remote terminal ask->response
#define  RM_REM_CMD   1  //received some command #
#define  RM_REM_CMDR  2  //wait for integer command R
#define  RM_REM_CMDV  3  //wait for integer command V
#define  RM_REM_CMDM  4  //wait for integer command M
#define  RM_REM_RES   5  //responsing to 

#define  RM_TERMINAL  10  //mode for configurations 
#define  RM_TERMCMDw  11  //procesing command w
#define  RM_TERMCMDr  12  //procesing command r
#define  RM_TERMCMDR  13  //procesing command R
#define  RM_TERMRES   15  //response to

#define  CFGA_PIN  7
#define  CFGB_PIN  8

//DHT Temperature & Humidity Sensor library for Arduino
//DHT22 sensor library Developed by Ben Adams - 2011
//Version 0.5: 15 Jan 2012 by Craig Ringer
#include <DHT22.h>

#define DHT22_PIN  2
#define DHT_PERIOD  5000  //millis
DHT22          DHT22(DHT22_PIN);
DHT22_ERROR_t  DHTerror;
int16_t  DHThum;
int16_t  DHTtemp;
unsigned long DHTNextRead = 0;

//OneWire library
// http://www.pjrc.com/teensy/td_libs_OneWire.html
//Version 2.2 Copyright (c) 2007, Jim Studt
#include <OneWire.h>

#define        OW_PIN  3
OneWire        OW(OW_PIN);

#define  OWC_DS18B20  0x28  //ds18b20 family code

byte  OWaddr[8];
byte  OWdata[12];
int16_t   OWtemp;

byte OWerror;
#define  OW_OK     1
#define  OW_ERROR  0
#define  OW_ERCNV  2  //check UpdateDS 
#define  OW_ERCRC  3  //TODO: add exclusions and some error 
#define  OW_ERRES  4  //

byte RunMode= RM_TERMINAL;

void setup()
{
  Serial.begin(9600);
  ClearSensors();
  LoadSensors();
  if (SensorsCount>SENSORSMAX)
    {
      PrintF("##Sensor load failed.");
      ClearSensors();
    }
  pinMode(CFGA_PIN, OUTPUT);
  pinMode(CFGB_PIN, INPUT);
  digitalWrite(CFGA_PIN, 0);
  if (digitalRead(CFGB_PIN)==0)
  {
    digitalWrite(CFGA_PIN, 1);
    if (digitalRead(CFGB_PIN))
    {//shorted
      digitalWrite(CFGA_PIN, 0);
      RunMode= RM_TERMINAL;
      PrintF("##Start in terminal mode...");
    }
    else
    {//no shorted
      RunMode= RM_REMOTE;
      PrintF("####");
    }
  }
  else
  {//pulluped?
    PrintF("\n\rCFG pins reading error!");
  }
}//end setup

void loop()
{
  byte income;
  if (RunMode >= RM_TERMINAL)
    {
      if (Serial.available())
        {
          income= Serial.read();
          switch(RunMode)
            {
              case RM_TERMINAL:
                if (income < ' ') return;
                switch(income)
                  {
                    case '?':
                      PrintHelp();
                    break;
                    case 'a':
                      PrintF("\n\rAdd next Sensor to table:");                     
                      WriteSensor(SensorsCount);
                    break;
                    case 'c':
                      PrintF("\n\rClear Sensors table");
                      ClearSensors();
                    break;
                    case 'v':
                      PrintF("\n\rInternal Voltage:");
                      PrintD(ReadVCC());
                    break;
                    case 'h':
                      if (millis()>= DHTNextRead) UpdateDHT();
                      PrintF("\n\rHumidity: ");
                      PrintD(DHThum);
                    break;
                    case 'l':
                      PrintF("\n\rLoad Sensors table");
                      LoadSensors();
                    break;
                    case 'o':
                      PrintF("\n\rOneWire search:");
                      ScanOW();
                    break;
                    case 'p':
                      PrintF("\n\rSensors table:");
                      PrintSensors();
                    break;
                    case 'r':
                      PrintF("\n\rRead sensor at index (0-9): ");
                      RunMode= RM_TERMCMDr;
                    break;
                    case 'R':
                      RunMode= RM_TERMCMDR;
                    break;
                    case 's':
                      PrintF("\n\rSave Sensors table");
                      SaveSensors();
                    break;
                    case 't':
                      if (millis()>= DHTNextRead) UpdateDHT();
                      PrintF("\n\rDHT Temp: ");
                      PrintD(DHTtemp);
                      PrintF("\n\rDS Temps: ");
                      for(byte pos= 3; pos< SensorsCount; pos++)
                      {
                        memcpy(OWaddr, Sensors[pos].mac, 8);
                        UpdateDS();
                        if (OWerror==OW_OK)
                          {
                            PrintD(OWtemp);
                            Print("\t");
                          }
                        else
                          {
                            PrintF("ERROR\t");
                          }
                      }                      
                    break;
                    case 'w':
                        PrintF("\n\rWriting sensor at index (0-9): ");
                        RunMode= RM_TERMCMDw;
                    break;
                    default:
                      PrintF("Type ? for help.");
                  }//end switch-income
              break;  //TODO clean following commands
              case RM_TERMCMDw:
                if (income < ' ') return;
                if ((income>=48) && (income <= 57))
                  {
                    income= map(income, 48, 57, 0, 9);
                    Print(income);
                    WriteSensor(income);
                    RunMode= RM_TERMINAL;
                  }
              break;
              case RM_TERMCMDr:
                if (income < ' ') return;
                if ((income>=48) && (income <= 57))
                  {
                    income= map(income, 48, 57, 0, 9);
                    Print(income);
                    ReadSensor(income, 1);
                    RunMode= RM_TERMINAL;
                  }
              break;
              case RM_TERMCMDR:
                if (income < ' ') return;
                if ((income>=48) && (income <= 57))
                  {
                    income= map(income, 48, 57, 0, 9);
                    ReadSensor(income, 0);
                    Print("\n\r"); 
                    RunMode= RM_TERMINAL;
                  }
              break;              
            }//end switch-runmode
        }
    }//end runmode-terminal
  else 
    {
      if (Serial.available())
        {
          income= Serial.read();
          if (income < ' ') return;
          if (RunMode==RM_REMOTE)
            {
              if (income=='#') RunMode= RM_REM_CMD;
            }
          else if (RunMode==RM_REM_CMD)
            {
              RunMode=RM_REMOTE;
              switch(income)
                {
                  case 'c':
                  Print(SensorsCount);
                  case '#':
                  Print('#');                 
                  break;
                  case 'V':
                    RunMode=RM_REM_CMDV;
                  break;
                  case 'R':
                    RunMode=RM_REM_CMDR;
                  break;
                  case 'M':
                    RunMode=RM_REM_CMDM;
                  break;
                }
            }
          else if ((RunMode >= RM_REM_CMDR) && (RunMode <= RM_REM_CMDM))
            {
              if (income=='#')//escape ;-)
                {
                  RunMode=RM_REMOTE;
                  return;
                }
              if ((income>=48) && (income <= 57))
                {
                  income= map(income, 48, 57, 0, 9);
                  switch(RunMode)
                    {
                      case RM_REM_CMDR:
                      PrintSensorData(PRINTDATAR, income);
                      break;
                      case RM_REM_CMDV:
                      PrintSensorData(PRINTDATAV, income);
                      break;
                      case RM_REM_CMDM:
                      PrintSensorData(PRINTDATAM, income);
                      break;
                    }
                 Print('#');
                 RunMode=RM_REMOTE;
                }
            }
        }
    }//end runmode-remote
}//end loop


//**********  functions
void PrintSensorData(byte data, byte sensno)
{
  if (sensno >= SensorsCount) return;
  switch(data)
    {
      case PRINTDATAV:
        if (sensno== 0) PrintF("V");
        else if (sensno == 1) PrintF("H");
        else PrintF("C");
      break;
      case PRINTDATAR:
        ReadSensor(sensno, 0);
      break;
      case PRINTDATAM:
      for (byte x=0; x<8; x++) PrintH(byte(Sensors[sensno].mac[x]));
      break;
    }
}
void ScanOW()
{
  //scan and print OW bus devices
byte i;
for (i=1; i< 10; i++)
    {
      if (!OW.search(OWaddr))
      {
        OW.reset_search();
        break;
      }
      else
      {
        PrintFV("\n\rNo:", i);
        PrintF("\t");
        for(byte a=0; a<8; a++) PrintH(OWaddr[a]);
        if (OneWire::crc8(OWaddr, 7) == OWaddr[7])
        {
          PrintF(" CRCok");
          if (OWaddr[0]== OWC_DS18B20)
          {
            PrintF("\tDS18B20");
            OW.reset();
            OW.select(OWaddr);
            OW.write(0x44, 0); //Start conv, parasite off
            delay(1000);
            if (OW.reset())
              {//conversion
              }
            else
              {
                PrintF("\n\rError after conversion!");
                break;
              }
            OW.select(OWaddr);
            OW.write(0xBE);  //read scratchpad
            for (byte a=0; a<9; a++)
            {
              OWdata[a]= OW.read();
            }
            if (OneWire::crc8(OWdata, 8) == OWdata[8])
              {
                PrintF(" CRCok");
                if ((OWdata[4] && 0x60))
                  {
                    PrintF(" 12bits");       
                    OWtemp= (OWdata[1] << 11) | (OWdata[0]<<3);
                    OWtemp= ((long)OWtemp*10)/128;
                    PrintF(" Temp:"); PrintD(OWtemp);                                    
                  }
                else
                  {
                    PrintF("RES not 12bits");
                  }
              }
            else
              {
                PrintF(" CRCerr");
              }
            
          }
          else
          {
            PrintF("\tFamily not supported!");
          }
        }
        else
        {
          PrintF(" CRCerr");
        }

      }
    }  
}

byte ReadVCC()
{
  long result;
  // Read 1.1V reference against AVcc
  ADMUX = _BV(REFS0) | _BV(MUX3) | _BV(MUX2) | _BV(MUX1);
  delay(2); // Wait for Vref to settle
  ADCSRA |= _BV(ADSC); // Convert
  while (bit_is_set(ADCSRA,ADSC));
  result = ADCL;
  result |= ADCH<<8;
  result = 1126400L / result; // Back-calculate AVcc in mV
  //return result;
  return (result/100);
}

void ClearSensors()
{
  for (byte a=0; a< SENSORSMAX; a++)
  {
    Sensors[a].index=0;
    for (byte x=0; x< 8; x++) Sensors[a].mac[x]=0;
  }
  SensorsCount= 3;
  Sensors[0].index= 0;
  Sensors[1].index= 1;
  Sensors[2].index= 2;
  memcpy(Sensors[0].mac, "_IntVol", 7);
  memcpy(Sensors[1].mac, "_DHThum", 7);
  memcpy(Sensors[2].mac, "_DHTtem", 7);
}


void LoadSensors()
{
  SensorsCount= EEPROM.readByte(5);
  EEPROM.readBlock(10, Sensors);
}
void SaveSensors()
{
  EEPROM.updateByte(5, SensorsCount);
  EEPROM.updateBlock(10, Sensors);
}

void PrintSensors()
{
  PrintF("\n\rNo:\tIndex:\tValue:\t\tValueHex:");
  for (byte a= 0; a < SensorsCount; a++)
      {
        PrintF("\n\r");
        Print(a+1);
        Print("\t");
        Print(Sensors[a].index);
        Print("\t");
        for (byte x=0; x<8; x++) Print(char(Sensors[a].mac[x]));
        Print("\t\t");
        for (byte x=0; x<8; x++) PrintH(byte(Sensors[a].mac[x]));
      }
}

byte HexToByte(byte H, byte L)
{   
    
  if ( H>= 48 && H <= 57) H = map(H, 48, 57, 0, 9);
  if ( L>= 48 && L <= 57) L = map(L, 48, 57, 0, 9);
  if ( H >= 65 && H <= 70) H = map(H, 65, 70, 10, 15);
  if ( L >= 65 && L <= 70) L = map(L, 65, 70, 10, 15);
  return ((H*16)+L);
}

void UpdateDHT()
{
  DHTerror= DHT22.readData();
  DHTNextRead= millis()+ DHT_PERIOD;
  if (DHTerror==DHT_ERROR_NONE)
    {
      DHTtemp= DHT22.getTemperatureCInt();
      DHThum= DHT22.getHumidityInt();
    }
}

void UpdateDS()
{
  OW.reset();
  OW.select(OWaddr);
  OW.write(0x44, 0); //Start conv, parasite off
  delay(1000);
  if (OW.reset())
    {
    OWerror= OW_OK;
    OW.select(OWaddr);
    OW.write(0xBE);
    for (byte a=0; a<9; a++)
      {
      OWdata[a]= OW.read();
      }
    if (OneWire::crc8(OWdata, 8) == OWdata[8])
        {
          if ((OWdata[4] && 0x60))
            {
              OWtemp= (OWdata[1] << 11) | (OWdata[0]<<3);
              OWtemp= ((long)OWtemp*10)/128;
            }
          else
            {
              OWerror= OW_ERRES;
            }
        }
    else
        {
          OWerror= OW_ERCRC;
        }    
    }
  else
    {//some error
    OWerror= OW_ERCNV;
    }
}

void ReadSensor(byte pos, byte infos)
{
  if (pos<SensorsCount)
    {
      if (pos > 2)
        {
          if(infos) PrintF("\n\rReading DS...");
          memcpy(OWaddr, Sensors[pos].mac, 8);
          UpdateDS();
          if(infos) 
            {
            PrintF("\n\rTemp: ");
            PrintD(OWtemp);
            }
          else Print(OWtemp);
        }
      else if (pos > 0)
        {
          if(infos) PrintF("\n\rReading DHT...");
          if (millis() >= DHTNextRead)
            {
              UpdateDHT();
              if (DHTerror != DHT_ERROR_NONE)
              {
              if(infos) PrintF("\n\rError DHT.");            
              } 
            }
           switch(pos)
            {
              case 1:
              if(infos) {PrintF("\n\rHumi: ");PrintD(DHThum);}
              else Print(DHThum);
              break;
              case 2:
              if(infos) {PrintF("\n\rTemp: ");PrintD(DHTtemp);}
              else Print(DHTtemp);
              break;
            }      
        }
      else
        {
          if(infos) {PrintF("\n\rVolt: ");PrintD(ReadVCC());}
          else Print(ReadVCC());
        }
    }
  else
    {
      if(infos) PrintF("\n\rWrong sensor index.");      
    }
}

void WriteSensor(byte pos)
{
 if ((pos < 3) || (pos >= SENSORSMAX))
   {
     PrintF("\n\rWrong sensor index.");
     return;
   }
 else if (pos > SensorsCount)
   {
     PrintF("\n\rUse add sensor.");
     return;
   }
 PrintF("\n\rType ValueHex (16 chars):");
 if (pos==SensorsCount) SensorsCount++;
 Sensors[pos].index= pos;
 byte tadr[8]= {0,0,0,0,0,0,0,0};
 byte hb;
 byte lb;
 byte i=0;
 while(i<8)
 {
 if (Serial.available() >= 2)
       {
         hb= Serial.read();
         lb= Serial.read();
         tadr[i++]= HexToByte(hb,lb);
       }
 
 }
 for (int i; i<8; i++)
   {
     PrintH(tadr[i]);
     Sensors[pos].mac[i]= tadr[i];    
   }
 PrintF("\n\rValue changed at pos :");
 Print(pos);
}

void PrintHelp()
{
PrintF("\n\r\n\r\n\r\tSensors station by dMb (2015)");
PrintF("\n\rCommands list:");
PrintF("\n\r\t?\tprint this\
\n\r\n\r\tp\tprint ST\
\n\r\tl\tload ST from flash\
\n\r\ts\tsave ST to flash\
\n\r\tc\tclear ST\
\n\r\n\r\to\tscan DS bus\
\n\r\ta\tadd next sensor to ST (aHEXMACADDRESS)\
\n\r\tw\twrite HEXMACADDRESS to ST (w3HEXMACADDRESS)\
\n\r\n\r\tr\tprint sensor value (r2)\
\n\r\tR\tprint sensor value only (R2)\
\n\r\tv\tprint Voltage\
\n\r\th\tprint Humidity\
\n\r\tt\tprint all temperatures");
PrintF("\n\rWhere: ST-SensorTable, DS-Maxim OneWire device, DHT-DHT22/AM2303 sensor");
PrintF("\n\rHowTo:\
\n\r1.Clear ST\n\r2.Sensors 0-2 are added by default\
\n\r3.Scan DS bus (type o)\n\r4.Add sensor to ST (type aHEXaddres)\
\n\r5.Check ST (print- type p)\
\n\r6.Save table to flash (type s)\n\rReset then and check ST after reboot.");
}


