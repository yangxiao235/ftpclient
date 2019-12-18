import socket
import threading
import sys

if len(sys.argv) < 3:
    print("Usage: server_recv_file serverport outfile\n")
    print(len(sys.argv))
    sys.exit()
    
bind_ip, bind_port, outfile = "127.0.0.1", int(sys.argv[1]), sys.argv[2]

server = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
server.bind((bind_ip, bind_port))
server.listen(5)  # max backlog of connections

print('Listening on {}:{}'.format(bind_ip, bind_port))


def handle_client_connection(client_socket):
    file = open(outfile, "wb+")
    while 1:
        contents = client_socket.recv(4096)
        if not contents:
            break
        file.write(contents)
    file.close()
    client_socket.close()
    
while True:
    client_sock, address = server.accept()
    print('Accepted connection from {}:{}'.format(address[0], address[1]))
    client_handler = threading.Thread(
        target=handle_client_connection,
        args=(client_sock,)  # without comma you'd get a... TypeError: handle_client_connection() argument after * must be a sequence, not _socketobject
    )
    client_handler.start()
