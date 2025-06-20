# populate a sample large log file
#
from datetime import datetime

def generate_large_log_file():
    with open('large_log_file.log', 'w') as file:
        for i in range(10000000):
          date = datetime.now()
          file.write(f"{date} Log entry {i}\n")

generate_large_log_file()
