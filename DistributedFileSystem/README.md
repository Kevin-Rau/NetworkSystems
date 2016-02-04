# Network Systems PA2: Distributed File Systems

### Running the DFS

using the shell file:

- chmod +x shell
- ./shell

cleaning/making files:

- make clean
- make 

# manual running of DFS:
./dfs /DFS1 10001 &
./dfs /DFS2 10002 &
./dfs /DFS3 10003 &
./dfs /DFS4 10004 &

# start client:
./dfc client/dfc.conf
x

* PUT: upload a file to server.
* GET: open chosen file on server.
* LIST: show files stored in server.

example terminal commands:

- > PUT <test.txt>
- > LIST
- > GET <test.txt>


