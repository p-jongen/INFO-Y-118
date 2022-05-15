import socket
import argparse
import sys

def recv(sock):
    data = sock.recv(1)
    message = b""
    while data.decode("utf-8") != '\n':
        message += data
        data = sock.recv(1)
    return message

def process(data):
    result = ""
    if data.startswith("[SENSOR_VALUE]"):
        sensor_value = data.split()[-1]
        result = "OPEN" #TODO compute to know if open or not
    return result

def main(ip, port, threshold):
    sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    sock.connect((ip, port))

    while True:
        data = recv(sock).decode("utf-8")
        result = process(data)
        if result:
            #TODO send the result to the border in the right format (not a str)
            #sock.sendall(result)
            print("")

if __name__ == "__main__":
    parser = argparse.ArgumentParser()
    parser.add_argument("--ip", dest="ip", type=str)
    parser.add_argument("--port", dest="port", type=int)
    parser.add_argument("--threshold", dest="threshold", type=int, default=0)
    args = parser.parse_args()

    main(args.ip, args.port, args.threshold)
	
	#usage: python3 server.py --ip 172.17.0.2 --port 60001