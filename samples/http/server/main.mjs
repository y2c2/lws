import { createServer } from 'http';

var server = createServer();

server.on('request', function(req, res) {
  console.log(req);
  res.end('Hello World');
});

server.listen({ 'address': '127.0.0.1', 'port': 3000 });
