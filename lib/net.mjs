import tcpwrap from "tcpwrap";

var _surface = {};

var Server = function() {
  this._surface = _surface;
  this._events = {
    'close': undefined,
    'connection': undefined,
    'error': undefined,
    'listening': undefined
  };
  this.address = "";
  this.port = 0;
  tcpwrap.create(this);
};

Server.prototype.on = function(name, cb) {
  if (name === 'close') { this._events.close = cb; }
  else if (name === 'connection') { this._events.connection = cb; }
  else if (name === 'error') { this._events.error = cb; }
  else if (name === 'listening') { this._events.listening = cb; }
};

Server.prototype.listen = function(options) {
  this.address = options.address;
  this.port = options.port;
  if (tcpwrap.listen(this, this.address, this.port, 5) == 0) {
    if (this._events.listening !== undefined) {
      this._events.listening();
    }
  }
};

Server.prototype.close = function() {
  tcpwrap.close(this);
};

var createServer = function() {
  var server = new Server();
  return server;
};

var Socket = function() {
  this._surface = _surface;
  this._events = {
    "close": undefined,
    "error": undefined,
    "connect": undefined,
    "data": undefined
  };
  tcpwrap.create(this);
};

Socket.prototype.on = function(name, cb) {
  if (name === 'close') { this._events.close = cb; }
  else if (name === 'error') { this._events.error = cb; }
  else if (name === 'connect') { this._events.connect = cb; }
  else if (name === 'data') { this._events.data = cb; }
}

Socket.prototype.connect = function(options) {
  this.address = options.address;
  this.port = options.port;
  tcpwrap.connect(this, this.address, this.port);
};

Socket.prototype.write = function(data) {
  tcpwrap.write(this, data);
};

Socket.prototype.destroy = function() {
  tcpwrap.close(this);
};

var createSocket = function() {
  var fd = new Socket();
  return fd;
};

_surface = {
  'Server' : Server,
  'createServer' : createServer,
  'Socket' : Socket,
  'createSocket' : createSocket
};

export {
  Server,
  createServer,
  Socket,
  createSocket
};

