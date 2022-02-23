import mqtt from 'mqtt';

var client = new mqtt.Client();

client.on('connect', function () {
  console.log('Connected');
  client.subscribe('presence');
  client.publish('presence', 'Hello World');
});

client.on('message', function(topic, message) {
  console.log('Received: ' + data);
  client.end();
});

client.on('close', function() {
	console.log('Closed');
});

client.connect({ 'address': '127.0.0.1', 'port': 1883 });

