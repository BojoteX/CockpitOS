import socket

DCS_IP = "192.168.7.166"  # Update this if needed
DCS_PORT = 7778

BATTERY_CMDS = {
    "OFF":  "BATTERY_SW 1",
    "ON":   "BATTERY_SW 2",
    "ORIDE": "BATTERY_SW 0",
}

PITOT_CMDS = {
    "AUTO": "PITOT_HEAT_SW 0",
    "ON":   "PITOT_HEAT_SW 1",
}


def send_udp(cmd):
    with socket.socket(socket.AF_INET, socket.SOCK_DGRAM) as sock:
        sock.sendto((cmd + "\n").encode(), (DCS_IP, DCS_PORT))
        print(f"[SENT] {cmd}")


def main():
    while True:
        print("\n=== Command Menu ===")
        print("1. Send BATTERY_SW command")
        print("2. Send PITOT_HEAT_SW command")
        print("3. Exit")
        choice = input("Select an option: ").strip()

        if choice == "1":
            print("Available BATTERY_SW values:")
            for k in BATTERY_CMDS: print(f"  - {k}")
            val = input("Enter BATTERY mode: ").strip().upper()
            if val in BATTERY_CMDS:
                send_udp(BATTERY_CMDS[val])
            else:
                print("Invalid BATTERY option.")
        elif choice == "2":
            print("Available PITOT_HEAT_SW values:")
            for k in PITOT_CMDS: print(f"  - {k}")
            val = input("Enter PITOT mode: ").strip().upper()
            if val in PITOT_CMDS:
                send_udp(PITOT_CMDS[val])
            else:
                print("Invalid PITOT option.")
        elif choice == "3":
            break
        else:
            print("Invalid option.")


if __name__ == "__main__":
    main()
