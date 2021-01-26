// Whitebox Interactive EZO Console
// https://www.whiteboxes.ch/
//
// Tool to help you setup Atlas Scientific EZO devices
//
// THIS CODE IS NOT INTENDED AS A BOILERPLATE FOR YOUR PROJECT.
//
// USAGE:
//---------------------------------------------------------------------------------------------
// - Set host serial terminal to 9600 baud
// - To open a I2C address (between 1 - 127), enter the number of the address - e.g. 99 ENTER
// - To issue a command, enter it directly to the console.
// - type "!help" to see available commands
//---------------------------------------------------------------------------------------------


#include <Wire.h>                        //enable I2C.
#include <errno.h>

enum errors {SUCCESS, FAIL, NOT_READY, NO_DATA };
enum states {REPL_READ_STATE, REPL_EVAL_STATE, REPL_PRINT_STATE, POLLING_READ_STATE};

char computer_data[20];                 // A 20 byte character array to hold incoming data from a pc/mac/other.
byte computer_bytes_count = 0;          // We need to know how many characters bytes have been received
int computer_in_byte;
boolean computer_msg_complete = false;

const int EZO_ANSWER_LENGTH = 32;
char ezo_answer[EZO_ANSWER_LENGTH];     // A 32 byte character array to hold incoming data from the sensors
int ezo_address = 0;                    // INT pointer for channel switching - 0-7 serial, 8-127 I2C addresses
String ezo_type;                        // hold the name / type of the EZO device
char* ezo_version;                      // hold the version of the EZO device

errors i2c_error;                       // error-byte to store result of Wire.transmissionEnd()
states state;
unsigned long next_receive_time;


void setup() {

  Serial.begin(9600);                    // Set the hardware serial port to 9600
  while (!Serial) ;                      // Leonardo-type arduinos need this to be able to write to the serial port in setup()
  Wire.begin();                          // enable I2C port.

  ezo_type.reserve(40);                  // reserve string buffer to save some SRAM

  intro();                               // display startup message
  state = REPL_READ_STATE;
}


void loop() {

  switch (state) {
    case REPL_READ_STATE:

      read_console();
      break;

    case REPL_EVAL_STATE:

      eval_command();
      break;

    case REPL_PRINT_STATE:

      receive_answer();
      break;

    case POLLING_READ_STATE:

      polling_read();
      break;
  }
}


// loop REPL_READ_STATE
void read_console() {
  while (Serial.available() > 0) {              // While there is serial data from the computer

    computer_in_byte = Serial.read();           // read a byte

    if (computer_in_byte == '\n' || computer_in_byte == '\r') {      // if a newline character arrives, we assume a complete command has been received
      computer_data[computer_bytes_count] = 0;
      computer_msg_complete = true;
      computer_bytes_count = 0;
    } else {                                    // command not complete yet, so just ad the byte to data array
      computer_data[computer_bytes_count] = computer_in_byte;
      computer_bytes_count++;
    }
  }

  if (computer_msg_complete) {                  // if there is a complete command from the computer
    state = REPL_EVAL_STATE;
    computer_msg_complete = false;              //Reset the var computer_bytes_count to equal 0
  }
}


// loop REPL_EVAL_STATE
void eval_command() {

  char *cmd = computer_data;
  Serial.print(ezo_address);
  Serial.print(F("> "));                       // echo to the serial console to see what we typed
  Serial.println(cmd);

  if (cmd[0] == 0 ) {                          // catch <ENTER>-only "command". not to be forwarded to the EZO device.
    state = REPL_READ_STATE;
    return;
  }
  else if (strcmp(cmd, "!help") == 0) {        // !help command
    help();
    computer_msg_complete = false;
    state = REPL_READ_STATE;
    return;
  }
  else if (strcmp(cmd, "!scan") == 0) {        // !scan command
    scan();
    computer_msg_complete = false;
    state = REPL_READ_STATE;
    return;
  }
  else if (strcmp(cmd, "!poll") == 0) {       // !poll command
    ezo_send_command("r");
    next_receive_time = millis() + 1000;
    state = POLLING_READ_STATE;
    return;
  }

  else {                                       // it's not a console command, so probaby it's a channel number or a command for the ezo device


    errno = 0;                                 // reset errno (errno.h) - it will be used by strtol below
    char *endptr = NULL;
    int _maybe_i2c_address = strtol(cmd, &endptr, 10); // strtol()
    
    // strtol sets endptr = input, if no digits are found to be converted
    if (cmd != endptr && errno == 0) {                  // check for errno - to distinguish between "0" input and \0 (error)

      if (_maybe_i2c_address >= 1 && _maybe_i2c_address <= 127) {
        if (set_active_ezo(_maybe_i2c_address)) {       // set I2C address

          serialPrintDivider();
          Serial.println( F("ACTIVE DEVICE: "));
          Serial.println( ezo_type );
          Serial.print( F("Address: ") );
          Serial.print( ezo_address );
          Serial.print( F(" | Firmware: "));
          Serial.println( ezo_version);
        }
        else {
          Serial.println(F("CHANNEL NOT AVAILABLE"));
          Serial.println(F("Try '!scan'."));
        }
        serialPrintDivider();
        state = REPL_READ_STATE;
        return;
        
      } else if (_maybe_i2c_address == 0) {           // don't try to set active ezo to 0 - set it back to "broadcast"
        ezo_address = 0;
        state = REPL_READ_STATE;
        return;
      }
    }

    // it's not a channel number. so let's forward this as a command to the EZO device.
    state = REPL_PRINT_STATE;

    if (strncmp(cmd, "Baud,", 5) == 0 || strncmp(cmd, "Serial,", 7) == 0) {
      Serial.println(F("! You have changed the EZO device to UART - it can no longer be accessed by this console. "));
    }

    ezo_send_command(cmd);

    if (cmd[0] == 'r' || cmd[0] == 'R' || strncmp ( cmd, "Cal", 3 ) == 0 || strncmp ( cmd, "cal", 3 ) == 0) { // it's a "R" or "Cal" command
      next_receive_time = millis() + 1000;
    } else {
      next_receive_time = millis() + 300;
    }
  }
}


// loop  REPL_PRINT_STATE
void receive_answer() {
  if (millis() > next_receive_time) {

    ezo_receive_command();

    Serial.print(ezo_address);
    Serial.print(F("> "));

    switch (i2c_error) {
      case SUCCESS:
        Serial.println(ezo_answer);
        break;

      case FAIL:
        Serial.println(F("COMMAND FAILED"));
        break;

      case NOT_READY:
        Serial.println(F("EZO DEVICE NOT READY"));
        break;

      case NO_DATA:
        Serial.println(F("NO DATA"));
        break;
    }

    state = REPL_READ_STATE;
  }
}


// loop POLLING_READ_STATE
void polling_read() {
  if (millis() > next_receive_time) {
    ezo_receive_command();
    if (i2c_error == 0) {
      Serial.print(ezo_address);
      Serial.print(F("> "));
      Serial.println(ezo_answer);
    }

    if (Serial.available() > 0) {
      state = REPL_READ_STATE;
    } else {
      ezo_send_command("r");
      next_receive_time = millis() + 1000;
    }
  }
}


//function controls which UART/I2C port is opened. returns true if channel could be changed.
boolean set_active_ezo(int _new_active_address) {
  ezo_address = _new_active_address;
  ezo_send_command("I");
  delay(400);
  ezo_receive_command();

  if (parseInfo()) {

    return true;
  }
  return false;
}


// parses the answer of the "i" command. returns true if answer was parseable, false if not.
boolean parseInfo() {

  char* _token = strtok(ezo_answer, ",");
  char* _type;

  if (_token != NULL) {

    _type = strtok(NULL, ",");
    //Serial.println(_type);

    ezo_version = strtok(NULL, ",");
    //Serial.println(ezo_version);

    if (strcmp(_type, "pH") == 0) {
      ezo_type = F("EZO pH Circuit");
    }
    else if (strcmp(_type, "ORP") == 0 || strcmp(_type, "OR") == 0) {
      ezo_type = F("EZO ORP Circuit");
    }
    else if (strcmp(_type, "DO") == 0) {
      ezo_type = F("EZO Dissolved Oxygen Circuit");
    }
    else if (strcmp(_type, "EC") == 0) {
      ezo_type = F("EZO Conductivity Circuit");
    }
    else if (strcmp(_type, "RTD") == 0) {
      ezo_type = F("EZO RTD Circuit");
    }
    else if (strcmp(_type, "FLO") == 0) {
      ezo_type = F("EZO FLOW - Embedded Flow Meter Totalizer");
    }
    else if (strcmp(_type, "CO2") == 0) {
      ezo_type = F("EZO CO2 - Embedded NDIR CO2 Sensor");
    }
    else if (strcmp(_type, "O2") == 0) {
      ezo_type = F("EZO O2 - Embedded Oxygen Sensor");
    }
    else if (strcmp(_type, "HUM") == 0) {
      ezo_type = F("EZO-HUM - Embedded Humidity sensor");
    }
    else if (strcmp(_type, "PRS") == 0) {
      ezo_type = F("EZO-PRS - Embedded Pressure Sensor");
    }
    else if (strcmp(_type, "PMP") == 0) {
      ezo_type = F("EZO-PMP - Embedded Dosing Pump");
    }
    else if (strcmp(_type, "RGB") == 0) {
      ezo_type = F("EZO-RGB - Embedded Color Sensor");
    }

    else {
      ezo_type = F("UNKNOWN EZO DEVICE");
    }

    return true;

  } else {

    // it's a legacy device (non-EZO) or something else (might be from the future)
    ezo_type = "UNKNOWN DEVICE";

    return false;                              // can not parse this info-string
  }
}


// scan the i2c bus for EZO devices
int scan() {

  int _ezo_address_before = ezo_address;       // remember active ezo device to return to after scanning
  int _ezo_count = 0;
  byte _i2c_error;
  int _address;

  serialPrintDivider();

  for (_address = 1; _address <= 127; _address++ ) {

    // "probing" the address - if result is '0', a device is present
    Wire.beginTransmission(_address);
    _i2c_error = Wire.endTransmission();

    if (_i2c_error == 0) {

      if (set_active_ezo(_address)) {

        _ezo_count++;
        Serial.print(  ezo_address);
        Serial.print(": ");
        Serial.println(  ezo_type);
      }
    }
  }

  if (_ezo_address_before == 0) {             // set the active ezo device back to what it was before the scan
    ezo_address = 0;
  } else {
    set_active_ezo(_ezo_address_before);
  }

  serialPrintDivider();

  Serial.print(    _ezo_count);
  Serial.println(  F(" EZO devices found"));

  return _ezo_count;
}


//print intro
void intro() {

  Serial.println( F("\n\n"));
  serialPrintDivider();
  Serial.println( F("Whitebox Interactive EZO Console"));

  if (scan() > 0) {
    Serial.println( F("\nType an I2C address to connect the console to an EZO deivce (1-127)"));
  }
  Serial.println( F("For info type '!help'"));
  Serial.println( F("\n"));
}


//print help dialogue
void help() {
  serialPrintDivider();
  Serial.println( F("To connect to an attached EZO device, type its address (1-127) followed by ENTER"));
  Serial.println( F("To send an I2C command to the EZO device, enter it directly to the console. e.g. 'r<ENTER>'"));
  Serial.println( F("You can find all available I2C commands in the datasheet of your EZO device."));
  Serial.println( F("Available console commands:"));
  Serial.println( F("!scan     lists all attached EZO devices"));
  Serial.println( F("!poll     polls the 'read' command of the current EZO device every second"));
  Serial.println( F("          to cancel polling, send any command or <ENTER>"));
  Serial.println( F("!help     this information"));
  serialPrintDivider();
}


void serialPrintDivider() {
  Serial.println(  F("------------------"));
}


// send a command to an EZO device
void ezo_send_command(const char* command) {
  Wire.beginTransmission(ezo_address);
  Wire.write(command);
  Wire.endTransmission();
}


// request answer from an EZO device
void ezo_receive_command() {
  byte sensor_bytes_received = 0;
  byte code = 0;
  byte in_char = 0;

  memset(ezo_answer, 0, EZO_ANSWER_LENGTH);                   // clear sensordata array;

  Wire.requestFrom(ezo_address, EZO_ANSWER_LENGTH - 1, 1);
  code = Wire.read();

  while (Wire.available()) {
    in_char = Wire.read();

    if (in_char == 0) {
      break;
    }
    else {
      ezo_answer[sensor_bytes_received] = in_char;
      sensor_bytes_received++;
    }
  }

  switch (code) {
    case 1:
      i2c_error = SUCCESS;
      break;

    case 2:
      i2c_error = FAIL;
      break;

    case 254:
      i2c_error = NOT_READY;
      break;

    case 255:
      i2c_error = NO_DATA;
      break;
  }
}
