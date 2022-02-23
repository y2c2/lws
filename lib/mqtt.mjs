import mqttwrap from 'mqttwrap';

var _surface = {};

var MQTT_DEFAULT_PORT = 1883;

var Client = function () {
  this._surface = _surface;
  this._events = {
    'connect': undefined,
    'reconnect': undefined,
    'close': undefined,
    'offline': undefined,
    'error': undefined,
    'end': undefined,
    'message': undefined,
    'packetsend': undefined,
    'packetreceive': undefined
  };
};

Client.prototype.on = function (name, cb) {
  if (name === 'connect') { this._events.connect = cb; }
  else if (name === 'reconnect') { this._events.reconnect = cb; }
  else if (name === 'close') { this._events.close = cb; }
  else if (name === 'offline') { this._events.offline = cb; }
  else if (name === 'error') { this._events.error = cb; }
  else if (name === 'end') { this._events.end= cb; }
  else if (name === 'message') { this._events.message = cb; }
  else if (name === 'packetsend') { this._events.packetsend = cb; }
  else if (name === 'packetreceive') { this._events.packetreceive = cb; }
};

Client.prototype.publish(topic, message, options, callback) {
  console.log('not implemented');
};

_surface = {
  'Client' : Client
}:

export {
  Client,
  connect
};

