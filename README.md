# BLE_Mute_Switch
Use an "iTag" device and an ESP32 as a remote control button or switch.  Intended as a radar detector mute switch.

# iTag Device

The "iTag" is budget bluetooth locator tag similar to "Tile".  When on, the iTag provides a bluetooth low energy
server.  An external device can connect and reqeust that the tag beep, as a means of locating a lost item.  The
tag will also beep when the connection is dropped as a means to remind you that you're leaving an item behind.

The iTag also has a button.  When the button is pressed it will notify on a specific BLE Characteristic.  This is
often used as a shutter release or to activate a voice recorder on the host phone software.

These iTag style devices seem to be plentyful on Aliexpress.  Most operate similarly, but of the few I have some
are slightly different.  You might need to edit UUIDs or experiment more with the source code to get any 
specific iTag device to connect.

# Operation

The source code provided, once loaded on to the Mini32 will operate as follows:
- The device will search for known iTag devices.  During this search the LED will blink.
- If it find ones, it will connect.  The LED will go solid. 
- Once connected, a press of the iTag button will blink the LED and also send a pulse to the defined 
MUTE_PIN, in my case GPIO17. On GPIO17, I have connected the optoisolator circut.
- Upon disconnect it will go back into scanning mode.

One unfortunate side effect is when the iTag is connected and TTGO module is powered down.  The iTag takes this
to mean the tag is "gone out of range" and it will beep.  A button press will stop this, but I have found the
iTag stops beeping eventually.

# Components of this system

This was build from parts I had handly.

- A TTGO Mini32 or "TTGO T7" - provides a small ESP32 processor.  The ESP32 has both WiFi and BLE but both
can't really be used simultaneously due to memory constraints.
- A "Wemos DC Power Shield" - takes a 12v source and regulates it down to 5v.  The TTGO Mini32 has another
regulator to get down to the 3.3v operating voltage.
- A protoshiled with a PC817 opto isolator and resistor.  You might need a second resisor if you intend
to use this with a Valentine One.  You could probably easily do this with a transistor, relay or some other
switching method.

# Schematic

# License
I have released my portions of this project under GPL v3.  It is based on some sample code which I belive is
Licensed under Apache 2.0.

# Disclaimer

**Basically, use the materials in this repository at your own risk.**

BECAUSE THE PROGRAM & HARDWARE DESIGN IS LICENSED FREE OF CHARGE, THERE IS NO WARRANTY FOR THE PROGRAM & HARDWARE 
DESIGN, TO THE EXTENT PERMITTED BY APPLICABLE LAW. EXCEPT WHEN OTHERWISE STATED IN WRITING THE COPYRIGHT HOLDERS 
AND/OR OTHER PARTIES PROVIDE THE PROGRAM & HARDWARE DESIGN "AS IS" WITHOUT WARRANTY OF ANY KIND, EITHER EXPRESSED
OR IMPLIED, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A 
PARTICULAR PURPOSE. THE ENTIRE RISK AS TO THE QUALITY AND PERFORMANCE OF THE PROGRAM & HARDWARE DESIGN IS WITH 
YOU. SHOULD THE PROGRAM OR HARDWARE DESIGN PROVE DEFECTIVE, YOU ASSUME THE COST OF ALL NECESSARY SERVICING, 
REPAIR OR CORRECTION.

IN NO EVENT UNLESS REQUIRED BY APPLICABLE LAW OR AGREED TO IN WRITING WILL ANY COPYRIGHT HOLDER, OR ANY OTHER 
PARTY WHO MAY MODIFY AND/OR REDISTRIBUTE THE PROGRAM OR HARDWARE DESIGN AS PERMITTED ABOVE, BE LIABLE TO YOU 
FOR DAMAGES, INCLUDING ANY GENERAL, SPECIAL, INCIDENTAL OR CONSEQUENTIAL DAMAGES ARISING OUT OF THE USE OR 
INABILITY TO USE THE PROGRAM OR HARDWARE DESIGN (INCLUDING BUT NOT LIMITED TO LOSS OF DATA OR DATA BEING RENDERED
INACCURATE OR LOSSES SUSTAINED BY YOU OR THIRD PARTIES OR A FAILURE OF THE PROGRAM OR HARDWARE DESIGN TO OPERATE
WITH ANY OTHER PROGRAMS OR HARDWARE), EVEN IF SUCH HOLDER OR OTHER PARTY HAS BEEN ADVISED OF THE POSSIBILITY OF
SUCH DAMAGES.

