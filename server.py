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
    #TODO we need to filter data and only check if data == something that border want to send to the server
    print(data)

def main(ip, port):
    sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    sock.connect((ip, port))

    while True:
        data = recv(sock).decode("utf-8")
        process(data)

if __name__ == "__main__":
    parser = argparse.ArgumentParser()
    parser.add_argument("--ip", dest="ip", type=str)
    parser.add_argument("--port", dest="port", type=int)
    args = parser.parse_args()
    
    main(args.ip, args.port)
	
	#usage: python3 server.py --ip 172.17.0.2 --port 60001