import time
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

GAIN_CMDS = {
    "NORM": "GAIN_SWITCH 0",
    "ORIDE": "GAIN_SWITCH 1",
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
        print("3. Send GAIN_SWITCH command")
        print("4. Exit")
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
            print("Available GAIN_SWITCH values:")
            for k in GAIN_CMDS: print(f"  - {k}")
            val = input("Enter GAIN mode: ").strip().upper()
            if val in GAIN_CMDS:
                if val == "ORIDE":
                    send_udp("GAIN_SWITCH_COVER 1")
                    time.sleep(0.100)  # 50 ms 
                    send_udp(GAIN_CMDS[val])
                elif val == "NORM":
                    send_udp(GAIN_CMDS[val])
                    time.sleep(0.100)  # 50 ms 
                    send_udp("GAIN_SWITCH_COVER 0")
            else:
                print("Invalid GAIN option.")
        elif choice == "4":
            break
        else:
            print("Invalid option.")

if __name__ == "__main__":
    main()
