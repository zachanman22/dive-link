// my first program in C++
// conditional operator
#include <iostream>
#include <chrono>
#include <ctime>
#include <stdlib.h> /* srand, rand */
using namespace std;

int main()
{
    srand(time(NULL));
    auto start = std::chrono::system_clock::now();
    float q1, q2;
    q2 = (rand() % 10 + 1) / (rand() % 10 + 2);
    float a = (rand() % 10 + 1) / (rand() % 10 + 2);
    float b = (rand() % 10 + 1) / (rand() % 10 + 2);
    float c = (rand() % 10 + 1) / (rand() % 10 + 2);
    float d = (rand() % 256 + 1) / 170.0;
    for (long i = 0; i < 100000000; i++)
    {
        float q0 = std::min(float(0.25), std::max(float(1.0), (a * b + c * a + b) * q2 * d));
        q1 = q0 / (q2 + 1);
        q2 = q1;
    }
    auto end = std::chrono::system_clock::now();

    std::chrono::duration<double> elapsed_seconds = end - start;
    std::time_t end_time = std::chrono::system_clock::to_time_t(end);
    std::cout << "Q2 " << q2 << std::endl;
    std::cout << "finished computation at " << std::ctime(&end_time) << "elapsed time: " << elapsed_seconds.count() << "s" << std::endl;
}