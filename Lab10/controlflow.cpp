#include <iostream>

using namespace std;

int obfuscated_sum(int a, int b) {
    int result = 0;

    // Dead code (չօգտագործվող)
    for (int i = 0; i < 5; i++) {
        int temp = i * 999; // useless
    }

    // Control Flow Obfuscation
    int state = 0;

    while (true) {
        if (state == 0) {
            result = a + b;
            state = 1;
        }
        else if (state == 1) {
            break;
        }
    }

    return result;
}

int main() {
    cout << "Result: " << obfuscated_sum(5, 7) << endl;
    return 0;
}
