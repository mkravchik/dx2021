#!/bin/bash

java -Xms8G -Xmx16G -jar build/shadow/lib-*.jar "$@"
