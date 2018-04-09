Are you tired of manually unlocking your LUKS drives? Are you wondering if you couldn't just store the LUKS key(file) somewhere *temporary*? Do you have some spare micro controllers? Then this might be what you are looking for!

UART based RAM banks (ubrb)
=
With this firmware you can store arbitary data in the RAM of a micro controller! The device needs to be accessible through a serial port (/dev/ttySOMETHING mostly on Linux) which is used for communication. While the library code can be used by anything there are two implementations for Arduino IDE for the Digispark and XMC2GO controllers.

API
-
The interface is rather simple:
* Cn    will clear bank n
* Gn    will return the data from bank n
* Sn <data> will write data to bank n

The new line character '\n' is used as a seperator of each command. `S0\n48656c6c6f0a\n` will store 'Hello' in bank zero. The data is simply encoded in hexadecimal to easily handle special characters (like '\n'). While this encoding doubles the size of the data, it can be super small implemented and does not require any buffering or state handling (like base64 does).

Usage example
-
```
sudo ./ubrb /dev/ttyACM0 C0
sudo ./ubrb /dev/ttyACM0 S0 `echo 'Hello' | xxd -p -c 64`
sudo ./ubrb /dev/ttyACM0 G0 | xxd -r -p                  
Hello
```

LUKS
-
The mkinitcpio folder contains hooks to unlock a root-partition and the systemd folder contains a service that sends the keyfile to the device.
