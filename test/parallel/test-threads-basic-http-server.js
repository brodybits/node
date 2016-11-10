'use strict';

require('../common');
const assert = require('assert');

const newThread = require('threads').newThread;
const http = require('http');

const th = newThread();

th.run(
`var http=require("http");
http.createServer((req, res) => {
  res.writeHead(200);
  res.end("hello");
}).listen(8123);`);

var gotResponse = false;

setTimeout(() => {
  http.get({port: 8123}, (res) => {
    gotResponse = true;
  });
}, 100);


process.on('exit', function onExit() {
  assert.ok(gotResponse);
});
