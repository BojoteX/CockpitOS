import configparser
import os
import time
import threading

DEFAULT_REPORT_SIZE = 64
DEFAULT_MULTICAST_IP = "239.255.50.10"
DEFAULT_UDP_PORT = 5010
HANDSHAKE_REQ = b"DCSBIOS-HANDSHAKE"
FEATURE_REPORT_ID = 0
IDLE_TIMEOUT = 2.0
MAX_DEVICES = 20
LOCKFILE = "cockpitos_dashboard.lock"

def read_vid_pid_from_ini(filename="settings.ini"):
    config = configparser.ConfigParser()
    # If the file doesn't exist, create it with defaults
    if not os.path.isfile(filename):
        config['USB'] = {'VID': '0xCAFE', 'PID': '0xCAF3'}
        with open(filename, 'w') as configfile:
            config.write(configfile)
    config.read(filename)
    try:
        vid = int(config['USB']['VID'], 0)
        pid = int(config['USB']['PID'], 0)
    except Exception:
        vid, pid = 0xCAFE, 0xCAF3
    return vid, pid

# Read VID & PID from settings.ini or set defaults if not present
USB_VID, USB_PID = read_vid_pid_from_ini()

stats = {
    "frame_count_total": 0,
    "frame_count_window": 0,
    "bytes": 0,
    "start_time": time.time()
}
global_stats_lock = threading.Lock()


# UDP global state (must be accessed with care)
reply_addr = [None]  # As a mutable list!

# Per-device reconnection tracking (must be global/shared)
prev_reconnections = {}