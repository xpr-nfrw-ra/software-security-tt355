#include <iostream>
#include <vector>
#include <string>

using namespace std;

// Encode function
vector<int> xor_encode(const string& text, int key) {
    vector<int> encoded;
    for (char c : text) {
        encoded.push_back(c ^ key);
    }
    return encoded;
}

// Decode function
string xor_decode(const vector<int>& data, int key) {
    string decoded;
    for (int val : data) {
        decoded.push_back(char(val ^ key));
    }
    return decoded;
}

int main() {
    string original = "Hello World";
    int key = 23;

    vector<int> encoded = xor_encode(original, key);

    cout << "Encoded: ";
    for (int val : encoded) {
        cout << val << " ";
    }
    cout << endl;

    string decoded = xor_decode(encoded, key);
    cout << "Decoded: " << decoded << endl;

    return 0;
}
