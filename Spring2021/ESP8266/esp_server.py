#!/usr/bin/python

import socket
import sys

host = '192.168.1.100'
port = 65432

while True:
    s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    s.bind((host, port))
    s.listen(5)
    print('Listening on: ', host, port)

    conn, addr = s.accept()
    while True:
        print("Connected by ", addr)
        message = "Please enter a filename and extension to be sent")
        data = conn.recv(1024)
        str_data = data.decode('utf-8')
        print('File Requested: ', data.decode('utf-8'))
        fd = open(str_data, 'r')

        data = fd.read()

        conn.sendall(data)
        
        if str_data == 'bye':
            print("Goodbye!")
            s.shutdown(socket.SHUT_RDWR)
            s.close()
            sys.exit()

        print("Waiting for new connection!")
