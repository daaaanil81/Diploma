# Diploma
My Diploma work
In this repositorie, I keep my diploma work, because I don`t want his to lose.
In camera_file directory:
  1. export IP='your ip address for server'
  2. make start_http_server // starting server with https protocol
  3. make compile // compiling source files
  4. make clean // clear executable file
# For start program
  1. cd Diploma
  2. make compile
  3. node server.js IP
  4. ./tcp_server/ws-server IP
# Programe will compile with openssl with different parameters -lssl -lcrypto
  1. cd /usr/local/include/
  2. ln -s ../opt/openssl/include/openssl .
  3. export LIBRARY_PATH=$LIBRARY_PATH:/usr/local/opt/openssl/lib/
# For create certificate:
  openssl req -new -newkey rsa:4096 -days 36500 -nodes -x509 -keyout "localhost-100y.key" -out "localhost-100y.cert
# For install libwebsockets
  1. git clone -b v3.2-stable https://github.com/warmcat/libwebsockets.git
  2. cd libwebsockets/ 
  3. mkdir build 
  4. cd build
  5. cmake ..
  6. make
  7. sudo make install    
