#!/usr/bin/python

import socket

host = '127.0.0.1'
port = 65432

with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as s:
    s.bind((host, port))
    s.listen()
    conn, addr = s.accept()

    with conn:
        print("Connected by ", addr)
        while True:

            print("Waiting for message!\n")
            data = conn.recv(1024)
            conn.sendall(data)

            str_data = data.decode('utf-8')

            if str_data == 'bye':
                print("Exiting")
                s.shutdown(socket.SHUT_RDWR)
                s.close()
                exit()
