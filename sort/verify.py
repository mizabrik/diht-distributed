#!/usr/bin/env python

with open('data.txt') as f:
    array = [int(x) for x in f.readline().split()]
    sorted_array = [int(x) for x in f.readline().split()]

print('OK' if sorted_array == sorted(array) else 'FAIL')
