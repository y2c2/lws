#!/bin/sh
ab -c 100 -v 1 -n 50000 http://127.0.0.1:9950/index.html
