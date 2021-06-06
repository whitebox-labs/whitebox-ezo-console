# Whitebox Interactive Console for EZO
An interactive console (REPL) on Arduino for Atlas Scientific EZO devices. Use the Arduino IDE Serial Monitor (or any other Serial Console) to setup your Atlas Scientific EZO devices. 

## Usage
Using the console you identifiy and list available EZO devices and send commands to the EZO device. See the EZO device's datasheet for a full list of commands.

**All EZO device must be in I2C mode**


### Select the active EZO device
Type the I2C address of the device, followed by <ENTER> - e.g. `99<ENTER>`
Once connected, you can send any I2C command the EZO device accepts - e.g. `r<ENTER>` to read the current value.
  
### Console Commands
Console commands start with an '!' and are not sent to the EZO device but are used as tools in the console:

#### !help
show help information
#### !scan
scan and list available EZO devices
#### !poll
continuously polls the read-command from the active EZO device. Any key or command will cancel the polling.


## Whitebox Shields Compatibility
* Whitebox T1 MkII `T1.21`: fully compatible
* Whitebox T2 MkII `T2.21` SMA: fully compatible
* Whitebox T2 MkII `T2.21` BNC: fully compatible
* _(Legacy)_ Tentacle Shield `T1.16`: fully compatible if used in I2C-only mode
* _(Legacy)_ Tentacle Mini `T2.16`: fully compatible

## Atlas Scientific EZO Compatibility
* EZO pH Circuit
* EZO Conductivity Circuit
* EZO ORP Circuit
* EZO Dissolved Oxygen Circuit
* EZO RTD Circuit
* EZO FLOW - Embedded Flow Meter Totalizer
* EZO CO2 - Embedded NDIR CO2 Sensor
* EZO O2 - Embedded Oxygen Sensor
* EZO HUM - Embedded Humidity sensor
* EZO PRS - Embedded Pressure Sensor
* EZO PMP - Embedded Dosing Pump
* EZO RGB - Embedded Color Sensor
