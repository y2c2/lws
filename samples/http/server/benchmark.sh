#!/bin/sh
wrk -t12 -c400 -d5s http://127.0.0.1:3000/index.html
