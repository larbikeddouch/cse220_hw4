#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>

#define PORT1 8080
#define PORT2 8082
#define BUFFER_SIZE 1024

#define HIT 1
#define MISSED 2
#define SHOT_OUTSIDE_BOARD 3

long file_size = 0;

typedef struct {
    int type;
    int rotation;
    int circled_x;
    int circled_y;
    int coordinates[4][2];
} Ship;

typedef struct {
    Ship *ships;
    int **board;
    int board_length;
    int board_height;
    char *log;
    int remaining_ships;
} Player;

// 7 shapes; 4 rotations; 4 coordinates 
int shapes[7][4][4][2];

// defining shape 1
shapes[0] = {
    {
        {0, 0}, {0, 1}, {1, 0}, {1, 1}
    }, {
        {0, 0}, {0, 1}, {1, 0}, {1, 1}
    }, {
        {0, 0}, {0, 1}, {1, 0}, {1, 1}
    }, {
        {0, 0}, {0, 1}, {1, 0}, {1, 1}
    }
};

// defining shape 2
shapes[1] = {
    {
        {0, 0}, {1, 0}, {2, 0}, {3, 0}
    }, {
        {0, 0}, {0, 1}, {0, 2}, {0, 3}
    }, {
        {0, 0}, {1, 0}, {2, 0}, {3, 0}
    }, {
        {0, 0}, {0, 1}, {0, 2}, {0, 3}
    }
};

// defining shape 3
shapes[2] = {
    {
        {0, 0}, {0, 1}, {-1, 1}, {-1, 2}
    }, {
        {0, 0}, {1, 0}, {1, 1}, {2, 1}
    }, {
        {0, 0}, {0, 1}, {-1, 1}, {-1, 2}
    }, {
        {0, 0}, {1, 0}, {1, 1}, {2, 1}
    }
};

// defining shape 4
shapes[3] = {
    {
        {0, 0}, {1, 0}, {2, 0}, {2, 1}
    }, {
        {0, 0}, {1, 0}, {0, 1}, {0, 2}
    }, {
        {0, 0}, {0, 1}, {1, 1}, {2, 1}
    }, {
        {0, 0}, {0, 1}, {0, 2}, {-1, 2}
    }
};

// defining shape 5
shapes[4] = {
    {
        {0, 0}, {0, 1}, {1, 1}, {1, 2}
    }, {
        {0, 0}, {1, 0}, {0, 1}, {-1, 1}
    }, {
        {0, 0}, {0, 1}, {1, 1}, {1, 2}
    }, {
        {0, 0}, {1, 0}, {0, 1}, {-1, 1}
    }
};

// defining shape 6
shapes[5] = {
    {
        {0, 0}, {0, 1}, {-1, 1}, {-2, 1}
    }, {
        {0, 0}, {1, 0}, {1, 1}, {1, 2}
    }, {
        {0, 0}, {0, 1}, {1, 0}, {2, 0}
    }, {
        {0, 0}, {0, 1}, {0, 2}, {1, 2}
    }
};

// defining shape 7
shapes[6] = {
    {
        {0, 0}, {0, 1}, {1, 1}, {0, 2}
    }, {
        {0, 0}, {0, 1}, {-1, 1}, {1, 1}
    }, {
        {0, 0}, {0, 1}, {-1, 1}, {0, 2}
    }, {
        {0, 0}, {1, 0}, {2, 0}, {1, 1}
    }
};

int main()
{   
    
    // Server Setup --------------------------------------------------------------------------->
    int listen_fd_1, listen_fd_2, conn_fd;
    struct sockaddr_in address;
    int opt = 1;
    int addrlen = sizeof(address);
    char buffer[BUFFER_SIZE] = {0};

    // Create socket 1
    if ((listen_fd_1 = socket(AF_INET, SOCK_STREAM, 0)) == 0)
    {
        perror("socket failed");
        exit(EXIT_FAILURE);
    }

    // Set socket options
    if (setsockopt(listen_fd_1, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)))
    {
        perror("setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt))");
        exit(EXIT_FAILURE);
    }
    if (setsockopt(listen_fd_1, SOL_SOCKET, SO_REUSEPORT, &opt, sizeof(opt)))
    {
        perror("setsockopt(server_fd, SOL_SOCKET, SO_REUSEPORT, &opt, sizeof(opt))");
        exit(EXIT_FAILURE);
    }

    // Bind socket to port
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT1);

    if (bind(listen_fd_1, (struct sockaddr *)&address, sizeof(address)) < 0)
    {
        perror("[Server] bind() failed.");
        exit(EXIT_FAILURE);
    }

    // Listen for incoming connections
    if (listen(listen_fd_1, 3) < 0)
    {
        perror("[Server] listen() failed.");
        exit(EXIT_FAILURE);
    }

    printf("[Server] Running on port %d\n", PORT1);

    // Accept incoming connection
    if ((conn_fd = accept(listen_fd_1, (struct sockaddr *)&address, (socklen_t *)&addrlen)) < 0)
    {
        perror("[Server] accept() failed.");
        exit(EXIT_FAILURE);
    }


    // Create socket
    if ((listen_fd_2 = socket(AF_INET, SOCK_STREAM, 0)) == 0)
    {
        perror("socket failed");
        exit(EXIT_FAILURE);
    }

    // Set socket options
    if (setsockopt(listen_fd_2, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)))
    {
        perror("setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt))");
        exit(EXIT_FAILURE);
    }
    if (setsockopt(listen_fd_2, SOL_SOCKET, SO_REUSEPORT, &opt, sizeof(opt)))
    {
        perror("setsockopt(server_fd, SOL_SOCKET, SO_REUSEPORT, &opt, sizeof(opt))");
        exit(EXIT_FAILURE);
    }

    // Bind socket to port
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT2);
    if (bind(listen_fd_2, (struct sockaddr *)&address, sizeof(address)) < 0)
    {
        perror("[Server] bind() failed.");
        exit(EXIT_FAILURE);
    }

    // Listen for incoming connections
    if (listen(listen_fd_2, 3) < 0)
    {
        perror("[Server] listen() failed.");
        exit(EXIT_FAILURE);
    }

    printf("[Server] Running on port %d\n", PORT2);

    // Accept incoming connection
    if ((conn_fd = accept(listen_fd_2, (struct sockaddr *)&address, (socklen_t *)&addrlen)) < 0)
    {
        perror("[Server] accept() failed.");
        exit(EXIT_FAILURE);
    }

    while (1){

    }
}

int **makeBoard(int height, int width){

    int **board = malloc(sizeof(int *) * height);

    for (int i = 0; i<height; i++){
        board[i] = calloc(width, sizeof(int));
    }
    return board;
}

void initialize_piece(Ship arr[], int type, int rotation, int row, int col){
    Ship result = {.type = type, .rotation = rotation, .circled_x = row, .circled_y = col};

    int **directions = shapes[result.type][result.rotation];

    for (int i = 0; i<4; i++){
        result.coordinates[i][0] = row + directions[i][0];
        result.coordinates[i][1] = col + directions[i][1];
    }

    arr[0] = result;
}

int shoot(Player p, int row, int col){
    if (row >= p.board_length || row < 0 || col >= p.board_height || col < 0){
        return SHOT_OUTSIDE_BOARD;
    }
    if (p.board[row][col] == '0'){
        return MISSED;
    }
    if (p.board[row][col] == '1'){
        return HIT;
    }
}

int count_remaining_ships(Player p){

    int count = 0;
    // for each of the five ships
    for (int i = 0; i<5; i++){
        // look at all four blocks
        for (int j = 0; j<4; j++){
            int *coordinates = p.ships[i].coordinates[j];

            int x = coordinates[0];
            int y = coordinates[1];

            if (p.board[x][y] == '1'){
                count++;
                break;
            }
        }
    }
    p.remaining_ships = count;
    return count;
}

void update_query_log(Player p, int status, int r, int c){

    char res = (status == HIT) ?  'H' : 'M';
    char str[5];

    sprintf(str, "%c %d %d", res, c, r);

    p.log = realloc(p.log, strlen(p.log) + strlen(str) + 2);

    strncat(p.log, " ", 1);
    strncat(p.log, str, strlen(str));
}












