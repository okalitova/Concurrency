//
// Created by okalitova on 28.11.16.
//

#include <iostream>
#include <vector>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <fstream>

using namespace std;

int height, width;
int turns_number;
int threads_number;

class Server {
public:
    short port_num;
    int socket_id;
    struct sockaddr_in addr;
    vector<int> accepted_clients;
    vector<vector<int>> tasks;
    int clients_number;

    Server(int init_clients_number, int init_port) {
        clients_number = init_clients_number;
        port_num = init_port;
        socket_id = socket(PF_INET, SOCK_STREAM, 0);/* use TCP */
        addr.sin_family = AF_INET; /* for ipv4*/
        addr.sin_port = htons(port_num);
        addr.sin_addr.s_addr = INADDR_ANY; /* listen from anywhere */
        if (bind(socket_id, (struct sockaddr *) &addr, sizeof(addr)) == -1) {
            perror("Problems with bind... :(\n");
            exit(1);
        }

        listen(socket_id, 10);

        for (int i = 0; i < clients_number; i++) {
            vector<int> empty;
            tasks.push_back(empty);
        }
    }

    void Accept() {
        cout << "Server is running\n";

        int accepted_number = 0;
        while (true) {
            struct sockaddr_in new_addr;
            socklen_t new_size;
            int accepted = accept(socket_id, (struct sockaddr *) &new_addr, &new_size);
            if (accepted != -1) {
                accepted_clients.push_back(accepted);
                accepted_number++;
            }
            if (accepted_number == clients_number) break;
        }
    }

    void Run() {
        int succesful_reads = 0;
        int buffers_to_send[clients_number][turns_number + 1][2 * height];

        int turn_number_left[clients_number];
        int turn_number_right[clients_number];

        for (int i = 0; i < clients_number; i++) {
            turn_number_left[i] = -1;
            turn_number_right[i] = -1;
        }

        while (true) {
            int buffer[2 * height];

            for (int i = 0; i < threads_number; i++) {
                int bytes = read(accepted_clients[i], buffer, sizeof(buffer));
                if (bytes == sizeof(buffer)) {
                    succesful_reads++;
                    int prev = (i - 1 + threads_number) % threads_number;
                    int next = (i + 1 + threads_number) % threads_number;
                    turn_number_right[prev]++;
                    turn_number_left[next]++;
                    for (int j = 0; j < height; j++) {
                        buffers_to_send[prev][turn_number_right[prev]][j + height] = buffer[j];
                        buffers_to_send[next][turn_number_left[next]][j] = buffer[j + height];
                    }
                    if (turn_number_right[prev] <= turn_number_left[prev]) {
                        write(accepted_clients[prev], buffers_to_send[prev][turn_number_right[prev]], sizeof(buffers_to_send[prev][turn_number_right[prev]]));
                    }
                    if (turn_number_left[next] <= turn_number_right[next]) {
                        write(accepted_clients[next], buffers_to_send[next][turn_number_left[next]], sizeof(buffers_to_send[next][turn_number_left[next]]));
                    }
                }
            }

            if (succesful_reads == threads_number * (turns_number + 1)) break;
        }
    }
};

vector<vector<int>> ReadTheMatrix(string filename) {
    vector<vector<int>> field;
    ifstream input(filename);
    string s;
    while (getline(input, s)) {
        vector<int> row;
        for (int i = 0; i < s.length(); i++) {
            row.push_back(s[i] - '0');
        }
        field.push_back(row);
    }
    return field;
}


int main(int argc, char* argv[]) {
    turns_number = 10;
    threads_number = atoi(argv[1]);
    int port = atoi(argv[2]);
    vector<vector<int>> field = ReadTheMatrix("input_simple.txt");
    height = field.size();
    width = field[0].size();
    Server server(threads_number, port);
    server.Accept();
    server.Run();
    return 0;
}