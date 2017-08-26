/**
 * Created by okalitova on 13.11.16.
 */

#include <iostream>
#include <fstream>
#include <vector>
#include <thread>
#include <sys/un.h>
#include <sys/socket.h>
#include <bits/socket.h>
#include <unistd.h>
#include <netinet/in.h>
#include <random>

using namespace std;

vector<vector<int>> field;
int height, width;
int threads_number;
int turns_number;
int port;

class GameOfLifeOnLine {
public:
    pair<int, int> line;

    vector<vector<int>> field_part;
    vector<vector<int>> updated_field_part;
    int n, m;
    int current_turn;
    int rank;

    //for sockets implementation
    vector<int> left_column, right_column;
    int client_socket;

    GameOfLifeOnLine(vector<vector<int>> init_field_part, pair<int, int> init_line, int init_rank) {
        line = init_line;
        rank = init_rank;
        field_part = init_field_part;
        n = field_part.size();
        m = field_part[0].size();
        updated_field_part = field_part;

        InitializeSocket();
        for (int i = 0; i < n; i++) {
            left_column.push_back(0);
            right_column.push_back(0);
        }
    }

    void InitializeSocket() {
        struct sockaddr_in addr;
        int port_num = port;
        client_socket = socket(PF_INET, SOCK_STREAM, 0);
        addr.sin_family = AF_INET; /* for ipv4*/
        addr.sin_port = htons(port_num);
        addr.sin_addr.s_addr = INADDR_ANY; /* listen from anywhere */
        int connected = connect(client_socket, (struct sockaddr *) &addr, sizeof(addr));
        cout << "connectred = " << connected << "\n";
    }

    void SendBorders() {
        int buffer[2 * n];
        for (int i = 0; i < n; i++) {
            buffer[i] = field_part[i][0];
        }
        for (int i = 0; i < n; i++) {
            buffer[n + i] = field_part[i][m - 1];
        }
        write(client_socket, buffer, sizeof(buffer));
    }

    void ReceiveBoarders() {
        int buffer[2 * n];
        while (read(client_socket, buffer, sizeof(buffer)) != sizeof(buffer)) {}
        for (int i = 0; i < n; i++) {
            left_column[i] = buffer[i];
        }
        for (int i = 0; i < n; i++) {
            right_column[i] = buffer[i + n];
        }
    }

    void UpdateField() {
        SendBorders();
        for (int turn = 0; turn < turns_number; turn++) {
            current_turn = turn;

            ReceiveBoarders();

            for (int i = 0; i < n; i++) {
                for (int j = 0; j < m; j++) {
                    UpdateCell(i, j);
                }
            }

            field_part = updated_field_part;

            SendBorders();
        }
    }

    void UpdateCell(int i, int j) {
        int alive_neighbours =  CountAliveNeighbours(i, j);
        if (field_part[i][j] == 1) {
            if (alive_neighbours == 2 || alive_neighbours == 3) {
                updated_field_part[i][j] = 1;
            } else {
                updated_field_part[i][j] = 0;
            }
        } else {
            if (alive_neighbours == 3) {
                updated_field_part[i][j] = 1;
            } else {
                updated_field_part[i][j] = 0;
            }
        }
    }

    int CountAliveNeighbours(int i, int j) {
        int alive_neighbours = 0;
        for (int x = -1; x < 2; x++) {
            for (int y = -1; y < 2; y++) {
                if (x != 0 || y != 0) {
                    alive_neighbours += GetCell(i + x, j + y);
                }
            }
        }
        return alive_neighbours;
    }

    int GetCell(int i, int j) {
        i = (i + n) % n;
        if (0 <= j && j <= m - 1) {
            return field_part[i][j];
        } else {
            if (j == -1) {
                GetCellFromThePrevLineSocket(i);
            } else {
                GetCellFromTheNextLineSocket(i);
            }
        }
    }

    int GetCellFromThePrevLineSocket(int i) {
        return left_column[i];
    }

    int GetCellFromTheNextLineSocket(int i) {
        return right_column[i];
    }

    void UpdateGlobalFeld() {
        for (int i = 0; i < n; i++) {
            for (int j = line.first; j <= line.second; j++) {
                field[i][j] = field_part[i][j - line.first];
            }
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

pair<vector<vector<int>>, pair<int, int>> GetFieldPart(vector<vector<int>> field, int part_number) {
    int k = width / threads_number;
    int ost = width % threads_number;
    int l, r;
    if (part_number < ost) {
        l = (k + 1) * part_number;
        r = l + k;
    } else {
        l = ost * (k + 1) + (part_number - ost) * k;
        r = l + k - 1;
    }

    vector<vector<int>> field_part;
    for (int i = 0; i < height; i++) {
        vector<int> row;
        for (int j = l; j < r + 1; j++) {
            row.push_back(field[i][j]);
        }
        field_part.push_back(row);
    }
    return make_pair(field_part, make_pair(l, r));
}

void GenerateField(int height, int width, string filename) {
    srand(43);
    ofstream output(filename);
    for (int i = 0; i < height; i++) {
        for (int j = 0; j < width; j++) {
            output << rand() % 2;
        }
        output << "\n";
    }
}

int main(int argc, char* argv[]) {
    turns_number = 10;
    threads_number = atoi(argv[1]);
    port = atoi(argv[2]);

    field = ReadTheMatrix("input_simple.txt");
    height = field.size();
    width = field[0].size();
    vector<thread> threads;
    vector<GameOfLifeOnLine> tasks;

    for (int i = 0; i < threads_number; i++) {
        pair<vector<vector<int>>, pair<int, int>> field_part = GetFieldPart(field, i);
        tasks.push_back(GameOfLifeOnLine(field_part.first, field_part.second, i));
    }

    for (int i = 0; i < threads_number; i++) {
        threads.push_back(thread(&GameOfLifeOnLine::UpdateField, &tasks[i]));
    }

    for (int i = 0; i < threads_number; i++) {
        threads[i].join();
    }

    for (int i = 0; i < threads_number; i++) {
        tasks[i].UpdateGlobalFeld();
    }

//    cout << "\n";
//
//    for (int i = 0; i < height; i++) {
//        for (int j = 0; j < width; j++) {
//            cout << field[i][j];
//        }
//        cout << "\n";
//    }

    return 0;
}
