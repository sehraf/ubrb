```
sudo ./ubrb /dev/ttyACM0 C0
sudo ./ubrb /dev/ttyACM0 S0 `echo 'Hello' | xxd -p -c 64`
sudo ./ubrb /dev/ttyACM0 G0 | xxd -r -p                  
Hello
```
