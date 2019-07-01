#!/bin/bash

for i in {1..16}; do callps 0 PSoff $i; done
for i in {1..8}; do callps 1 PSoff $i; done

