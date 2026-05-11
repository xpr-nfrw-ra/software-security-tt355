#include <iostream>
#include <string>
#include <chrono>
#include <windows.h>
using namespace std;

bool check(string input) {
    int key[] = {72, 29, 7, 0, 91};

    if (input.length() != 5)
        return false;

    for (int i = 0; i < 5; i++) {
        if ((input[i] ^ 0x55) != key[i])
            return false;
    }
    return true;
}

int main() {
    // Anti-debug #1: IsDebuggerPresent
    if (IsDebuggerPresent()) {
        cout << "Debugger detected. Exiting.\n";
        return 0;
    }

    // Anti-debug #2: timing check. A debugger single-stepping through
    // this trivial loop makes it take far longer than on bare metal.
    auto t1 = chrono::high_resolution_clock::now();
    volatile int sink = 0;
    for (int i = 0; i < 1000; i++) sink += i;
    auto t2 = chrono::high_resolution_clock::now();
    auto elapsed_us = chrono::duration_cast<chrono::microseconds>(t2 - t1).count();
    if (elapsed_us > 5000) {
        cout << "Suspicious timing. Exiting.\n";
        return 0;
    }

    string pass;
    cout << "Enter password: ";
    cin >> pass;

    if (check(pass)) {
        cout << "Access Granted!\n";
    } else {
        cout << "Access Denied!\n";
    }
    return 0;
}
