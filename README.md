# Cinder-DMX

A library for sending DMX to various devices in Cinder. The scenarios of light command from Cinder this block supports are as follows:

### DMX communication over USB serial with Enttec USB Pro.
The Enttec hardware receives commands over serial and translates them into a stream of DMX commands for your lighting hardware.

	 Using the block requires three steps:


   1. Connect to a physical device by constructing a `dmx::EnttecDevice`.
   2. Start the virtual device’s data loop `device->startLoop()`.
   3. Pass buffers of data to your virtual device `device->bufferData(buffer)`.

Have a look at the DMXBasicApp sample to see these commands in context. Note that it will run even if you aren’t able to connect to a device.

Serial message delivery is handled on a secondary thread, with the `bufferData` method providing a threadsafe interface to update the data sent to the DMX controller. Querying the DMX hardware’s basic parameters is also supported (though responses from the device occasionally stall). We can attempt to set the hardware’s parameters, but sending a message matching the device documentation doesn’t appear to work. We may try adjusting that message in the future.

If you are curious about what is leaving the DMX serial box, read up a bit on [DMX](https://en.wikipedia.org/wiki/DMX512). Make sure that your installation is using proper cabling and a terminator at the final device.

### KiNet v2

The KiNet v2 protocol is essentially a DMX array underneath an internet protocol and a Philips-specific header containing KiNet port and version information. It is used in conjunction with their newer lightning control hardware.

Power supply models we have used that accept KiNet v2 :

[sPDS-480](https://www.colorkinetics.com/support/datasheets/sPDS-480ca_ProductGuide.pdf)

[sPDS-60ca](https://www.colorkinetics.com/support/datasheets/sPDS60ca_24v_ProductGuide.pdf)

### KiNet v1
The KiNet v1 protocol is the same as KiNet v2 with a slightly different header. This Philips protocol to communicate to older power supplies.

Power supply models we have used that accept KiNet v1 :

[PDS-70mr 24V power supply](https://www.colorkinetics.com/ls/pds/pds70mr/)

### Streaming ACN
sACN (architecture for control networks) is a standard for controlling entertainment equipment, such as lighting, over an IP protocol. It is essentially a DMX frame wrapped in an ACN-specific header. There is a wealth of documentation of sACN. To learn more about it, visit [this page](https://www.rdmprotocol.org/files/What_Comes_After_Streaming_DMX_over_ACN_%20(4).pdf).