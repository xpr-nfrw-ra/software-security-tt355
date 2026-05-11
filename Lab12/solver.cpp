#include <iostream>
using namespace std;

int main() {
    int key[] = {72, 29, 7, 0, 91};
    cout << "Recovered password: ";
    for (int i = 0; i < 5; i++) {
        char c = key[i] ^ 0x55;
        cout << c;
    }
    cout << "\n";
    return 0;
}
