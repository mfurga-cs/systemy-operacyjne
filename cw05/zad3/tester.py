#!/usr/bin/env python3

with open("data/res.txt", "r") as f:
  data = f.read().splitlines()
  c = ord("a")
  for row in data:
    s = set(row)
    if len(s) != 1 or chr(c) not in s:
      print("ERROR: data/res.txt is not valid!")
      break
    c += 1
  else:
    print("OK")



