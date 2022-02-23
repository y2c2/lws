import { Socket } from 'net';

var client = new Socket();

var HOST = '127.0.0.1';
var PORT = 3000;

client.on('connect', function () {
  console.log('Connected');
  client.write('GET / HTTP/1.1\r\n' +
    'Host: ' + HOST + '\r\n\r\n');
});

client.on('data', function(data) {
  console.log('Received: ' + data);
  client.destroy();
});

client.on('close', function() {
	console.log('Closed');
});

client.connect({ 'address': '127.0.0.1', 'port': 3000 });

