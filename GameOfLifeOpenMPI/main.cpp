/**
 * Created by okalitova on 28.11.16.
 */

#include <iostream>
#include <fstream>
#include <vector>
#include <omp.h>
#include <thread>

using namespace std;

vector<vector<int>> field;
vector<int> turns_counter;
int height, width;
int threads_number;
int turns_number;
vector<vector<pair<vector<int>, vector<int>>>> boarders;

class GameOfLifeOnLine {
public:
    pair<int, int> line;
    int rank;
    int prev_rank, next_rank;
    int current_turn;

    vector<vector<int>> field_part;
    vector<vector<int>> updated_field_part;
    int n, m;

    GameOfLifeOnLine(vector<vector<int>> init_field_part, pair<int, int> init_line, int init_rank) {
        rank = init_rank;
        prev_rank = ((rank - 1) + threads_number) % threads_number;
        next_rank = ((rank + 1) + threads_number) % threads_number;
        line = init_line;
        field_part = init_field_part;
        n = field_part.size();
        m = field_part[0].size();
        updated_field_part = field_part;

        vector<int> l, r;
        for (int i = 0; i < n; i++) {
            l.push_back(field_part[i][0]);
            r.push_back(field_part[i][m - 1]);
        }

        boarders[rank][0] = make_pair(l, r);
    }

    void UpdateField() {
        for (int turn = 0; turn < turns_number; turn++) {
            current_turn = turn;
            for (int i = 0; i < n; i++) {
                for (int j = 0; j < m; j++) {
                    UpdateCell(i, j);
                }
            }
            field_part = updated_field_part;

            vector<int> l, r;
            for (int i = 0; i < n; i++) {
                l.push_back(field_part[i][0]);
                r.push_back(field_part[i][m - 1]);
            }

            boarders[rank][turn + 1] = make_pair(l, r);

            turns_counter[rank]++;
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
                GetCellFromThePrevLine(i);
            } else {
                GetCellFromTheNextLine(i);
            }
        }
    }

    int GetCellFromThePrevLine(int i) {
        while (turns_counter[prev_rank] < current_turn) {}
        return boarders[prev_rank][current_turn].second[i];
    }

    int GetCellFromTheNextLine(int i) {
        while (turns_counter[next_rank] < current_turn) {}
        return boarders[next_rank][current_turn].first[i];
    }

    void UpdateGlobalField() {
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

int main(int argc, char* argv[]) {
    turns_number = 10;
    threads_number = atoi(argv[1]);

    field = ReadTheMatrix("input.txt");
    height = field.size();
    width = field[0].size();
    vector<GameOfLifeOnLine> tasks;

    for (int i = 0; i < threads_number; i++) {
        turns_counter.push_back(0);
        vector<pair<vector<int>, vector<int>>> empty;
        boarders.push_back(empty);
        for (int j = 0; j < turns_number + 1; j++) {
            pair<vector<int>, vector<int>> empty_pair;
            boarders[i].push_back(empty_pair);
        }
    }

    for (int i = 0; i < threads_number; i++) {
        pair<vector<vector<int>>, pair<int, int>> field_part = GetFieldPart(field, i);
        tasks.push_back(GameOfLifeOnLine(field_part.first, field_part.second, i));
    }

#pragma omp parallel num_threads(threads_number)
    {
#pragma omp for

            for (int i = 0; i < threads_number; i++) {
                tasks[i].UpdateField();
            }
    }

    for (int i = 0; i < threads_number; i++) {
        tasks[i].UpdateGlobalField();
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