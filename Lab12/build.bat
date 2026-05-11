@echo off
g++ -std=c++17 packer.cpp  -o packer.exe  || exit /b 1
g++ -std=c++17 stub.cpp    -o stub.exe    || exit /b 1
g++ -std=c++17 crackme.cpp -o crackme.exe || exit /b 1
g++ -std=c++17 solver.cpp  -o solver.exe  || exit /b 1
echo Build OK
