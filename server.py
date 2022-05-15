import socket
import argparse
import sys

threshold = 60
dict_sensors = dict()

def recv(sock):
    data = sock.recv(1)
    message = b""
    while data.decode("utf-8") != '\n':
        message += data
        data = sock.recv(1)
    return message

def verify_slope(address):
    slope = sum(dict_sensors[address]) / 30
    return "OPEN" if slope > threshold else "CLOSE"

def process(data):
    result = ""
    if data.startswith("[SENSOR_VALUE]"):
        splitted = data.split()
        address = splitted[-1]
        sensor_value = int(splitted[-2])
        
        #create the list if it is a new address
        if address not in dict_sensors.keys():
            dict_sensors[address] = list()

        #remove first element to keep 30 elements
        len_dict_address = len(dict_sensors[address])
        if len_dict_address == 30:
            dict_sensors[address].pop(0)

        dict_sensors[address].append(sensor_value)

        #verify the slope only if there is at least 30 values
        if len_dict_address == 30:
            result = verify_slope(address)
    return result

def main(ip, port):
    sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    sock.connect((ip, port))

    while True:
        data = recv(sock).decode("utf-8")
        result = process(data)
        if result:
            sock.sendall(bytes(result, "utf-8"))

if __name__ == "__main__":
    parser = argparse.ArgumentParser()
    parser.add_argument("--ip", dest="ip", type=str)
    parser.add_argument("--port", dest="port", type=int)
    args = parser.parse_args()

    main(args.ip, args.port)
	
	#usage: python3 server.py --ip 172.17.0.2 --port 60001