# B08705021_part2 readme

**盧德原 | B08705021 | 資管三**

---

### Environment
Language: C++ / C<br>
OS: Ubuntu 18.04 (Parallel on macOS Monterey 12.1)

### Complie
By typing `make ` in terminal to run the makefile, an executable names **server** will be compiled.
```bash
make
```

Then, type `./server <server IP address> <server port>` to run the server, for example,
```
./server 127.0.0.1 8888
```

Then the server will wait for clients to connect, some commands and status changes will show on the terminal.

#### makefile code
```makefile
output:
    g++ server.cpp -lpthread -o server
```

### Reference
- https://www.thegeekstuff.com/2011/12/c-socket-programming/
- https://www.binarytides.com/socket-programming-c-linux-tutorial/