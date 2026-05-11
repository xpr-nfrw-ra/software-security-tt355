#include <iostream>
#include <fstream>
#include <vector>
#include <cstdlib>
using namespace std;

void xor_decrypt(vector<char>& data, char key) {
    for (auto& b : data) {
        b ^= key;
    }
}

int main() {
    ifstream file("packed.bin", ios::binary | ios::ate);
    streamsize size = file.tellg();
    file.seekg(0, ios::beg);
    vector<char> data(size);
    file.read(data.data(), size);

    xor_decrypt(data, 0xAA);

    ofstream out("unpacked.exe", ios::binary);
    out.write(data.data(), data.size());
    out.close();

    system(".\\unpacked.exe");
    return 0;
}
