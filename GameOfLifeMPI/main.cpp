/**
 * Created by okalitova on 28.11.16.
 */

#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <mpi.h>

std::vector<std::vector<int> > field;
int updated_field[3000000];
int height, width;
int turns_number;
int threads_number;
int myrank, size;

MPI_Request Requests[100];
MPI_Status Statuses[100];
MPI_Request Requests2[100];
MPI_Status Statuses2[100];

class GameOfLifeOnLine {
public:
    std::pair<int, int> line;

    std::vector<std::vector<int> > field_part;
    std::vector<std::vector<int> > updated_field_part;
    int n, m;
    int current_turn;
    int prev, next;

    MPI_Request Request;
    MPI_Status Status;
    std::vector<int> left_column, right_column;

    GameOfLifeOnLine(std::vector<std::vector<int> > init_field_part, std::pair<int, int> init_line) {
        line = init_line;
        field_part = init_field_part;
        n = field_part.size();
        m = field_part[0].size();
        updated_field_part = field_part;

        prev = myrank - 1;
        if (prev == 0) { prev = size - 1; }

        next = (myrank + 1) % size;
        if (next == 0) { next = 1; }

        for (int i = 0; i < n; i++) {
            left_column.push_back(0);
            right_column.push_back(0);
        }

        SendLeftBoard(0);
        SendRightBoard(0);
    }

    void UpdateField() {

        for (int turn = 0; turn < turns_number; turn++) {
            current_turn = turn;

            bool is_left_board_updated = false;
            bool is_right_board_updated = false;

            for (int i = 0; i < n; i++) {
                for (int j = 1; j < m; j++) {
                    if(!is_left_board_updated && IsLeftBoarderAvailable(current_turn)) {
                        ReceiveLeftBoarder(current_turn);
                        for (int k = 0; k < n; k++) {
                            UpdateCell(k, 0);
                        }
                        SendLeftBoard(current_turn + 1);
                        is_left_board_updated = true;
                    }
                    if (!is_right_board_updated && IsRightBoarderAvailable(current_turn)) {
                        ReceiveRightBoarder(current_turn);
                        for (int k = 0; k < n; k++) {
                            UpdateCell(k, m - 1);
                        }
                        SendRightBoard(current_turn + 1);
                        is_right_board_updated = true;
                    }
                    UpdateCell(i, j);
                }
            }

            while(!is_left_board_updated || !is_right_board_updated) {
                if(!is_left_board_updated && IsLeftBoarderAvailable(current_turn)) {
                    ReceiveLeftBoarder(current_turn);
                    for (int k = 0; k < n; k++) {
                        UpdateCell(k, 0);
                    }
                    SendLeftBoard(current_turn + 1);
                    is_left_board_updated = true;
                }
                if (!is_right_board_updated && IsRightBoarderAvailable(current_turn)) {
                    ReceiveRightBoarder(current_turn);
                    for (int k = 0; k < n; k++) {
                        UpdateCell(k, m - 1);
                    }
                    SendRightBoard(current_turn + 1);
                    is_right_board_updated = true;
                }
            }

            field_part = updated_field_part;
        }
    }

    bool IsRightBoarderAvailable(int turn_to_receive) {
        int flag_right;
        MPI_Iprobe(next, turn_to_receive, MPI_COMM_WORLD, &flag_right, &Status);
        return flag_right;
    }

    bool IsLeftBoarderAvailable(int turn_to_receive) {
        int flag_left;
        MPI_Iprobe(prev, turns_number + turn_to_receive, MPI_COMM_WORLD, &flag_left, &Status);
        return flag_left;
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
                return GetCellFromThePrevLineSimple(i);
            } else {
                return GetCellFromTheNextLineSimple(i);
            }
        }
    }

    int GetCellFromThePrevLineSimple(int i) {
        return left_column[i];
    }

    int GetCellFromTheNextLineSimple(int i) {
        return right_column[i];
    }

    void SendLeftBoard(int turn_to_send) {
        int buffer1[n];
        for (int i = 0; i < n; i++) {
            buffer1[i] = updated_field_part[i][0];
        }

        MPI_Isend(buffer1, n, MPI_INT, prev, turn_to_send, MPI_COMM_WORLD, &Request);
    }

    void SendRightBoard(int turn_to_send) {
        int buffer2[n];
        for (int i = 0; i < n; i++) {
            buffer2[i] = updated_field_part[i][m - 1];
        }

        MPI_Isend(buffer2, n, MPI_INT, next, turns_number + turn_to_send, MPI_COMM_WORLD, &Request);
    }

    void ReceiveLeftBoarder(int turn_to_receive) {
        int buffer1[n];

        MPI_Irecv(buffer1, n, MPI_INT, prev, turns_number + turn_to_receive, MPI_COMM_WORLD, &Request);

        MPI_Wait(&Request, &Status);

        for (int i = 0; i < n; i++) {
            left_column[i] = buffer1[i];
        }
    }

    void ReceiveRightBoarder(int turn_to_receive) {
        int buffer2[n];

        MPI_Irecv(buffer2, n, MPI_INT, next, turn_to_receive, MPI_COMM_WORLD, &Request);

        MPI_Wait(&Request, &Status);

        for (int i = 0; i < n; i++) {
            right_column[i] = buffer2[i];
        }
    }

    void UpdateGlobalField() {
        MPI_Isend(&m, 1, MPI_INT, 0, 1000 + size + myrank, MPI_COMM_WORLD, &Requests[myrank - 1]);

        int buffer[n * m];
        for (int j = 0; j < m; j++) {
            for (int i = 0; i < n; i++) {
                buffer[j * n + i] = field_part[i][j];
            }
        }

        MPI_Isend(buffer, n * m, MPI_INT, 0, 100000 + myrank, MPI_COMM_WORLD, &Requests2[myrank - 1]);
    }
};

std::vector<std::vector<int> > ReadTheMatrix(const std::string& filename) {
    std::vector<std::vector<int> > field;
    std::ifstream input(filename.c_str());
    std::string s;
    while (getline(input, s)) {
        std::vector<int> row;
        for (int i = 0; i < s.length(); i++) {
            row.push_back(s[i] - '0');
        }
        field.push_back(row);
    }
    input.close();
    return field;
}

std::pair<std::vector<std::vector<int> >, std::pair<int, int> > GetFieldPart(std::vector<std::vector<int> > field, int part_number) {
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

    std::vector<std::vector<int> > field_part;
    for (int i = 0; i < height; i++) {
        std::vector<int> row;
        for (int j = l; j < r + 1; j++) {
            row.push_back(field[i][j]);
        }
        field_part.push_back(row);
    }
    return make_pair(field_part, std::make_pair(l, r));
}

int main(int argc, char* argv[]) {
    turns_number = 10;

    field = ReadTheMatrix("input.txt");
    height = field.size();
    width = field[0].size();

    MPI_Status Status;
    MPI_Init(&argc, &argv);
    MPI_Comm_size(MPI_COMM_WORLD, &size);
    MPI_Comm_rank(MPI_COMM_WORLD, &myrank);

    MPI_Barrier(MPI_COMM_WORLD);

    double start, end;
    start = MPI_Wtime();

    threads_number = size - 1;

    std::pair<std::vector<std::vector<int> >, std::pair<int, int> > field_part;
    GameOfLifeOnLine* task;

    if (myrank != 0) {
        field_part = GetFieldPart(field, myrank - 1);
        task = new GameOfLifeOnLine(field_part.first, field_part.second);
    }

    MPI_Barrier(MPI_COMM_WORLD);

    if (myrank != 0) {
        task->UpdateField();
    }

    if (myrank != 0) {
        task->UpdateGlobalField();
    } else {
        int m[size];
        m[0] = 0;
        for (int i = 1; i < size; i++) {
            MPI_Irecv(&m[i], 1, MPI_INT, i, 1000 + size + i, MPI_COMM_WORLD, &Requests[i - 1]);
        }
        MPI_Waitall(size - 1, Requests, Statuses);

        for (int i = 1; i < size; i++) {
            m[i] = m[i - 1] + m[i];
            MPI_Irecv(&updated_field[height * m[i - 1]], height * m[i], MPI_INT, i, 100000 + i, MPI_COMM_WORLD, &Requests2[i - 1]);
        }

        MPI_Waitall(size - 1, Requests2, Statuses2);
    }

//    if (myrank == 0) {
//        for (int i = 0; i < height; i++) {
//            for (int j = 0; j < width; j++) {
//                std::cout << updated_field[j * height + i];
//            }
//            std::cout << "\n";
//        }
//    }

    MPI_Barrier(MPI_COMM_WORLD);

    end = MPI_Wtime();
    if (myrank == 0) { std::cout << end - start; }

    MPI_Finalize();

    return 0;
}