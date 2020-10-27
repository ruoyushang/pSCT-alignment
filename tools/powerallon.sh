#!/bin/bash

for i in {0..17}; do callps 0 PSon $i; done
for i in {0..9}; do callps 1 PSon $i; done
callps 1 PSon 17