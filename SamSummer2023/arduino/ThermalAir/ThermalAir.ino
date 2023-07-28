
/***************************************************************************************************\

Name: ThermalAir.ino
Author: A. Loreti phys2114.ox.ac.uk ok20686@bristol.ac.uk, Modified by S. Haken ji20840@bristol.ac.uk
Date: July 2023
Description: Arduino code for interfacing with test stand

\***************************************************************************************************/


#include <Wire.h>
#include <Adafruit_MAX31865.h> //from MAX31865 library from Adafruit
#include <HIH61xx.h>
#include <AsyncDelay.h>



#define POLYNOMIAL 0x31     //P(x)=x^8+x^5+x^4+1 = 100110001   //magic number used in the CRC decoding for the flowmeter
#define sfm3300i2c 0x40     //I2C address of flowmeter

const int humidityCS = 7;   //Chip Select for humidity sensor is connected to pin 7

bool human_readable = false;    //flag to set whether to transmit human or machine readable output
bool airflow_stable = false;    //flag to show if airflow control loop is stable
bool broadcast_flag = true;     //flag to enable/disable auto transmission of serial output on every measurement cycle


//---Stuff for MAX31865 temperature sensor
// Use software SPI: CS, DI, DO, CLK
//Adafruit_MAX31865 thermo = Adafruit_MAX31865(10, 11, 12, 13);
// use hardware SPI, just pass in the CS pin
Adafruit_MAX31865 thermo = Adafruit_MAX31865(10);
// The value of the Rref resistor. Use 430.0 for PT100 and 4300.0 for PT1000
#define RREF      4300.0
// The 'nominal' 0-degrees-C resistance of the sensor
// 100.0 for PT100, 1000.0 for PT1000
#define RNOMINAL  1000.0


//Humidity Sensor HIH6131-021
HIH61xx<TwoWire> hih(Wire);
AsyncDelay samplingInterval;


const unsigned mms = 1000; // measurement interval in ms
unsigned long mt_prev = millis(); // last measurement time-stamp
unsigned long ms_display = millis(); // timer for display "soft interrupts"


float flow; // current flow value in slm
float air_flow; // thermal air flow (scfm or L/min)
float flow_values[10] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0}; //an array to hold historical flow values
int flow_array_index = 0;
float flow_moving_avg = 0;
int i;
bool crc_error; // flag

float temperature = 0; //temperature which is calculated from reading the MAX31865 sensor
float air_temp;        //thermal air temperature
float DUT_temp;        //DUT temp from thermocouple
float air_setpoint;    //thermal air temperature setpoint

float rel_humidity;
float amb_temp;
bool printed = true;


void setup() {

  pinMode(humidityCS, OUTPUT);
  digitalWrite(humidityCS, LOW);    //TEMPORARY - assert this low to disable humidity sensor. - no code written for it yet.

  Wire.begin();                     //set up serial port and I2C
  Serial.begin(9600);
  Serial1.begin(9600, SERIAL_8N1);
  delay(500);

  // soft reset
  Wire.beginTransmission(sfm3300i2c);
  Wire.write(0x20);
  Wire.write(0x00);
  Wire.endTransmission();
  delay(1000);

  /// Enter remote mode on thermal air
  // if thermal air left in remote mode after switching off
  // needs to be in local mode before re-entering remote mode
  // delays added as switching modes is not instantaneous
  Serial1.flush();
  send_command("%GL");
  delay(3000);
  send_command("%RM");
  delay(3000);

  #if 1
  Wire.beginTransmission(sfm3300i2c);
  Wire.write(0x31);  // read serial number
  Wire.write(0xAE);  // command 0x31AE
  Wire.endTransmission();
  if (6 == Wire.requestFrom(sfm3300i2c,6)) {
      uint32_t sn = 0;
      sn = Wire.read(); sn <<= 8;
      sn += Wire.read(); sn <<= 8;
      Wire.read(); // CRC - omitting for now
      sn += Wire.read(); sn <<= 8;
      sn += Wire.read();
      Wire.read(); // CRC - omitting for now
      Serial.println(sn);
  } else {
      Serial.println("serial number - i2c read error");
  }
  #endif

  // start continuous measurement
  Wire.beginTransmission(sfm3300i2c);
  Wire.write(0x10);
  Wire.write(0x00);
  Wire.endTransmission();

  delay(1000);

  //setup for the MAX31865 temperature sensor
  thermo.begin(MAX31865_2WIRE);  // set to 2,3or4WIRE as necessary
  //thermo.begin(MAX31865_3WIRE);
  //thermo.begin(MAX31865_4WIRE);


  // turning on thermal air DUT control mode, setting sensor type to T-type thermocouple and turning on auto tune mode
  send_command("DSNS 4");
  send_command("DUTM 1");
  send_command("DUTN 1");

  /// turn on thermal air compressor and main nozzle air flow
  //send_command("COOL 1;FLOW 1");

  // set initial thermal air setpoint to 20C
  send_command("SETN 0");
  send_command("SETP 20");
  air_setpoint = 20.0;

}




void loop() { //-----------------Main loop------------------//

  unsigned long ms_curr = millis();

  if (ms_curr - ms_display >= mms) { // "soft interrupt" every mms milliseconds
      ms_display = ms_curr;

      //read from the flowmeter
      SFM_measure();

      /// Read air flow rate from thermal air (litres/min)
      send_command("FLRL?");
      air_flow = read_output().toFloat();
      delay(20);

      /// Read air temperature from thermal air
      send_command("TMPA?");
      air_temp = read_output().toFloat();
      delay(20);

      /// Read DUT temp from thermocouple
      send_command("TMPD?");
      DUT_temp = read_output().toFloat();
      delay(20);

      /// Read temperature in test stand
      temperature = thermo.temperature(RNOMINAL, RREF);




      if (samplingInterval.isExpired() && !hih.isSampling()) {
          hih.start();
          printed = false;
          samplingInterval.repeat();
          //Serial.println("Sampling started (using Wire library)");
      }
      hih.process();
      hih.read(); // instruct the HIH61xx to take a measurement - blocks until the measurement is ready.
      if (hih.isFinished() && !printed) {
          printed = true;
          // Print saved values
          rel_humidity = hih.getRelHumidity() / 100.0;
          amb_temp = hih.getAmbientTemp() / 100.0;
      }


      flow_values[flow_array_index]=flow;  //record the current flow value in the array
      flow_array_index++;
      if (flow_array_index == 10){
          flow_array_index = 0;
      }

      //calculate moving windowed average
      flow_moving_avg = 0;
      for (i=0; i<10; i++){
          flow_moving_avg += flow_values[i];
      }
      flow_moving_avg /= 10;


      if (broadcast_flag){
          transmit_data(); // output data via serial port
      }

  } // end measurement cycle


  // checking for serial input
  // add any commands to used by the midas front end here
  while (Serial.available() > 0) {
      char command;
      command = Serial.read();

      if(command == '?'){
          Serial.println(F("Commands:"));   //F stores the strings in Flash memory, saves using RAM space.
          Serial.println(F("?: Help"));
          Serial.println(F("v: produce (V)erbose human readable output"));
          Serial.println(F("m: produce compact (M)achine readable output"));
          Serial.println(F("s: new (S)etpoint. followed by integer. e.g. s35 for 35 l/min"));
          Serial.println(F("r: (R)un - begin closed loop control - not implememted yet"));
          Serial.println(F("x: Break - stop closed loop control and turn off fan - not implememted yet"));
          Serial.println(F("t: (T)ransmit all measurements"));
          Serial.println(F("b: (B)roadcast measurements"));
          Serial.println(F("n: (N)o broadcasting"));
          Serial.println(F("r: (r)eset remote mode on Thermal Air"));
          Serial.println(F("c: Turn (c)ompressor on/off"));
          Serial.println(F("f: Turn air (f)low on/off"));
          Serial.println(F("d: Set Thermal Air to (D)UT mode"));

      }

      if (command == 's'){
          //Serial.print("New setpoint entered:");
          float setpoint_input = Serial.parseFloat();
          //Serial.println(setpoint_input);
          air_setpoint = constrain(setpoint_input, 0, 1000);

          String command = "SETP " + String(air_setpoint);
          send_command(command);
      }

      if (command == 'v'){
          human_readable = true;
      }

      if (command == 'm'){
          human_readable = false;
      }

      if (command == 'b'){
          broadcast_flag = true;
      }

      if (command == 'n'){
          broadcast_flag = false;
      }

      if (command == 't'){
          transmit_data(); // output data via serial port
      }

      // reset remote mode on thermal air
      if (command == 'r'){
          Serial1.flush();
          send_command("%GL");
          delay(5000);
          send_command("%RM");
          delay(5000);
      }

      // turn compressor on/off
      if (command == 'c'){
        send_command("COOL?");
        float on = read_output().toFloat();

        if (on == 0) send_command("COOL 1");
        else if (on == 1) send_command("COOL 0");
      }

      // turn air flow on/off
      if (command == 'f'){
        send_command("FLOW?");
        float on = read_output().toFloat();

        if (on == 0) send_command("FLOW 1");
        else if (on == 1) send_command("FLOW 0");
      }

      // Set thermal air to DUT mode
      if (command == 'd'){
        send_command("DSNS 4");
        send_command("DUTM 1");
        send_command("DUTN 1");
      }





      command = 0; //reset current command to null

  } // end while serial available

} //------------------end main loop-------------------//




//-----Reads from Flowmeter and converts into SFM units
void SFM_measure() {
    if (3 == Wire.requestFrom(sfm3300i2c, 3)) {
        uint8_t crc = 0;
        uint16_t a = Wire.read();
        crc = CRC_prim (a, crc);
        uint8_t  b = Wire.read();
        crc = CRC_prim (b, crc);
        uint8_t  c = Wire.read();
        unsigned long mt = millis(); // measurement time-stamp
        if (crc_error = (crc != c)) // report CRC error
            return;
        a = (a<<8) | b;
        float new_flow = ((float)a-32768) / 120;

        flow = new_flow;
        unsigned long mt_delta = mt - mt_prev; // time interval of the current measurement
        mt_prev = mt;

    } else {
        // report i2c read error
    }
}




//-----Checks CRC of received flowmeter data.
uint8_t CRC_prim (uint8_t x, uint8_t crc) {
    crc ^= x;
    for (uint8_t bit = 8; bit > 0; --bit) {
        if (crc & 0x80) crc = (crc << 1) ^ POLYNOMIAL;
        else crc = (crc << 1);
    }
    return crc;
}



//Humidity Sensor - control functions
void powerUpErrorHandler(HIH61xx<TwoWire>& hih){
    Serial.println("Error powering up HIH61xx device");
}
void readErrorHandler(HIH61xx<TwoWire>& hih){
    Serial.println("Error reading from HIH61xx device");
}




/// Thermal Air send command
void send_command(String command)
{
  Serial1.print(command);
  Serial1.print("\r\n");
  return;
}

/// Thermal Air read output
String read_output()
{
  String output;
  while(Serial1.available() > 0)
  {
    char received = Serial1.read();
    output += received;

    if (received == '\n')
    {
      return output;
    }
  }
}



////----------------- OUTPUT ---------------////
void transmit_data (void) {
    if (human_readable){
        Serial.print("Temp: ");
        Serial.print(temperature);
        Serial.print("\t");

        Serial.print("DUT Temp: ");
        Serial.print(DUT_temp);
        Serial.print("\t");
        Serial.print("Air Temp: ");
        Serial.print(air_temp);
        Serial.print("\t");
        Serial.print("Flow In: ");
        Serial.print(air_flow);
        Serial.print("\t");

        Serial.print("Flow Out: ");
        Serial.print(flow);
        Serial.print("\t");
        Serial.print("Flow Avg: ");
        Serial.print(flow_moving_avg);
        Serial.print("\t");
        Serial.print("Setpoint: ");
        Serial.print(air_setpoint);
        Serial.print("\t");
        Serial.print("Relative humidity: ");
        Serial.print(rel_humidity);
        Serial.print("\t");
        Serial.print("Ambient temperature: ");
        Serial.print(amb_temp);
        Serial.print("\t");
        if (airflow_stable){
            Serial.print("Stable");
        }else{Serial.print("Setting...");}
        Serial.print("\t");

        Serial.println(crc_error?" CRC error":"");
    }

    else{

        // T D TA FA F A S RH AT

        Serial.print("T");
        Serial.print(temperature);

        Serial.print("D");
        Serial.print(DUT_temp);

        Serial.print("TA");
        Serial.print(air_temp);

        Serial.print("FA");
        Serial.print(air_flow);

        Serial.print("F");
        Serial.print(flow);

        Serial.print("A");
        Serial.print(flow_moving_avg);

        Serial.print("S");
        Serial.print(air_setpoint);

        Serial.print("RH");
        Serial.print(rel_humidity);

        Serial.print("AT");
        Serial.print(amb_temp);

        if (airflow_stable){
            Serial.print("K");
        }else{Serial.print("N");}


        Serial.println(crc_error?" CRC error":"");
    }
}

