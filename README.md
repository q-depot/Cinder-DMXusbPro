# Cinder-DMX

A library for DMX communication over USB serial using an Enttec USB Pro box (called a 'widget' in Enttec’s documentation).

Based on the MIT-licensed [Cinder-DMXUsbPro](https://github.com/q-depot/Cinder-DMXusbPro) by Nocte Studio Ltd.

## What it does

The Enttec hardware receives commands over serial and translates them into a stream of DMX commands for your lighting hardware.

This library eases connecting to and sending commands to the Enttec hardware.

Using the block requires three steps:

1. Connect to a physical device by constructing a `dmx::EnttecDevice`.
2. Start the virtual device’s data loop `device->startLoop()`.
3. Pass buffers of data to your virtual device `device->bufferData(buffer)`.

Have a look at the DMXBasicApp sample to see these commands in context. Note that it will run even if you aren’t able to connect to a device.

Serial message delivery is handled on a secondary thread, with the `bufferData` method providing a threadsafe interface to update the data sent to the DMX controller. Querying the DMX hardware’s basic parameters is also supported (though responses from the device occasionally stall). We can attempt to set the hardware’s parameters, but sending a message matching the device documentation doesn’t appear to work. We may try adjusting that message in the future.

## What is DMX

If you are curious about what is leaving the DMX serial box, read up a bit on [DMX](https://en.wikipedia.org/wiki/DMX512). Make sure that your installation is using proper cabling and a terminator at the final device.
