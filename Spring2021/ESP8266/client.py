#!/usr/bin/python

import socket
import sys

host_name = '192.168.1.100'
port = 65432


with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as s:
    s.connect((host_name, port))
    while True:
        message = input("Enter your message: ")
        message_in_bytes = message.encode()
        s.sendall(message_in_bytes)
        data = s.recv(1024)
        data = data.decode('utf-8')

        if data == 'bye':
            print('Exiting!')
            s.shutdown(socket.SHUT_RDWR)
            s.close()
            sys.exit()

        print("Recieved", data)
