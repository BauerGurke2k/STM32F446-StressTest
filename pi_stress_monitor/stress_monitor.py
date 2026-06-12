# pi_stress_monitor/stress_monitor.py

import asyncio
import serial
import can
import threading
import time
import logging
import csv
from pathlib import Path
from typing import Optional, Dict, Any

import uvicorn
from fastapi import FastAPI, Request
from fastapi.responses import HTMLResponse
from fastapi.templating import Jinja2Templates

# --- Configuration ---
SERIAL_PORT = '/dev/ttyACM0'
SERIAL_BAUDRATE = 115200 # Baudrate is not critical for USB CDC, but must be set
CAN_INTERFACE = 'socketcan'
CAN_CHANNEL = 'can0'
CAN_BITRATE = 500000
CAN_REQUEST_ID = 0x200
CAN_RESPONSE_ID = 0x201
LOG_FILE = 'stress_test_log.csv'
LOG_INTERVAL_S = 1.0

# --- Global State ---
# This dictionary will be shared across threads and updated continuously.
# It's simple enough that we rely on Python's GIL for thread safety on atomic operations.
# For more complex updates, a lock would be needed.
app_state: Dict[str, Any] = {
    "start_time": time.time(),
    "test_uptime_str": "0h 0m 0s",
    "test_status": "STARTING",
    "last_log_time": 0,
    "usb_status": {
        "is_connected": False,
        "last_message_time": 0,
        "packets_received": 0,
        "packets_per_sec": 0,
        "last_seq": -1,
        "seq_errors": 0,
        "crc_errors": 0,
        "timeouts": 0,
    },
    "can_status": {
        "last_message_time": 0,
        "packets_received": 0,
        "packets_per_sec": 0,
        "requests_sent": 0,
        "responses_received": 0,
        "last_rtt_ms": None,
        "pending_request_time": None,
        "seq_errors": 0,
        "timeouts": 0,
    },
    "stm32_stats": {
        "uptime_ms": 0,
        "cpu_iter": 0,
        "can_tx": 0,
        "can_rx": 0,
        "can_err": 0,
        "i2c_errors": 0,
        "fram_crc_errors": 0,
        "usb_busy": 0,
        "can_busoff_count": 0, # From separate CAN message
    }
}

# --- Logging Setup ---
log_path = Path(LOG_FILE)
log_header = [
    'timestamp', 'uptime_s', 'test_status', 'usb_connected', 'usb_pps', 'usb_seq_err', 'usb_crc_err', 'usb_timeout',
    'can_pps', 'can_rtt_ms', 'can_seq_err', 'can_timeout', 'stm32_uptime_ms', 'stm32_can_busoff',
    'stm32_i2c_err', 'stm32_fram_err'
]
if not log_path.exists():
    with open(log_path, 'w', newline='') as f:
        writer = csv.writer(f)
        writer.writerow(log_header)

logging.basicConfig(level=logging.INFO, format='%(asctime)s - %(levelname)s - %(message)s')


# --- Helper Functions ---
def calculate_checksum(data: bytes) -> int:
    crc = 0
    for byte in data:
        crc ^= byte
    return crc

def update_test_status():
    """Defines the PASS/WARN/FAIL logic."""
    s = app_state # Shorthand
    if not s["usb_status"]["is_connected"] or s["can_status"]["timeouts"] > 5 or s["stm32_stats"]["can_busoff_count"] > 0 or s["stm32_stats"]["fram_crc_errors"] > 0:
        s["test_status"] = "FAIL"
    elif s["usb_status"]["seq_errors"] > 0 or s["usb_status"]["crc_errors"] > 0 or s["can_status"]["seq_errors"] > 0 or s["stm32_stats"]["i2c_errors"] > 0:
        s["test_status"] = "WARN"
    else:
        s["test_status"] = "PASS"


# --- Serial Communication Thread ---
def serial_reader_thread():
    logging.info(f"Starting serial reader for {SERIAL_PORT}")
    usb = app_state["usb_status"]
    stm32 = app_state["stm32_stats"]
    
    while True:
        try:
            with serial.Serial(SERIAL_PORT, SERIAL_BAUDRATE, timeout=1) as ser:
                logging.info(f"Serial port {SERIAL_PORT} opened successfully.")
                usb["is_connected"] = True
                last_packet_count = 0
                last_rate_calc_time = time.time()

                sio = serial.io.TextIOWrapper(serial.BufferedReader(ser), newline='\n')
                
                while usb["is_connected"]:
                    line = sio.readline()
                    if not line:
                        usb["timeouts"] += 1
                        logging.warning("USB read timeout.")
                        if time.time() - usb["last_message_time"] > 5:
                            logging.error("USB disconnected.")
                            usb["is_connected"] = False
                        continue
                    
                    usb["last_message_time"] = time.time()

                    try:
                        line = line.strip()
                        if not line.startswith("USBSTAT"):
                            continue
                        
                        parts = line.split(',')
                        # Format: USBSTAT,seq,uptime_ms,cpu_iter,can_tx,can_rx,can_err,i2c_err,fram_crc_err,usb_busy,crc
                        if len(parts) != 11:
                            continue

                        # Checksum verification
                        payload = ",".join(parts[:-1]).encode('ascii')
                        received_crc = int(parts[-1])
                        calculated_crc = calculate_checksum(payload)
                        
                        if received_crc != calculated_crc:
                            usb["crc_errors"] += 1
                            logging.warning(f"USB CRC error! Got {received_crc}, expected {calculated_crc}")
                            continue

                        usb["packets_received"] += 1

                        # Sequence check
                        seq = int(parts[1])
                        if usb["last_seq"] != -1 and seq != (usb["last_seq"] + 1):
                            usb["seq_errors"] += 1
                            logging.warning(f"USB sequence error! Got {seq}, expected {usb['last_seq'] + 1}")
                        usb["last_seq"] = seq

                        # Update STM32 stats
                        stm32["uptime_ms"] = int(parts[2])
                        stm32["cpu_iter"] = int(parts[3])
                        stm32["can_tx"] = int(parts[4])
                        stm32["can_rx"] = int(parts[5])
                        stm32["can_err"] = int(parts[6])
                        stm32["i2c_errors"] = int(parts[7])
                        stm32["fram_crc_errors"] = int(parts[8])
                        stm32["usb_busy"] = int(parts[9])

                        # Calculate rate
                        now = time.time()
                        if now - last_rate_calc_time >= 1.0:
                            usb["packets_per_sec"] = (usb["packets_received"] - last_packet_count) / (now - last_rate_calc_time)
                            last_packet_count = usb["packets_received"]
                            last_rate_calc_time = now

                    except (ValueError, IndexError) as e:
                        logging.error(f"Error parsing USB message: {line} -> {e}")

        except serial.SerialException as e:
            if usb["is_connected"]:
                logging.error(f"Serial port error: {e}")
                usb["is_connected"] = False
                usb["packets_per_sec"] = 0
            time.sleep(2) # Wait before trying to reconnect


# --- CAN Communication ---
def can_reader_thread():
    logging.info("Starting CAN reader thread.")
    can_state = app_state["can_status"]
    stm32 = app_state["stm32_stats"]
    last_packet_count = 0
    last_rate_calc_time = time.time()

    with can.Bus(interface=CAN_INTERFACE, channel=CAN_CHANNEL, bitrate=CAN_BITRATE) as bus:
        while True:
            msg = bus.recv(timeout=1.0)
            if msg:
                can_state["last_message_time"] = time.time()
                can_state["packets_received"] += 1

                if msg.arbitration_id == CAN_RESPONSE_ID and can_state["pending_request_time"]:
                    rtt = time.time() - can_state["pending_request_time"]
                    can_state["last_rtt_ms"] = rtt * 1000
                    can_state["pending_request_time"] = None
                    can_state["responses_received"] += 1

                elif msg.arbitration_id == 0x102: # Error Counter
                    stm32["can_busoff_count"] = msg.data[2]
                
                # Calculate rate
                now = time.time()
                if now - last_rate_calc_time >= 1.0:
                    can_state["packets_per_sec"] = (can_state["packets_received"] - last_packet_count) / (now - last_rate_calc_time)
                    last_packet_count = can_state["packets_received"]
                    last_rate_calc_time = now
            else:
                logging.warning("CAN read timeout.")
                if can_state["pending_request_time"] and (time.time() - can_state["pending_request_time"] > 2.0):
                     can_state["timeouts"] += 1
                     logging.error("CAN request timed out.")
                     can_state["pending_request_time"] = None


async def can_writer_task():
    logging.info("Starting CAN writer task.")
    can_state = app_state["can_status"]
    seq = 0
    with can.Bus(interface=CAN_INTERFACE, channel=CAN_CHANNEL, bitrate=CAN_BITRATE) as bus:
        while True:
            # Avoid sending a new request if one is pending
            if can_state["pending_request_time"] is None:
                msg = can.Message(arbitration_id=CAN_REQUEST_ID, data=[seq], is_extended_id=False)
                try:
                    bus.send(msg)
                    can_state["requests_sent"] += 1
                    can_state["pending_request_time"] = time.time()
                    seq = (seq + 1) % 256
                except can.CanError:
                    logging.error("Failed to send CAN message.")
            await asyncio.sleep(0.5) # Send request every 500ms


# --- Main Application & Web Server ---
app = FastAPI()
templates = Jinja2Templates(directory="templates")

@app.get("/", response_class=HTMLResponse)
async def read_root(request: Request):
    return templates.TemplateResponse("index.html", {"request": request})

@app.get("/api/status")
async def get_status():
    return app_state


async def main_loop():
    logging.info("Starting main application loop.")
    while True:
        # Update uptime string
        uptime_seconds = int(time.time() - app_state["start_time"])
        hours, remainder = divmod(uptime_seconds, 3600)
        minutes, seconds = divmod(remainder, 60)
        app_state["test_uptime_str"] = f"{hours}h {minutes}m {seconds}s"
        
        # Update overall status
        update_test_status()

        # Log to CSV
        if time.time() - app_state["last_log_time"] >= LOG_INTERVAL_S:
            app_state["last_log_time"] = time.time()
            with open(log_path, 'a', newline='') as f:
                writer = csv.writer(f)
                writer.writerow([
                    f"{time.time():.2f}",
                    uptime_seconds,
                    app_state["test_status"],
                    app_state["usb_status"]["is_connected"],
                    f"{app_state['usb_status']['packets_per_sec']:.2f}",
                    app_state["usb_status"]["seq_errors"],
                    app_state["usb_status"]["crc_errors"],
                    app_state["usb_status"]["timeouts"],
                    f"{app_state['can_status']['packets_per_sec']:.2f}",
                    f"{app_state['can_status']['last_rtt_ms']:.2f}" if app_state['can_status']['last_rtt_ms'] else -1,
                    app_state["can_status"]["seq_errors"],
                    app_state["can_status"]["timeouts"],
                    app_state["stm32_stats"]["uptime_ms"],
                    app_state["stm32_stats"]["can_busoff_count"],
                    app_state["stm32_stats"]["i2c_errors"],
                    app_state["stm32_stats"]["fram_crc_errors"],
                ])

        await asyncio.sleep(0.5)


if __name__ == "__main__":
    # Start background threads for blocking IO
    threading.Thread(target=serial_reader_thread, daemon=True).start()
    threading.Thread(target=can_reader_thread, daemon=True).start()

    # Setup asyncio event loop
    loop = asyncio.get_event_loop()
    
    # Run FastAPI server in a separate thread
    config = uvicorn.Config(app, host="0.0.0.0", port=8000, loop="asyncio")
    server = uvicorn.Server(config)
    
    async def run_server_and_tasks():
        # Start the CAN writer task
        writer_task = loop.create_task(can_writer_task())
        # Start the main logic loop
        main_task = loop.create_task(main_loop())
        # Run the server
        await server.serve()
        # Await tasks to finish if server stops
        await writer_task
        await main_task

    loop.run_until_complete(run_server_and_tasks())
