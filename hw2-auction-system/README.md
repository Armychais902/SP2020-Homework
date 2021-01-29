# System Programming 2020 HW2: Auction System

##  Problem Description

Implement an auction system which handles a number of auctions simultaneously.

## Tasks
1. Understand how to communicate between processes through `pipe` and `FIFO`

2. Practice using `fork` to create processes

3. Write a program in **shell script**

4. Detailed description and spec of the auction system, please refer to ```description.pdf```.

## How to Run

Run ```make``` and generate:
- ```host```

- ```player```

### auction_system.sh

```bash
bash auction_system.sh [n_host] [n_player]
```

- Two arguments
    - `n_host`: the number of hosts
        
    - `n_player`: the number of players

### host.c

```bash
./host [host_id] [key] [depth]
```

- Three arguments
    - `host_id`: the id of host
    
    - `key`: a key randomly generated for each host (0 ≤ key ≤ 65535)
    
        - Used for auction system to map the responses from hosts, since all hosts write to the same FIFO file.
        
    - `depth`: the depth of hosts
    
        - Starting from 0 and incrementing by 1 per fork
        
### player.c

```bash
./player [player_id]
```

- One argument

    - `player_id`: the id of player
    
## Implementation

### auction_system.sh
- `exec`
  
  - Open fifo file in read/write mode to prevent `EOF` or `SIGPIPE`
  
  - Execute `host`

- `read`

    - Read key to identify host
    
    - Read the responses from host

- `wait`

    - Wait for all the background processes to terminate
    
### host.c

- `fork()` and `pipe()`
  
  - `pipe()` befor `fork()` to guarantee the child inherits fds

- `pipe()` and `dup2()`

  - `dup2()` after `pipe()`, so the child process could read/write from `stdin`/`stdout`

- `execv()`
  
  - Execute `host` and pass corresponding args to it.

- `fprintf()`, `fscanf()`, `fflush()`, `fdopen()`

  - `fdopen()` open file with pipe fd and associates a stream with it
  
  - `fflush()` each time after `fprintf()` and `fscanf()` to make sure the data in buffer is being flushed out
