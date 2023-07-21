
void setup() {
  // put your setup code here, to run once:

    Serial.begin(9600);
    Serial1.begin(9600, SERIAL_8N1);

    // if thermal air left in remote mode after switching off
    // needs to be in local mode before re-entering remote mode
    // delays added as switching modes is not instantaneous 
    send_command("%GL");
    delay(5000);
    send_command("%RM");
    delay(5000);

    /// turn on thermal air compressor and main nozzle air flow
    

}

void loop() { //-----------------Main loop------------------//
 
  // send_command("TEMP?");
  // delay(1000);
  // String output = read_output();
  // Serial.print("Temp: ");
  // Serial.print(output);

  while (Serial.available() > 0)
  {
    String command = Serial.readString();
    
    send_command(command);
    delay(1000);
    String output = read_output();
    Serial.print(command);
    Serial.println(output);
  }


} //------------------end main loop-------------------//

void send_command(String command)
{
  Serial1.print(command);
  Serial1.print("\r\n");
  return;
}

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
