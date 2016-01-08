# Cinder-DMX

A library for DMX communication over USB serial using an Enttec USB Pro box (called a 'widget' in Enttec’s documentation).

Based on the MIT-licensed [Cinder-DMXUsbPro](https://github.com/q-depot/Cinder-DMXusbPro) by Nocte Studio Ltd.

## What it does

The Enttec device receives commands over serial and translates them into a stream of DMX commands for your lighting hardware.

This library simplifies connecting to and sending commands to the Enttec device. Regular DMX packet delivery is handled on a secondary thread, with a threadsafe `bufferData` method for providing complete information packets. Querying and setting the DMX hardware’s basic parameters is also supported.

Have a look at the DMXBasicApp sample to get an idea for how to use the block.

## What is DMX

If you are curious about what is leaving the DMX serial box, read up a bit on [DMX](https://en.wikipedia.org/wiki/DMX512). Make sure that your installation is using proper cabling and a terminator at the final device.
