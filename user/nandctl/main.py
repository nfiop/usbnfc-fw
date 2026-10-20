#!/usr/bin/env python3

# SPDX-License-Identifier: MIT
# Copyright (c) 2026 Liav A

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
    """CRC-32/ISO-HDLC, compatible with the RP2040 firmware."""
    return zlib.crc32(data) & 0xFFFFFFFF

def encode_packet(message: dict) -> bytes:
    """CBOR -> little-endian CRC-32 -> COBS -> zero delimiter."""
    payload = cbor2.dumps(message)

    if len(payload) > MAX_CBOR:
        raise ValueError(f"CBOR payload too large: {len(payload)} bytes")

    crc = crc32_iso_hdlc(payload)
    raw = payload + crc.to_bytes(CRC_SIZE, "little")

    return cobs.encode(raw) + b"\x00"

def read_packet(ser: serial.Serial) -> dict:
    """Read frames until a valid CRC-protected CBOR map arrives."""
    deadline = time.monotonic() + ser.timeout
    last_error = None

    while time.monotonic() < deadline:
        ser.timeout = max(0.001, deadline - time.monotonic())
        frame = ser.read_until(b"\x00")

        if not frame:
            continue

        if not frame.endswith(b"\x00"):
            last_error = "Incomplete frame (timeout before delimiter)"
            continue

        if frame == b"\x00":
            continue

        try:
            raw = cobs.decode(frame[:-1])
        except (ValueError, TypeError) as exc:
            last_error = f"COBS error: {exc}"
            continue

        if len(raw) < CRC_SIZE:
            last_error = "Frame too short for CRC"
            continue

        if len(raw) > MAX_RAW_FRAME:
            last_error = "Frame exceeds maximum size"
            continue

        payload = raw[:-CRC_SIZE]
        received_crc = int.from_bytes(raw[-CRC_SIZE:], "little")
        expected_crc = crc32_iso_hdlc(payload)

        if received_crc != expected_crc:
            last_error = (
                f"CRC mismatch (received 0x{received_crc:08X}, "
                f"expected 0x{expected_crc:08X})"
            )
            continue

        try:
            message = cbor2.loads(payload)
        except Exception as exc:
            last_error = f"CBOR error: {exc}"
            continue

        if not isinstance(message, dict):
            last_error = "Response is not a CBOR map"
            continue

        return message

    raise TimeoutError(
        "No valid response before timeout"
        + (f"; last error: {last_error}" if last_error else "")
    )


def send_command(
    ser: serial.Serial,
    seq: int,
    cmd: str,
    arg: bytes = b"",
) -> dict:
    """Send a command with a binary argument."""
    if not isinstance(arg, bytes):
        raise TypeError("arg must be bytes")

    request = {"seq": seq, "cmd": cmd, "arg": arg}

    ser.write(encode_packet(request))
    ser.flush()

    response = read_packet(ser)

    if response.get("seq") != seq:
        raise ValueError(
            f"Sequence mismatch: sent {seq}, "
            f"received {response.get('seq')}"
        )

    return response


def main():
    parser = argparse.ArgumentParser(
        description="CBOR/COBS/CRC-32 command client"
    )
    parser.add_argument("port", help="COM5 or /dev/ttyACM0")
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
        time.sleep(0.2)

        print("Commands:")
        print("  ping")
        print("  hex <command> <hex-bytes>")
        print("  led 0")
        print("  led 1")
        print("  quit")
        print()
        print("Example: hex set_rgb FF 64 00")

        while True:
            try:
                line = input("> ").strip()
            except (EOFError, KeyboardInterrupt):
                break

            if not line:
                continue

            if line.lower() == "quit":
                break

            parts = line.split()

            if parts[0].lower() == "ping" and len(parts) == 1:
                cmd, arg = "ping", b""

            elif (
                len(parts) == 2
                and parts[0].lower() == "led"
                and parts[1] in ("0", "1")
            ):
                cmd = "led_set"
                arg = bytes([int(parts[1])])

            elif len(parts) >= 2 and parts[0].lower() == "hex":
                cmd = parts[1]

                try:
                    # Accept: hex command FF 64 00
                    # Also accepts: hex command FF6400
                    hex_text = "".join(parts[2:]).replace(" ", "")
                    arg = bytes.fromhex(hex_text)
                except ValueError as exc:
                    print(f"Invalid hex argument: {exc}")
                    continue

            else:
                print("Unknown input")
                continue

            try:
                response = send_command(ser, seq, cmd, arg)
                print(
                    f"seq={seq} {response['rc']}: "
                    f"{response.get('ret_buf')!r}"
                )
            except (
                TimeoutError,
                ValueError,
                TypeError,
                OSError,
                serial.SerialException,
            ) as exc:
                print(f"Communication error: {exc}")
            finally:
                seq = (seq + 1) & 0x7FFFFFFFFFFFFFFF

if __name__ == "__main__":
    main()