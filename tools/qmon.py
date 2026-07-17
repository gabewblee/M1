#!/usr/bin/env python3
import socket
import sys
import time

KEYMAP = {
    " ": "spc", "/": "slash", ".": "dot", "-": "minus", "_": "shift-minus",
    "\n": "ret", ",": "comma", "'": "apostrophe", "=": "equal", ";": "semicolon",
    ">": "shift-dot", "<": "shift-comma", "?": "shift-slash", ":": "shift-semicolon",
}


def send(sock, command):
    sock.sendall((command + "\n").encode())
    time.sleep(0.05)


def type_text(sock, text):
    for ch in text:
        if ch in KEYMAP:
            key = KEYMAP[ch]
        elif ch.isupper():
            key = "shift-" + ch.lower()
        else:
            key = ch
        send(sock, "sendkey " + key)


def main():
    sock = socket.socket(socket.AF_UNIX, socket.SOCK_STREAM)
    sock.connect(sys.argv[1])
    sock.settimeout(1)
    time.sleep(0.3)
    for arg in sys.argv[2:]:
        if arg.startswith("type:"):
            type_text(sock, arg[5:])
        elif arg.startswith("sleep:"):
            time.sleep(float(arg[6:]))
        else:
            send(sock, arg)
    time.sleep(0.5)
    try:
        while sock.recv(4096):
            pass
    except socket.timeout:
        pass
    sock.close()


if __name__ == "__main__":
    main()
