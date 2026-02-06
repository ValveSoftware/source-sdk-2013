#!/usr/bin/python
import datetime

def main():
  date = datetime.datetime.now().isoformat(timespec="milliseconds")
  print(f"// Last rebuilt at {date}\n")

if __name__ == "__main__":
  main()