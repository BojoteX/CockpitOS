import sys

REQUIRED_MODULES = {
    "hid":    "hidapi",
    "filelock": "filelock",
    "tkinter": None,  # handled specially
}

missing = []

# Check each module
for mod, pip_name in REQUIRED_MODULES.items():
    if mod == "tkinter":
        try:
            import tkinter
        except ImportError:
            missing.append("tkinter")
    else:
        try:
            __import__(mod)
        except ImportError:
            missing.append(pip_name if pip_name else mod)

if missing:
    # Build pip install command
    pip_cmd = "pip install " + " ".join(m for m in missing if m != "tkinter")
    msg =  "Some required modules are missing:\n\n"
    for mod in missing:
        if mod == "tkinter":
            msg += "- tkinter (usually installable via system package manager)\n"
        else:
            msg += f"- {mod}\n"
    if pip_cmd.strip() != "pip install":
        msg += f"\nTo install the missing Python modules, run:\n\n    {pip_cmd}\n"
    msg += "\nAfter installing, restart this program."
    # Try Tkinter popup
    try:
        import tkinter as tk
        from tkinter import scrolledtext

        root = tk.Tk()
        root.title("Missing Required Modules")
        root.geometry("560x320")
        root.resizable(False, False)
        lbl = tk.Label(root, text="Missing required modules for CockpitController HID Handler:", font=("Arial", 12, "bold"), pady=10)
        lbl.pack()
        text = scrolledtext.ScrolledText(root, width=68, height=10, font=("Consolas", 10))
        text.pack(padx=12, pady=(0,10))
        text.insert("1.0", msg)
        text.config(state='normal')
        text.focus()
        def close():
            root.destroy()
            sys.exit(1)
        btn = tk.Button(root, text="Close", command=close, width=18)
        btn.pack(pady=10)
        root.protocol("WM_DELETE_WINDOW", close)
        root.mainloop()
    except Exception:
        # Fallback to console error
        print(msg)
    sys.exit(1)

import os
from filelock import FileLock, Timeout

LOCKFILE = os.path.join(os.path.dirname(os.path.abspath(__file__)), "cockpitos_dashboard.lock")

try:
    lock = FileLock(LOCKFILE + ".lock")
    lock.acquire(timeout=0.1)
except Timeout:
    print("ERROR: Another instance of CockpitController HID Handler is already running.")
    sys.exit(1)

import tkinter as tk
os.chdir(os.path.dirname(os.path.abspath(__file__)))

from gui import CockpitGUI
from network import NetworkManager
from config import reply_addr

def main():
    root = tk.Tk()
    # Instantiate GUI with a placeholder for network_mgr for now
    gui = CockpitGUI(root, None)
    # Create the network manager with a reference to reply_addr and the GUI's get_devices method
    net = NetworkManager(gui.uiq, reply_addr, gui.get_devices)
    # Link the network manager to the GUI so device_reader_wrapper can access it
    gui.network_mgr = net
    net.start()
    root.mainloop()

if __name__ == "__main__":
    main()
