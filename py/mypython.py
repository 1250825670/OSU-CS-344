import random
import string

# Uses a list comprehension as a shortcut to generate strings of 10 chars
string1 = ''.join(random.choice(string.ascii_lowercase) for x in range(10))
string2 = ''.join(random.choice(string.ascii_lowercase) for x in range(10))
string3 = ''.join(random.choice(string.ascii_lowercase) for x in range(10))

# Creates files and writes strings to them
with open('file1.txt', 'w') as file1:
    file1.write(string1 + '\n')

with open('file2.txt', 'w') as file2:
    file2.write(string2 + '\n')

with open('file3.txt', 'w') as file3:
    file3.write(string3 + '\n')

# Generates random ints
int1 = random.randint(1, 42)
int2 = random.randint(1, 42)

# Prints all required statements
print(string1)
print(string2)
print(string3)
print(int1)
print(int2)
print(int1 * int2)
