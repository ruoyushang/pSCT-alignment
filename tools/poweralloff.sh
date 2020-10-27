#!/bin/bash

for i in {0..17}; do callps 0 PSoff $i; done
for i in {0..9}; do callps 1 PSoff $i; done
callps 1 PSoff 17