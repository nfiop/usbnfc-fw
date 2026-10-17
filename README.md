# USB NAND flash controller firmware

`usbnfc-fw` is a project that aims to provide a cross platform firmware
which turns a set of popular MCUs to act as a USB NAND flash controller.

## Goals

The projects goals are to:
 - Provide a unified communication mechanism between an host machine
   and a USB NAND flash controller, for the sake of connecting to a NAND
   flash chip, with proper support within the Linux kernel and the MTD
   subsystem.

 - Create an extensible framework for different I/O busses and digital
   interfaces to NAND flash chips, supporting:
   - Bitbanging via raw GPIO I/Os
   - Fast raw parallel connection (e.g. RP2040 PIOs)
   - A specific MCU flash controller (e.g. STM32 FMC)
   - Bitbanging via a JTAG boundary-scan register
   - A Fast SPI master-slave bus

   The framework should expose configuration knobs, related to NAND
   timings, voltages and dynamic assignment of I/O, where applicable.

   The framework should encapsulate platform-specific details in a set
   of known and recognized files, and keep the rest of the project as
   agnostic as possible to these details.

 - Standardize a configuration mechanism for a variety of use-cases 
   related to NAND flash chips, via a known AT-command set.

## Planned features

These are the planned features for now; others could be added in as well
where needed -
   - Support for MCUs like the RP2040, STM32H5

   - Support for the [custom USB NAND flash controller](https://github.com/nfiop/custom-usb-nfc-adapter) project.

   - Support for NAND drivers - JTAG BSR, raw GPIO, RP2040 PIOs, MCU
     flash memory controller (like those in STM32 chips), SPI bus

   - Voltage domain & NAND timings control, when platform allows.

   - Basic ECC support - mainly software Hamming & BCH, and hardware ECC
     (for Hamming and BCH) when platform allows.
