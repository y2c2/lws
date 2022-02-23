import { createServer } from 'net';

var server = createServer();

server.on('listening', function () {
  console.log('Listening on 127.0.0.1:3000');
});

server.on('error', function (e) {
  if (e.code === 'EADDRINUSE') {
    console.log('Address in use');
    server.close();
  }
});

server.on('connection', function (c) {
  console.log('Client connected');
  c.on('data', function(data) {
    console.log('Incoming data');
    var responseBody = "Hello World";
    c.write('HTTP/1.1 200 OK\r\n' +
      'Content-Length: ' + responseBody.length + '\r\n' +
      '\r\n' +
      responseBody);
  });
});

server.listen({ 'address': '127.0.0.1', 'port': 3000 });
