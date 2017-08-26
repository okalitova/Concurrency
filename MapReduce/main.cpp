#include <iostream>
#include <mpi/mpi.h>
#include <fstream>
#include <unistd.h>
#include <sstream>
#include <vector>
#include <algorithm>

using namespace std;

int size, myrank;

string ToString(int x) {
    stringstream ss;
    ss << x;
    string str = ss.str();
    return str;
}

void DivideIntoFiles(string filename, int files_number) {
    ifstream input(filename.c_str());
    string line;
    int current_file = 0;
    while (getline(input, line)) {
        string current_filename = "input" + ToString(current_file) +  ".txt";
        std::ofstream output(current_filename.c_str(), fstream::app);
        output << line << "\n";
        current_file = (current_file + 1) % files_number;
    }
}

void SendFileToMapper(string filename, int mapper_rank) {
    vector<string> lines;

    ifstream input(filename.c_str());
    string line;
    while (getline(input, line)) {
        lines.push_back(line);
    }

    const int lines_number = lines.size();
    MPI_Send(const_cast<int*>(&lines_number), 1, MPI_INT, mapper_rank, 4, MPI_COMM_WORLD);

    for (int i = 0; i < lines_number; i++) {
        MPI_Send(const_cast<char*>(lines[i].c_str()), lines[i].size(), MPI_CHAR, mapper_rank, 5, MPI_COMM_WORLD);
    }
}

void MapperFunction(string mapper) {
    MPI_Status Status;

    int lines_number;
    MPI_Recv(&lines_number, 1, MPI_INT, 0, 4, MPI_COMM_WORLD, &Status);
    for (int i = 0; i < lines_number; i++) {
        int count;
        MPI_Probe(0, 5, MPI_COMM_WORLD, &Status);
        MPI_Get_count(&Status, MPI_CHAR, &count);
        char line[count + 1];
        line[count] = 0;
        MPI_Recv(line, count, MPI_CHAR, 0, 5, MPI_COMM_WORLD, &Status);

        string filename = "mapper_input" + ToString(myrank - 2) +  ".txt";
        std::ofstream output(filename.c_str(), fstream::app);
        output << line << "\n";
    }

    string input_filename = "mapper_input" + ToString(myrank - 2) + ".txt";
    string output_filename = "mapper_output" + ToString(myrank - 2) + ".txt";
    string command = mapper + "<" + input_filename + ">" + output_filename;
    system(command.c_str());

    vector<pair<string, string> > keys_values;
    ifstream sort_input(output_filename.c_str());
    string line;
    while (getline(sort_input, line)) {
        stringstream ss(line);
        string key, value;
        ss >> key >> value;
        keys_values.push_back(make_pair(key, value));
    }

    sort(keys_values.begin(), keys_values.end());
    const int elements_number = keys_values.size();
    MPI_Send(const_cast<int*>(&elements_number), 1, MPI_INT, 1, 0, MPI_COMM_WORLD);
    for (int i = 0; i < keys_values.size(); i++) {
        string key, value;
        key = keys_values[i].first;
        value = keys_values[i].second;
        MPI_Send(const_cast<char*>(key.c_str()), key.size(), MPI_CHAR, 1, 1, MPI_COMM_WORLD);
        MPI_Send(const_cast<char*>(value.c_str()), value.size(), MPI_CHAR, 1, 2, MPI_COMM_WORLD);
    }
}

void ReducerFunction(string reducer) {
    MPI_Status Status;

    int elemets_number[size - 2];
    int global_elements_number = 0;
    for (int rank = 2; rank < size; rank++) {
        MPI_Recv(&elemets_number[rank - 2], 1, MPI_INT, rank, 0, MPI_COMM_WORLD, &Status);
        global_elements_number += elemets_number[rank - 2];
    }

    ofstream output("reducer_input.txt");
    string keys[size - 2];
    string values[size - 2];

    for (int rank = 2; rank < size; rank++) {
        int count;
        MPI_Probe(rank, 1, MPI_COMM_WORLD, &Status);
        MPI_Get_count(&Status, MPI_CHAR, &count);
        //cout << "count_key: " << count << "\n";
        char key[count + 1];
        key[count] = 0;
        MPI_Recv(key, count, MPI_CHAR, rank, 1, MPI_COMM_WORLD, &Status);
        keys[rank - 2] = string(key);

        MPI_Probe(rank, 2, MPI_COMM_WORLD, &Status);
        MPI_Get_count(&Status, MPI_CHAR, &count);
        //cout << "count_value: " << count << "\n";
        char value[count + 1];
        value[count] = 0;
        MPI_Recv(value, count, MPI_CHAR, rank, 2, MPI_COMM_WORLD, &Status);
        values[rank - 2] = string(value);

        elemets_number[rank - 2]--;
    }

    while(global_elements_number >= 0) {
        //cout << "here\n";
        string* min_key = min_element(keys, keys + size - 2);
        int min_key_index = min_key - keys;
        int min_key_rank = min_key_index + 2;

        output << *min_key << "\t" << values[min_key_index] << "\n";
        global_elements_number--;

        //cout << global_elements_number << "\n";

        if (global_elements_number == 0) break;

        if (elemets_number[min_key_rank - 2] > 0) {
            int count;
            MPI_Probe(min_key_rank, 1, MPI_COMM_WORLD, &Status);
            MPI_Get_count(&Status, MPI_CHAR, &count);
            //cout << "count_key: " << count << "\n";
            char key[count + 1];
            key[count] = 0;
            MPI_Recv(key, count, MPI_CHAR, min_key_rank, 1, MPI_COMM_WORLD, &Status);
            keys[min_key_rank - 2] = string(key);

            MPI_Probe(min_key_rank, 2, MPI_COMM_WORLD, &Status);
            MPI_Get_count(&Status, MPI_CHAR, &count);
            //output << "count_value: " << count << "\n";
            char value[count + 1];
            value[count] = 0;
            MPI_Recv(value, count, MPI_CHAR, min_key_rank, 2, MPI_COMM_WORLD, &Status);
            values[min_key_rank - 2] = string(value);
            elemets_number[min_key_rank - 2]--;

            //cout << string(key) << " " << min_key_rank - 2 << "\n";
        } else {
            char max_char = char(255);
            keys[min_key_rank - 2] = string(&max_char);
        }
    }

    output.close();
    // reducer!!! ura
    string input_filename = "reducer_input.txt";
    string output_filename = "output.txt";
    string command = reducer + "<" + input_filename + ">" + output_filename;
    system(command.c_str());
}

void Clean() {
    if (myrank == 0) {
        system("rm input?*.txt;");
    }
    if (myrank == 1) {
        system("rm reducer_input.txt;");
    }
    if (myrank > 1) {
        system("rm mapper_input*.txt; rm mapper_output*.txt;");
    }
}

int main(int argc, char** argv) {
    MPI_Status Status;
    MPI_Init(&argc, &argv);
    MPI_Comm_size(MPI_COMM_WORLD, &size);
    MPI_Comm_rank(MPI_COMM_WORLD, &myrank);

    // rank: 0-master, 1-reducer, >1-mapper

    if (myrank == 0) {
        string filename = string(argv[1]);
        int files_number = size - 2; // number of mappers
        DivideIntoFiles(filename, files_number);
        for (int mapper_rank = 2; mapper_rank < size; mapper_rank++) {
            string current_filename = "input" + ToString(mapper_rank - 2) +  ".txt";
            SendFileToMapper(current_filename, mapper_rank);
        }
    }

    MPI_Barrier(MPI_COMM_WORLD); // waits until all mappers get their inputs

    if (myrank > 1) {
        string mapper = argv[2];
        MapperFunction(mapper);
    }

    if (myrank == 1) {
        string reducer = argv[3];
        ReducerFunction(reducer);
    }

    MPI_Barrier(MPI_COMM_WORLD);

    Clean();

    MPI_Finalize();

    return 0;
}