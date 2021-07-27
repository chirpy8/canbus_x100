Prototype solution to ‘spoof’ the XKR Mercedes TCM (EGS51 for the W5A580/722.6/NAG1 transmission) in the 4.0 XKR to change the rear axle ratio.
Of course reprogramming the EGS51 TCM would be much better, but there is no readily available developer info I can find to create the appropriate EEPROM profile, and not much interest anymore it seems in this old unit.

This technique might work for other transmissions/TCMs besides the 722.6 (e.g. ZF), or it might not…..

**Disclaimer**

This is not an industrial/automotive grade solution, it is not a commercial or production solution, use at your own risk, not for on-road use, prototype solution for private testing only, no warranty expressed or implied.

**Overview**

The solution is based on an Arduino driven canbus repeater to intercept the messages between the TCM and the rest of the canbus (ECU, ABS, INST).
Two MCP2515 based canbus modules are used, one facing the TCM side of the bus, and the other facing the ECU/ABS side of the bus.
Since the TCM module is conveniently located next to the the ECU in the 2002 XKR, it is easy to snip the canbus wires at the TCM connection, and bridge in the repeater module.
The repeater module receives the messages on the TCM side, and sends them onwards on the ECU/ABS side, and vice versa.
The ABS wheel speed messages towards the TCM are intercepted, and the wheel speed values for all 4 wheels are scaled appropriately so that the TCM is not aware of the different axle ratio, and then sent on.

The code is based on interrupts triggered when a message is received on either canbus. The Arduino communicates with the canbus modules using SPI interface. The interrupt service routines (ISR), one for each direction, read the new message, and then send it back out on the other bus. The ABS facing one also checks for the ABS wheel bus message ID (SID 0x4B0) on ABS side of the bus, and if found it executes the code to scale the speeds. The wheel speeds are big endian 2 byte. Testing to date is limited to a 3.27 axle with 3.06 TCM, but the code includes estimates for the factors for other combinations. The spreadsheet “axle ratio change algorithms” allows computation for factors not listed in the code.

To reduce PCB design efforts, a base PCB design mounts modules that can be purchased on eBay or elsewhere, which saves time and eliminates the need to assemble a board using surface mount components. The compromise is obviously that the solution is bigger.

The design can mount up to 3 canbus modules, the idea being that the third module could be used for off-board telemetry and control. Current testing has only used 2 modules. By not adding the third module, there are convenient connections to add an ICSP connector to program the Arduino, which is used for this specific project.

More details in the docs folder

