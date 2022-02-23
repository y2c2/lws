import httpwrap from "httpwrap";

var _surface = {};

var Server = function() {
  this._surface = _surface;
  this._events = {
    'close': undefined,
    'request': undefined,
    'error': undefined,
    'listening': undefined
  };
  this.address = "";
  this.port = 0;
  httpwrap.create(this);
};

Server.prototype.on = function(name, cb) {
  if (name === 'close') { this._events.close = cb; }
  else if (name === 'request') { this._events.request = cb; }
  else if (name === 'error') { this._events.error = cb; }
  else if (name === 'listening') { this._events.listening = cb; }
};

Server.prototype.listen = function(options) {
  this.address = options.address;
  this.port = options.port;
  if (httpwrap.listen(this, this.address, this.port, 5) == 0) {
    if (this._events.listening !== undefined) {
      this._events.listening();
    }
  }
};

Server.prototype.close = function() {
  httpwrap.close(this);
};

var createServer = function() {
  var server = new Server();
  return server;
};

var IncomingMessage = function(httpVersion, method, url, headers, body) {
  this.httpVersion = httpVersion;
  this.method = method;
  this.url = url;
  this.headers = headers;
  this.body = body;
};

var createIncomingMessage = function(httpVersion, method, url, headers, body) {
  return new IncomingMessage(httpVersion, method, url, headers, body);
};

var ServerResponse = function() {
  this.sendBuf = '';
  this.statusCode = 200;
  this.statusMessage = '';
  this.headers = {};
};

ServerResponse.prototype.writeHead = function(statusCode, statusMessage, headers) {
  if (statusCode !== undefined)
  {
    this.statusCode = statusCode;
    if (statusMessage !== undefined)
    {
      this.statusMessage = statusMessage;
      if (headers !== undefined)
      {
        this.headers = this.headers;
      }
    }
  }
};

ServerResponse.prototype.end = function(data) {
  this.sendBuf += data;
};

var createServerResponse = function() {
  return new ServerResponse();
};

_surface = {
  'Server' : Server,
  'createServer' : createServer,
  'IncomingMessage' : IncomingMessage,
  'createIncomingMessage' : createIncomingMessage,
  'ServerResponse' : ServerResponse,
  'createServerResponse' : createServerResponse
};

export {
  Server,
  createServer,
  IncomingMessage,
  createIncomingMessage,
  ServerResponse,
  createServerResponse
};

