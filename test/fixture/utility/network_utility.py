# encoding: utf8
import socket
import time
import os
import sys
import threading
import datetime
import copy
import traceback
import _thread

def print_exception_info(ex):
    print('%s %s' % (type(ex), str(ex)))
    traceback.print_exc()
    
class ServiceBase:
    def __init__(self):
        self.sock = None
        self.addr = None

    def init_network(self, sock, addr):
        self.sock = sock
        self.addr = addr

    def init(self):
        """ 需要定制的行为 """
        pass

    def run(self):
        self.service_handler()

    def service_handler(self):
        """ 需要定制的行为 """
        pass

    def __del__(self):
        if self.addr:
            print("[{}]: Disconnect from {}:{}".format(
                datetime.datetime.now().ctime(), self.addr[0], self.addr[1]))


class ServerBase:
    def __init__(self, port):
        self.port = port
        self.master_socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        self.master_socket.bind(('', self.port))
        self.master_socket.setblocking(False)
        self.master_socket.listen(5)
        print('[{}]: Starting listen to port {}'.format(
            datetime.datetime.now().ctime(), self.port))

    def do_accept(self):
        sock, addr = self.master_socket.accept()
        sock.setblocking(False)
        print('[{}]: Accepting connection from {}:{}'.format(
            datetime.datetime.now().ctime(), addr[0], addr[1]))
        return sock, addr

    def init_service(self, sock, addr, *args):
        service = ServiceBase(sock, addr)
        return service

    def run(self, service):
        sock = None
        addr = None
        service_template = service
        service = None
        while True:
            try:
                sock, addr = self.do_accept()
                service = copy.deepcopy(service_template)
                service.init()
                service.init_network(sock, addr)
                thread = threading.Thread(target=service.run)
                thread.start()
                service = None
                thread = None
                sock = None
                addr = None
            except BlockingIOError:
                pass
            except Exception as ex:
                print_exception_info(ex)
                break


class EchoService(ServiceBase):
    def service_handler(self):
        while True:
            try:
                data = self.sock.recv(1024)
                if not len(data):
                    break
                self.sock.send(data)
            except BlockingIOError:
                pass
            except Exception as ex:
                print_exception_info(ex)
                break


class SendFileService(ServiceBase):
    """ 提供文件下载功能 """

    def __init__(self, filename):
        ServiceBase.__init__(self)
        self.filename = filename
        self.file = None

    def init(self):
        self.file = open(self.filename, 'rb')

    def service_handler(self):
        while True:
            try:
                data = self.file.read()
                if not len(data):
                    break
                self.sock.send(data)
            except BlockingIOError:
                pass
            except Exception as ex:
                print_exception_info(ex)
                break


class RecvFileService(ServiceBase):
    """ 提供文件接收功能 """
    def __init__(self, filename):
        ServiceBase.__init__(self)
        self.filename = filename
        self.file = None

    def init(self):
        #if os.path.exists(self.filename):
         #   raise Exception('{} already exists!'.format(self.filename))
        self.file = open(self.filename, 'wb')

    def service_handler(self):
        while True:
            try:
                data = self.sock.recv(4096)
                if not len(data):
                    break
                self.file.write(data)
            except BlockingIOError:
                pass
            except Exception as ex:
                print_exception_info(ex)
                break


class MsgService(ServiceBase):
    def __init__(self, msg, interval):
        """ interval 表示发送的时间间隔, 以ms计算, 必须为整数 """
        assert(interval > 0)
        ServiceBase.__init__(self)
        self.msg = msg
        self.interval = interval

    def service_handler(self):
        while True:
            try:
                data = self.sock.send(bytes(self.msg, 'utf-8'))
                time.sleep(self.interval / 1000.0)  # sleep按照秒计算
            except BlockingIOError:
                pass
            except Exception as ex:
                print_exception_info(ex)
                break


class ClientBase:
    def __init__(self, ip, port):
        self.ip = ip
        self.port = port
        self.sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)

    def do_connect(self):
        self.sock.connect((self.ip, self.port))

    def run(self, service):
        self.do_connect()
        service.init()
        service.init_network(self.sock, (self.ip, self.port))
        self.sock = None
        service.run()


class ClientEchoService(ServiceBase):
    def handle_read_thread_func(self):
        while True:
            try:
                data = self.sock.recv(4096)
                if not len(data):
                    break
                print(data.decode())
            except BlockingIOError:
                pass
            except Exception as ex:
                print_exception_info(ex)
                break

    def service_handler(self):
        thread = threading.Thread(
            target=self.handle_read_thread_func, daemon=True)
        thread.start()
        thread = None
        while True:
            try:
                data = input()
                if not len(data):
                    break
                self.sock.send(bytes(data, 'utf-8'))
            except BlockingIOError:
                pass
            except Exception as ex:
                print_exception_info(ex)
                break


def print_usage():
    exe_file_name = sys.argv[0].split('\\')[-1]
    usage = 'Usage:\n'\
            '\t{} --server [[--echo | \n'\
            '\t\t          --recv-file <filename> |\n'\
            '\t\t          --send-file <filename> |\n'\
            '\t\t          --msg <text> --interval <interval>] --port <port>]\n'\
            '\t{} --client [[--echo [ --stdin | --msg <text> --interval <interval>]\n'\
            '\t\t\t\t    [ --download-file <filename> ]\n'\
            '\t\t\t\t    [ --upload-file <filename>   ] --ip <ip> --port <port>]\n'\
            '\t{} --help'.format(exe_file_name, exe_file_name, exe_file_name)
    print(usage)


def print_warning_and_exit():
    print('Invalid argument!')
    print_usage()
    sys.exit(-1)


def main():
    if len(sys.argv) < 2:
        print_warning_and_exit()
    args = sys.argv[1:]
    arg_count = len(args)
    if sys.argv[1].lower() == '--server':
        # server 模式
        if arg_count == 4 and args[1].lower() == '--echo':
            port = int(args[-1])
            server = ServerBase(port)
            server.run(EchoService())
        elif arg_count == 5:
            if args[1].lower() == '--recv-file':
                filename = args[2]
                port = int(args[-1])
                server = ServerBase(port)
                server.run(RecvFileService(filename))
            elif args[1].lower() == '--send-file':
                filename = args[2]
                port = int(args[-1])
                server = ServerBase(port)
                server.run(SendFileService(filename))
            else:
                print_warning_and_exit()
        elif arg_count == 7 and args[1] == '--msg':
            msg = args[2] + '\r\n'
            interval = int(args[4])
            port = int(args[-1])
            server = ServerBase(port)
            server.run(MsgService(msg, interval))
        else:
            print_warning_and_exit()
    elif sys.argv[1].lower() == '--client':
        # client 模式
        ip = args[-3]
        port = int(args[-1])
        client = ClientBase(ip, port)
        if args[1].lower() == '--echo':
            if arg_count == 7 and args[2].lower() == '--stdin': 
                client.run(ClientEchoService())
            elif arg_count == 10 and args[2].lower() == '--msg':
                msg = args[3]
                interval = int(args[5])
                client.run(MsgService(msg, interval))
            else:
                print_warning_and_exit()
        elif arg_count == 7:
            if args[1].lower() == '--download-file':
                filename = args[2]
                client.run(RecvFileService(filename))
            elif args[1].lower() == '--upload-file':
                filename = args[2]
                client.run(SendFileService(filename))
            else:
                print_warning_and_exit()
        else:
            print_warning_and_exit()
    elif sys.argv[1].lower() == '--help':
        print_usage()
    else:
        print_warning_and_exit()


if __name__ == '__main__':
    main()
