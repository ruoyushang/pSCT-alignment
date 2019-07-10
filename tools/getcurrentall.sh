#!/bin/bash

echo "Primary"
for i in {0..48}; do callps 0 GetCurrent $i; done
echo "Secondary"
for i in {0..24}; do callps 1 GetCurrent $i; done

