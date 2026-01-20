import time
import socket

DCS_IP = "192.168.7.166"
DCS_PORT = 7778

BATTERY_CMDS = {
    "OFF":   "BATTERY_SW 1",
    "ON":    "BATTERY_SW 2",
    "ORIDE": "BATTERY_SW 0",
}

LTDR_CMDS = {
    "SAFE":   "LTD_R_SW 0",
    "ARM":    "LTD_R_SW 1",
    "INC":    "LTD_R_SW INC",
    "DEC":    "LTD_R_SW DEC",
    "TOGGLE": "LTD_R_SW TOGGLE",
}

GAIN_CMDS = {
    "NORM":  "GAIN_SWITCH 0",
    "ORIDE": "GAIN_SWITCH 1",
}

HOOK_CMDS = {
    "DOWN":  "HOOK_LEVER 0",
    "UP": "HOOK_LEVER 1",
}

def send_udp(cmd: str) -> None:
    with socket.socket(socket.AF_INET, socket.SOCK_DGRAM) as sock:
        sock.sendto((cmd + "\n").encode(), (DCS_IP, DCS_PORT))
        print(f"[SENT] {cmd}")

def numbered_choice(title: str, options: dict) -> str | None:
    """
    Show a 1..N menu for `options` (keys as labels).
    Returns the selected key, or None to go back.
    Accepts either the number or the exact key (case-insensitive).
    """
    keys = list(options.keys())  # preserves insertion order (Py 3.7+)
    print(f"\n== {title} ==")
    for i, k in enumerate(keys, 1):
        print(f"{i}. {k}")
    print("0. Back")

    choice = input("Select: ").strip()

    # numeric path
    if choice.isdigit():
        n = int(choice)
        if n == 0:
            return None
        if 1 <= n <= len(keys):
            return keys[n - 1]
        print("Invalid selection.")
        return None

    # name path
    key = choice.upper()
    if key in options:
        return key

    print("Invalid selection.")
    return None

def main():
    while True:
        print("\n=== Command Menu ===")
        print("1. BATTERY_SW")
        print("2. LTD/R")
        print("3. GAIN_SWITCH")
        print("4. HOOK")
        print("5. Exit")
        top = input("Select an option: ").strip()

        if top == "1":
            sel = numbered_choice("BATTERY_SW", BATTERY_CMDS)
            if sel:
                send_udp(BATTERY_CMDS[sel])

        elif top == "2":
            sel = numbered_choice("LTD/R", LTDR_CMDS)
            if sel:
                send_udp(LTDR_CMDS[sel])

        elif top == "3":
            sel = numbered_choice("GAIN_SWITCH", GAIN_CMDS)
            if sel:
                if sel == "ORIDE":
                    send_udp("GAIN_SWITCH_COVER 1")
                    time.sleep(0.100)  # 100 ms
                    send_udp(GAIN_CMDS[sel])
                elif sel == "NORM":
                    send_udp(GAIN_CMDS[sel])
                    time.sleep(0.100)  # 100 ms
                    send_udp("GAIN_SWITCH_COVER 0")
        elif top == "4":
            sel = numbered_choice("HOOK_LEVER", HOOK_CMDS)
            if sel:
                if sel == "DOWN":
                    send_udp("HOOK_LEVER 1")
                elif sel == "UP":
                    send_udp("HOOK_LEVER 0")

        elif top == "5":
            break
        else:
            print("Invalid option.")

if __name__ == "__main__":
    main()
