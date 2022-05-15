import socket
import argparse
import sys

PREFIX = "[SENSOR_VALUE]"
OPEN = "OPEN"
CLOSE = "CLOSE"
THRESHOLD = 60
MAX_ELEMENTS = 30

dict_sensors = dict()
return_result = {OPEN : '1', CLOSE : '0'}

def recv(sock):
    data = sock.recv(1)
    message = b""
    while data.decode("utf-8") != '\n':
        message += data
        data = sock.recv(1)
    return message

def verify_slope(address):
    slope = sum(dict_sensors[address]) / MAX_ELEMENTS
    return OPEN if slope > THRESHOLD else CLOSE

def process(data):
    result = ""
    #We receive information from the Border Router Node like this : [SENSOR_VALUE] ... sensor_value address
    if data.startswith(PREFIX):
        splitted = data.split()
        address = splitted[-1]
        sensor_value = int(splitted[-2])
        
        #create the list if it is a new address
        if address not in dict_sensors.keys():
            dict_sensors[address] = list()

        len_dict_address = len(dict_sensors[address])

        #remove first element when we reach the max_elements we want
        if len_dict_address == MAX_ELEMENTS:
            dict_sensors[address].pop(0)

        dict_sensors[address].append(sensor_value)

        #verify the slope only if there is at least 30 values
        if len_dict_address == MAX_ELEMENTS:
            result = verify_slope(address)
    return result

def main(ip, port):
    sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    sock.connect((ip, port))

    while True:
        data = recv(sock).decode("utf-8")
        result = process(data)
        if result:
            sock.sendall(bytes(return_result[result].encode()))

if __name__ == "__main__":
    parser = argparse.ArgumentParser()
    parser.add_argument("--ip", dest="ip", type=str)
    parser.add_argument("--port", dest="port", type=int)
    args = parser.parse_args()

    main(args.ip, args.port)
	
	#usage: python3 server.py --ip 172.17.0.2 --port 60001