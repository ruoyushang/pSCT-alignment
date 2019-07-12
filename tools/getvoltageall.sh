#!/bin/bash

echo "Primary"
for i in {1..16}; do callps 0 GetVoltage $i; done
echo "Secondary"
for i in {1..8}; do callps 1 GetVoltage $i; done


