# B08705021_part1 readme

**盧德原 | B08705021 | 資管三**

---

### Environment
Language: C++
OS: Ubuntu 18.04

### Complie
By typing `make ` in terminal to run the makefile, an executable names **client** will be compiled.
```bash
make
```

Then, type `./client <server IP address> <server port>` to connect to the server, for example,
```
./client 127.0.0.1 8888
```

Then the program will ask the user to type login port number, for example,
```bash
[Login port number] > 1234
```

Eventually the program will show a menu if connection is successfully established, and the the user can start using it!

#### makefile code
```makefile
output:
    g++ client.cpp -pthread -o client
```

#### menu exaple
```bash
==========MENU===========
1 -> Register
2 -> Login
3 -> Show My Balance and User List
4 -> Make a Transaction
5 -> Exit
=========================
```

### Reference
- https://mropengate.blogspot.com/2018/01/makefile.html
- https://www.youtube.com/watch?v=_r7i5X0rXJk
- https://www.thegeekstuff.com/2011/12/c-socket-programming/
- https://www.binarytides.com/socket-programming-c-linux-tutorial/
- https://www.youtube.com/watch?v=WDn-htpBlnU&t=196s