#!/bin/bash

for i in {0..48}; do callps 0 GetCurrent $i; done
for i in {0..24}; do callps 1 GetCurrent $i; done

