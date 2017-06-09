Introduction
------------

Spiguy is a standalone Linux command-line tool that can be used for sending
single SPI transactions (using an activated spidev device) and viewing the
result.

Features
--------

At the time of this writing, Spiguy supports the following features:

1. Performs a single full-duplex transaction.
1. Input and output is on stdin/stdout.
2. Input and output is raw binary by default, but can also encode/decode from hex and decimal.
3. SPI bus speed and operating mode (CPOL/CPHA) is controllable.
4. a GPIO chip-select line can optionally be controlled.

Current Status
--------------

At the time of this writing, Spiguy should compile cleanly and has no known bugs.
