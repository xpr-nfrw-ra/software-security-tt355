#include <iostream>
#include <fstream>
#include <vector>
using namespace std;

vector<char> read_file(const string& path) {
    ifstream file(path, ios::binary | ios::ate);
    streamsize size = file.tellg();
    file.seekg(0, ios::beg);
    vector<char> data(size);
    file.read(data.data(), size);
    return data;
}

void write_file(const string& path, const vector<char>& data) {
    ofstream file(path, ios::binary);
    file.write(data.data(), data.size());
}

void xor_encrypt(vector<char>& data, char key) {
    for (auto& b : data) {
        b ^= key;
    }
}

int main() {
    string input = "program.exe";
    string output = "packed.bin";
    char key = 0xAA;

    vector<char> data = read_file(input);
    xor_encrypt(data, key);
    write_file(output, data);

    cout << "Packed successfully!\n";
    return 0;
}
