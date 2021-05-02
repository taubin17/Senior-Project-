import pandas as pd
from pathlib import Path
import socket
import struct
import sys
import os
import websocket_helper
import webbrowser

# ESP8266 password.
passwd = "tyler123"

# Treat this remote directory as a root for file transfers
SANDBOX = ""

# Common HTTP flags to use to talk to ESP8266.
WEBREPL_REQ_S = "<2sBBQLH64s"
WEBREPL_PUT_FILE = 1
WEBREPL_GET_FILE = 2
WEBREPL_GET_VER  = 3


class test:
    def __init__(self):
        # By default, scores are skewed to be insanely high, so that no risk of returning wrong result if test data
        # can not be obtained
        self.filename = ''
        self.baseline_humidity = 1e2
        self.baseline_temperature = 1e6
        self.humidity_score = 1e6
        self.temperature_score = 1e6
        self.score = self.humidity_score + self.temperature_score

    def to_string(self):
        print()
        print(f'Test named: {self.filename} RESULTS')
        print(f"HUMIDITY SCORE: {self.humidity_score} ||| TEMPERATURE SCORE: {self.temperature_score}")
        print(f'TOTAL SCORE: {self.score}')

        return

# Debug function provided by webclient.py from ESP8266 webrepl repo
def debugmsg(msg):
    if DEBUG:
        print(msg)


# Websocket class with methods to talk to ESP8266
class websocket:

    def __init__(self, s):
        self.s = s
        self.buf = b""

    def write(self, data):
        l = len(data)
        if l < 126:
            # TODO: hardcoded "binary" type
            hdr = struct.pack(">BB", 0x82, l)
        else:
            hdr = struct.pack(">BBH", 0x82, 126, l)
        self.s.send(hdr)
        self.s.send(data)

    def recvexactly(self, sz):
        res = b""
        while sz:
            data = self.s.recv(sz)
            if not data:
                break
            res += data
            sz -= len(data)
        return res

    def read(self, size, text_ok=False):
        if not self.buf:
            while True:
                hdr = self.recvexactly(2)
                assert len(hdr) == 2
                fl, sz = struct.unpack(">BB", hdr)
                if sz == 126:
                    hdr = self.recvexactly(2)
                    assert len(hdr) == 2
                    (sz,) = struct.unpack(">H", hdr)
                if fl == 0x82:
                    break
                if text_ok and fl == 0x81:
                    break
                # debugmsg("Got unexpected websocket record of type %x, skipping it" % fl)
                while sz:
                    skip = self.s.recv(sz)
                    # debugmsg("Skip data: %s" % skip)
                    sz -= len(skip)
            data = self.recvexactly(sz)
            assert len(data) == sz
            self.buf = data

        d = self.buf[:size]
        self.buf = self.buf[size:]
        assert len(d) == size, len(d)
        return d

    def ioctl(self, req, val):
        assert req == 9 and val == 2


def login(ws, passwd):
    while True:
        c = ws.read(1, text_ok=True)
        if c == b":":
            assert ws.read(1, text_ok=True) == b" "
            break
    ws.write(passwd.encode("utf-8") + b"\r")


def read_resp(ws):
    data = ws.read(4)
    sig, code = struct.unpack("<2sH", data)
    assert sig == b"WB"
    return code


def send_req(ws, op, sz=0, fname=b""):
    rec = struct.pack(WEBREPL_REQ_S, b"WA", op, 0, 0, sz, len(fname), fname)
    debugmsg("%r %d" % (rec, len(rec)))
    ws.write(rec)


def get_ver(ws):
    send_req(ws, WEBREPL_GET_VER)
    d = ws.read(3)
    d = struct.unpack("<BBB", d)
    return d


def put_file(ws, local_file, remote_file):
    sz = os.stat(local_file)[6]
    dest_fname = (SANDBOX + remote_file).encode("utf-8")
    rec = struct.pack(WEBREPL_REQ_S, b"WA", WEBREPL_PUT_FILE, 0, 0, sz, len(dest_fname), dest_fname)
    debugmsg("%r %d" % (rec, len(rec)))
    ws.write(rec[:10])
    ws.write(rec[10:])
    assert read_resp(ws) == 0
    cnt = 0
    with open(local_file, "rb") as f:
        while True:
            sys.stdout.write("Sent %d of %d bytes\r" % (cnt, sz))
            sys.stdout.flush()
            buf = f.read(1024)
            if not buf:
                break
            ws.write(buf)
            cnt += len(buf)
    print()
    assert read_resp(ws) == 0


# Gets a file (remote file) from the ESP8266 and copies it to a local file on the machine
def get_file(ws, local_file, local_file_path, remote_file):
    # File to be grabbed from ESP8266. Put in byte form to be passed in header for GET request
    src_fname = (SANDBOX + remote_file).encode("utf-8")

    # Header for the GET request
    rec = struct.pack(WEBREPL_REQ_S, b"WA", WEBREPL_GET_FILE, 0, 0, 0, len(src_fname), src_fname)
    # debugmsg("%r %d" % (rec, len(rec)))

    # Send our request
    ws.write(rec)

    # And ensure the request was read
    assert read_resp(ws) == 0

    # If the path doesnt exist, make it
    if not os.path.exists(local_file_path):
        os.makedirs(local_file_path)

    with open(local_file_path / local_file, "w") as f:
        cnt = 0
        while True:
            ws.write(b"\0")
            (sz,) = struct.unpack("<H", ws.read(2))
            if sz == 0:
                break
            while sz:
                buf = ws.read(sz)
                if not buf:
                    raise OSError()
                cnt += len(buf)
                f.write(buf.decode('utf-8'))
                sz -= len(buf)
                sys.stdout.write("Received %d bytes\r" % cnt)
                sys.stdout.flush()
    print()
    assert read_resp(ws) == 0


# Taken from ESP8266 webclient.py, just a simple help window.
# Not useful for this project, but could be vital to someone who uses this project as a basis for their own
def help(rc=0):
    exename = sys.argv[0].rsplit("/", 1)[-1]
    print("%s - Perform remote file operations using MicroPython WebREPL protocol" % exename)
    print("Arguments:")
    print("  [-p password] <host>:<remote_file> <local_file> - Copy remote file to local file")
    print("  [-p password] <local_file> <host>:<remote_file> - Copy local file to remote file")
    print("Examples:")
    print("  %s script.py 192.168.4.1:/another_name.py" % exename)
    print("  %s script.py 192.168.4.1:/app/" % exename)
    print("  %s -p password 192.168.4.1:/app/script.py ." % exename)
    sys.exit(rc)


# Taken from ESP8266 webclient.py, helps debug issues with websocket interface
def error(msg):
    print(msg)
    sys.exit(1)


# Taken from ESP8266 webclient.py, not used
def parse_remote(remote):
    host, fname = remote.rsplit(":", 1)
    if fname == "":
        fname = "/"
    port = 8266
    if ":" in host:
        host, port = host.split(":")
        port = int(port)
    return (host, port, fname)


# Opens the test as a pandas dataframe to make getting sensor data easy
def get_current_test(filename, current_test_path):
    path_to_read = current_test_path / filename
    data = pd.read_csv(path_to_read)
    return data

# Function takes a list of humidity data, and returns a score for that humidity data
def get_humidity_score(humidity_data, baseline_humidity):

    score = 0

    # Add all the humidities together
    for each_humidity in humidity_data:
        score += each_humidity

    # Then divide by the number of samples taken
    score /= (len(humidity_data))

    # Subtract the baseline humidity, and then divide by the baseline humidity. This gets the percentage increase
    # of humidity over the baseline, which is a rough indicator for how effective a mask is
    score -= baseline_humidity
    score /= baseline_humidity

    # Score represents how much fluctuation between the baseline versus readings with mask on. By returning the inverse
    # a meaningful score where higher is better is returned
    return 1 / score


def get_temperature_score(temperature_data, baseline_temperature):

    score = 0

    # Add all the humidities together
    for each_humidity in temperature_data:
        score += each_humidity

    # Then divide by the number of samples taken
    score /= (len(temperature_data))

    # Subtract the baseline humidity, and then divide by the baseline humidity. This gets the percentage increase
    # of humidity over the baseline, which is a rough indicator for how effective a mask is
    score -= baseline_temperature
    score /= baseline_temperature

    # Score represents how much fluctuation between the baseline versus readings with mask on. By returning the inverse
    # a meaningful score where higher is better is returned. Also, due to room temperature dramatically changing temperature results
    # temperature will have a significantly smaller weight to final score than humidity
    return (1 / score) / 50


def get_mask_effectivity(filename):
    return 0


def get_test_results(filename, current_test, current_test_path):
    data = get_current_test(filename, current_test_path)

    # Separate humidity and temperature data
    temperature_data = data["TEMPERATURE"]
    humidity_data = data[" HUMIDITY"]

    # Convert Pandas dataframes to lists
    humidities = humidity_data.tolist()
    temperatures = temperature_data.tolist()

    # Now use algorithm to compare humidity and temperature results
    humidity_score = get_humidity_score(humidities, current_test.baseline_humidity)
    temperature_score = get_temperature_score(temperatures, current_test.baseline_temperature)

    return humidity_score, temperature_score

def get_test_baseline(filename, current_test_path):
    data = get_current_test(filename, current_test_path)

    # Separate humidity and temperature data
    temperature_data = data["TEMPERATURE"]
    humidity_data = data[" HUMIDITY"]

    # Convert Pandas dataframes to lists
    humidities = humidity_data.tolist()
    temperatures = temperature_data.tolist()

    baseline_humidity = humidities[-1]
    baseline_temperature = temperatures[-1]

    return baseline_humidity, baseline_temperature


def get_latest_test_score(current_test_path, baseline_data_path):
    # Create a new socket
    s = socket.socket()

    current_test = test()

    # Pass ESP8266 address and port. Defaults are given below
    ai = socket.getaddrinfo('192.168.4.1', 8266)
    address = ai[0][4]

    s.connect(address)

    # Creates websocket handshake with ESP8266
    websocket_helper.client_handshake(s)

    # Creates an empty websocket object to later interface the ESP8266
    ws = websocket(s)

    # Logs into the ESP8266
    login(ws, passwd)

    # Opens the ESP8266 up to get requests
    ws.ioctl(9, 2)

    # Filename for the copied data from ESP8266
    data_file = 'current_test.csv'
    baseline_file = 'baseline_data.csv'

    current_test.filename = data_file

    # Get the current_test.csv from the ESP8266
    get_file(ws,  data_file, current_test_path, 'current_test.csv')
    get_file(ws, baseline_file, baseline_data_path, 'baseline_data.csv')

    # Save our test scores to the current test
    current_test.baseline_humidity, current_test.baseline_temperature \
        = get_test_baseline(baseline_data_path / baseline_file, baseline_data_path)
    current_test.humidity_score, current_test.temperature_score \
        = get_test_results(current_test_path / data_file, current_test, current_test_path)

    # Generate overall score from previous 2 scores
    current_test.score = current_test.humidity_score + current_test.temperature_score

    # For testing purposes only
    # webbrowser.open('file:///C:/Users/taubi/OneDrive/Documents/ECE406/webrepl-master/webrepl-master/webrepl.html')

    # Close the connection to the ESP8266
    s.close()
    return current_test
