# Zx Programmable Interface

**ZPI** is the interface used by z80 programs to communicate with the Pico world.

# Port 3 Services:
## NMI emulator (W):
 - write value 14 (0b00001110)

`out 3,14` will tell IFp to assert `/NMI` line which will bring the IFp menu.

## Memory (R/W):

0b1Yxxxxxx - 8k memory paged transfer.
 - Y: 1 sending an 8k page from z80 to pico
 - Y: 0 receiving an 8k page from pico to z80
 - xxxxxx: is the page number (up to 64 pages)

How to send a page from z80 to pico:
```
addr = 0x4000;  // RAM (screen) start address
out 3, 0b1100'0000 // it's our first 8k RAM page
for (i = 0; i < 8*1024; ++i)
    out 3, addr[i]
```

How to receive a page from pico to z80:
```
addr = 0x4000;  // RAM
out 3, 0b1000'0000 // it's our first 8k RAM
wait until (in 3  != 0b1000'0000) // first byte must be the same as the command sent

for (i = 0; i < 8*1024; ++i)
    addr[i] = in 3;
```

## Screen (R/W):

0b01Y11111 - entire screen transfer
 - Y: 1 sending an entire screen
 - Y: 0 receiving an entire screen

How to send an entire screen from z80 to pico:
```
addr = 0x4000;  // screen start address
out 3, 0b0111'1111 // save screen command
for (i = 0; i < 6144 + 768; ++i)
    out 3, addr[i]
```

How to receive an entire screen from pico to z80:
```
addr = 0x4000;  // screen start address
out 3, 0b0101'1111 // receive screen command
wait until (in 3  != 0b0101'1111) // first byte must be the same as the command sent

for (i = 0; i < 6144 + 768; ++i)
    addr[i] = in 3
```

TODO: block transfer

## B4/BDOS(CP/M 3.14):
B4 is an extension to CP/M 3.14 BDOS Commands

### B4/BDOS common requests and responses:
#### BL
Block transfer will transfer the following bytes:
0x0000 - SL Size
0x0001 - XX bytes

#### BHL
Block transfer will transfer the following bytes:
0x0000 - SH Size (high byte)
0x0001 - SL Size (low byte)
0x0002 - XX bytes

#### AB
Address Block will send the following bytes:
- 0x0000 - AH Address (high byte)
- 0x0001 - AL Address (low byte)
- 0x0002 - SH Size (high byte)
- 0x0003 - SL Size (low byte)
- 0x0004 - XX bytes

#### JAB
Jmp to Address Block will send the following bytes:
- 0x0000 - AH Address (high byte)
- 0x0001 - AL Address (low byte)
- 0x0002 - SH Size (high byte)
- 0x0003 - SL Size (low byte)
- 0x0004 - XX bytes
- 0xXXXX - JH Jump Address (high byte)
- 0xYYYY - JL Jump Address (low byte)


0b0000'0010 - initiale B4/BDOS function call:

First we have the CPM 2.2/3 functions:

- 0x00 System reset function, fetch b4 gui
    * Send: None
    * Receive: `JAB`

- 0x01 Console Input:
    * Send: None
    * Receive: byte - ASCII Character

- 0x02 Console Output:
    * Send: byte - ASCII Character
    * Receive: None

- 0x03 Auxilary Input:
    * Send: None
    * Receive: byte - ASCII Character

- 0x04 Auxilary Output:
    * Send: byte - ASCII Character
    * Receive: None

- 0x05 List Output:
    * Send: byte - ASCII Character
    * Receive: None

- 0x06 Direct Console I/O
    * Send: byte - 0xFF or ASCII Character
    * Receive: byte - ASCII Character or status

- 0x07 Get I/O Byte
    * Send: None
    * Receive: I/O Byte Value

- 0x08 Set I/O Byte
    * Send: I/O Byte Value
    * Receive: None

- 0x09 Print String
    * Send: `BL` block
    * Receive: None

- 0x0a Read Console Buffer
    * Send: None
    * Receive: `BL` block

- 0x0b Get Console Status
    * Send: None
    * Receive: byte - console status

- 0x0c Return Version Number
    * Send: None
    * Receive: HHLL version number

- 0x0d Reset Disk System
    * Send: None
    * Receive: None

- 0x0e Select Disk
    * Send: byte (disk number)
    * Receive: None

- 0x0f OpenFile
    * Send: `BL` FCB Data Block
    * Receive: byte - directory code; `BL` updated FCB

- 0x10 CloseFile
    * Send: `BL` FCB Data Block
    * Receive: byte - directory code

- 0x11 Search for First
    * Send: `BL` FCB Data Block
    * Receive: byte - directory code; `BL` updated FCB

- 0x12 Search for Next
    * Send: `BL` FCB Data Block
    * Receive: byte - directory code; `BL` updated FCB

- 0x13 Delete file
    * Send: `BL` FCB Data Block
    * Receive: byte - directory code

- 0x14 Read Sequential
    * Send: `BL` FCB Data Block
    * Receive: byte - directory code; `BL` data

- 0x15 Write Sequential
    * Send: `BL` FCB Data Block; `BL` data
    * Receive: byte - directory code

- 0x16 Make file
    * Send: `BL` FCB Data Block
    * Receive: byte - directory code; `BL` updated FCB

- 0x17 Rename file
    * Send: `BL` FCB Data Block
    * Receive: byte - directory code; `BL` updated FCB

- 0x18 Return Log-in Vector
    * Send: None
    * Receive: HHLL Log-in Vector

- 0x19 Return Current Disk
    * Send: None
    * Receive: byte - disk number

- 0x1a Set DMA Address
    * Send: None
    * Receive: None

- 0x1b Get Addr (ALLOC)
    * Send: None
    * Receive: `BL` allocation vector

- 0x1c Write Protect
    * Send: None
    * Receive: None

- 0x1d Get Read Only Vector
    * Send: None
    * Receive: HHLL Read Only Vector

- 0x1e Set File Attributes
    * Send: `BL` FCB Data Block
    * Receive: byte - directory code; `BL` updated FCB

- 0x1f Get Addr (DISKPARMS)
    * Send: None
    * Receive: `BL` - DPB disk parameters

- 0x20 Set/Get User Code
    * Send: byte - 0FFH (get) or User Code (set)
    * Receive: byte - Current Code (get) or 0 (no value)

- 0x21 Read Random
    * Send: `BL` FCB Data Block
    * Receive: byte - Return Code; `BL` data

- 0x22 Write Random
    * Send: `BL` FCB Data Block; `BL` data
    * Receive: byte - Return Code

- 0x23 Compute File Size
    * Send: `BL` FCB Data Block
    * Receive: B0B1B2 File Size

- 0x24 Set Random Record
    * Send: `BL` FCB Data Block
    * Receive: byte - Return Code

- 0x25 Reset Drive
    * Send: `BL` Drive Vector
    * Receive: None

- 0x28 Write Random with Zero Fill
    * Send: `BL` FCB Data Block; `BL` data
    * Receive: byte - Return Code