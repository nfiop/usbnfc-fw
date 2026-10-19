#!/usr/bin/env python3
import argparse
import time

import serial
import cbor2
from cobs import cobs


def encode_packet(message: dict) -> bytes:
    """CBOR -> COBS -> zero delimiter."""
    payload = cbor2.dumps(message)
    return cobs.encode(payload) + b"\x00"


def read_packet(ser: serial.Serial) -> dict:
    """Read one zero-delimited COBS frame and decode its CBOR."""
    frame = ser.read_until(b"\x00")

    if not frame or not frame.endswith(b"\x00"):
        raise TimeoutError("Timed out waiting for response")

    payload = cobs.decode(frame[:-1])
    message = cbor2.loads(payload)

    if not isinstance(message, dict):
        raise ValueError("Response is not a CBOR map")

    return message


def send_command(ser, seq: int, cmd: str, arg: int = 0) -> dict:
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
    parser.add_argument("port", help="Serial port, e.g. COM5 or /dev/ttyACM0")
    parser.add_argument("--baud", type=int, default=115200)
    parser.add_argument("--timeout", type=float, default=2.0)
    args = parser.parse_args()

    seq = 1

    # timeout applies to reads; write_timeout prevents indefinite writes.
    with serial.Serial(
        args.port,
        args.baud,
        timeout=args.timeout,
        write_timeout=args.timeout,
    ) as ser:
        # Allow USB CDC to settle after opening the port.
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
            except (TimeoutError, ValueError, OSError) as exc:
                print(f"Communication error: {exc}")
            finally:
                seq = (seq + 1) & 0x7FFFFFFFFFFFFFFF


if __name__ == "__main__":
    main()
