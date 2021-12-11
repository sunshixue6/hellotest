import paramiko
import uuid
import time
import sys
import datetime

class Haproxy(object):

    def __init__(self):
        self.host = '172.31.30.32'
        self.port = 22
        self.username = 'root'
        self.pwd = 'root'
        self.__k = None

    def run(self):
        self.connect()
        ssh = paramiko.SSHClient()
        ssh._transport = self.__transport
        str = ""
        curr_time = datetime.datetime.now()
        print(datetime.datetime.strftime(curr_time,'%Y-%m-%d %H:%M:%S'))
        print("BEGIN")
        while 1:
          time.sleep(1)
          # stdin, stdout, stderr = ssh.exec_command('export LD_LIBRARY_PATH=${LD_LIBRARY_PATH}:/ota/install ', 1024, 1000)
          # str = stdout.read().decode('utf-8')
          # print(str)
          # stdin, stdout, stderr = ssh.exec_command('/bin/ps ', 1024, 1000)
          # str = stdout.read().decode('utf-8')
          # print(str)
          curr_time = datetime.datetime.now()
          print(datetime.datetime.strftime(curr_time,'%Y-%m-%d %H:%M:%S'))
          stdin, stdout, stderr = ssh.exec_command(' /lib/ld-2.32.so --library-path /ota/install/ /ota/install/bin/ps aux|head -1;/lib/ld-2.32.so --library-path /ota/install/  /ota/install/bin/ps aux|grep -v PID|sort -rn -k +3|head', 1024, 1000)
          str = stdout.read().decode('utf-8')
          print(str)

          stdin, stdout, stderr = ssh.exec_command(' /lib/ld-2.32.so --library-path /ota/install/ /ota/install/bin/ps aux|head -1;/lib/ld-2.32.so --library-path /ota/install/  /ota/install/bin/ps aux|grep -v PID|sort -rn -k +4|head ', 1024, 1000)
          str = stdout.read().decode('utf-8')
          print(str)
        self.close()

        print("END")

    def connect(self):
        transport = paramiko.Transport((self.host,self.port))
        transport.connect(username=self.username,password=self.pwd)
        self.__transport = transport

    def close(self):
        self.__transport.close()

ha = Haproxy()
while 1:
  try:
      ha.run()
  except IOError:
      print("err")
  else:
      print("else")
