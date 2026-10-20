# SPDX-License-Identifier: MIT
# Copyright (c) 2026 Liav A

#!/usr/bin/env python3
import argparse
import time

import zlib
import serial
import cbor2
from cobs import cobs

CRC_SIZE = 4
MAX_CBOR = 128
MAX_RAW_FRAME = MAX_CBOR + CRC_SIZE

def crc32_iso_hdlc(data: bytes) -> int:
    """CRC-32/ISO-HDLC, matching the framing protocol."""
    return zlib.crc32(data) & 0xFFFFFFFF

def encode_packet(message: dict) -> bytes:
    """CBOR -> append little-endian CRC-32 -> COBS -> zero delimiter."""
    payload = cbor2.dumps(message)

    if len(payload) > MAX_CBOR:
        raise ValueError(
            f"CBOR payload too large: {len(payload)} > {MAX_CBOR}"
        )

    crc = crc32_iso_hdlc(payload)
    raw = payload + crc.to_bytes(CRC_SIZE, byteorder="little")

    return cobs.encode(raw) + b"\x00"

def read_packet(ser: serial.Serial) -> dict:
    """
    Read zero-delimited frames until a valid CRC-protected CBOR map
    is received or the serial read timeout expires.
    """
    deadline = time.monotonic() + ser.timeout
    last_error = None

    while time.monotonic() < deadline:
        remaining = deadline - time.monotonic()
        ser.timeout = max(0.001, remaining)

        frame = ser.read_until(b"\x00")

        if not frame:
            continue

        if not frame.endswith(b"\x00"):
            # Partial frame: the read timed out before its delimiter.
            # Discard it; the next call starts with whatever remains
            # in the serial input buffer.
            last_error = "Timed out waiting for frame delimiter"
            continue

        encoded = frame[:-1]

        if not encoded:
            continue

        try:
            raw = cobs.decode(encoded)
        except (ValueError, TypeError) as exc:
            last_error = f"Invalid COBS frame: {exc}"
            continue

        if len(raw) < CRC_SIZE:
            last_error = "Frame too short to contain CRC-32"
            continue

        if len(raw) > MAX_RAW_FRAME:
            last_error = f"Frame too large: {len(raw)} bytes"
            continue

        payload = raw[:-CRC_SIZE]
        received_crc = int.from_bytes(raw[-CRC_SIZE:], "little")
        calculated_crc = crc32_iso_hdlc(payload)

        if received_crc != calculated_crc:
            last_error = (
                f"CRC mismatch: received 0x{received_crc:08X}, "
                f"calculated 0x{calculated_crc:08X}"
            )
            continue

        try:
            message = cbor2.loads(payload)
        except Exception as exc:
            last_error = f"Invalid CBOR payload: {exc}"
            continue

        if not isinstance(message, dict):
            last_error = "Response is not a CBOR map"
            continue

        return message

    raise TimeoutError(
        "No valid response received before timeout"
        + (f" (last error: {last_error})" if last_error else "")
    )

def send_command(
    ser: serial.Serial,
    seq: int,
    cmd: str,
    arg: int = 0,
) -> dict:
    request = {"seq": seq, "cmd": cmd, "arg": arg}

    ser.write(encode_packet(request))
    ser.flush()

    response = read_packet(ser)

    if response.get("seq") != seq:
        raise ValueError(
            f"Sequence mismatch: sent {seq}, "
            f"received {response.get('seq')}"
        )

    status = response.get("status")
    if status not in ("ACK", "NACK"):
        raise ValueError(f"Invalid response status: {status!r}")

    return response

def main():
    parser = argparse.ArgumentParser()
    parser.add_argument(
        "port",
        help="Serial port, e.g. COM5 or /dev/ttyACM0",
    )
    parser.add_argument("--baud", type=int, default=115200)
    parser.add_argument("--timeout", type=float, default=2.0)
    args = parser.parse_args()

    seq = 1

    with serial.Serial(
        args.port,
        args.baud,
        timeout=args.timeout,
        write_timeout=args.timeout,
    ) as ser:
        # Allow USB CDC or the serial bridge to settle.
        time.sleep(0.2)

        print("Commands: ping, led 0, led 1, quit")

        while True:
            try:
                line = input("> ").strip().lower()
            except (EOFError, KeyboardInterrupt):
                break

            if line == "quit":
                break

            if line == "ping":
                cmd, arg = "ping", 0
            elif line in ("led 0", "led 1"):
                _, value = line.split()
                cmd, arg = "led_set", int(value)
            else:
                print("Unknown input")
                continue

            try:
                response = send_command(ser, seq, cmd, arg)
                print(
                    f"seq={seq} {response['status']}: "
                    f"{response.get('result')}"
                )
            except (
                TimeoutError,
                ValueError,
                OSError,
                serial.SerialException,
            ) as exc:
                print(f"Communication error: {exc}")
            finally:
                seq = (seq + 1) & 0x7FFFFFFFFFFFFFFF

if __name__ == "__main__":
    main()