import socket
import threading
import sys

# ------------------------------------------
# Usage: server_send_file serverport
# for example:
#   server_send_file 6000
# ------------------------------------------
if len(sys.argv) != 3:
    print("Usage: server_send_file serverport filesend\n")
    sys.exit()
    
bind_ip, bind_port, filesend = "127.0.0.1", int(sys.argv[1]), sys.argv[2]

server = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
server.bind((bind_ip, bind_port))
server.setblocking(False)
server.listen(5)

print('Listening on {}:{}'.format(bind_ip, bind_port))


def handle_client_connection(client_socket, addr):
    file = open(filesend, "rb")
    client_socket.setblocking(True)
    while True:
        contents = file.read()
        if not contents:
            break
        try:
          client_socket.send(contents)
        except BlockingIOError:
          pass
        except Exception as e:
          print('Exception: {}'.format(str(e)))
          file.close()
          client_socket.close()
          break
    file.close()
    client_socket.close()
    print('({}) Close connection from {}:{}'.format(number_connections, addr[0], addr[1]))


number_connections = 0   
while True:
    try:
        client_sock, address = server.accept()
        number_connections = number_connections + 1
        print('({}) Accepted connection from {}:{}'.format(number_connections, address[0], address[1]))
        client_handler = threading.Thread(
            target=handle_client_connection,
            args=(client_sock, address, )  # without comma you'd get a... TypeError: handle_client_connection() argument after * must be a sequence, not _socketobject
        )
        client_handler.start()
    except:
        pass