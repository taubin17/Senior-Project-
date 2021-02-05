#!/usr/bin/python

import socket

host_name = '127.0.0.1'
port = 65432


with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as s:
    s.connect((host_name, port))
    while True:
        message = input("Enter your message: ")
        message_in_bytes = message.encode()
        s.sendall(message_in_bytes)
        data = s.recv(1024)
        data = data.decode('utf-8')
        print("Recieved", data)
