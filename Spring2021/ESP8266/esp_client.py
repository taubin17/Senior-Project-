#!/usr/bin/python

import socket
import sys

def main():

    host_name = '192.168.1.68'
    port = 65432

    sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        
    sock.connect((host_name, port))
    print("Trying to connect!")
    while True:
        try:
            message = input("Enter your message: ")
            message_in_bytes = message.encode()
            print("Sending message")
            sock.sendall(message)
            print("Message sent!")
            data = sock.recv(1024)
            received = data.decode('utf-8')
            print("Message recieved: ", received)

        except:
            print("Error receiving data!")
            sys.exit()
