#include <iostream>
#include <thread>
#include <queue>
#include <mutex>
#include <condition_variable>
#include <chrono>
#include <cstdlib>
#include <vector>

using namespace std;

const int maxSize = 5;

queue<string> flourBuffer;
queue<string> waterBuffer;
mutex mtx;
condition_variable bufferStatus;
condition_variable workStatus;
bool isProducing = true;
bool isDay = true;

class Miller {
public:
    Miller(int id, string type, int time)
        : id(id), type(type), time(time) {}

    void supply() {
        string ingredientType = type;
        while (isProducing) {
            this_thread::sleep_for(chrono::milliseconds(time));

            unique_lock<mutex> lock(mtx);
            workStatus.wait(lock, [] { return isDay; });
            bufferStatus.wait(lock, [&ingredientType] {
                if (ingredientType == "Flour") return flourBuffer.size() < maxSize;
                if (ingredientType == "Water") return waterBuffer.size() < maxSize;
                return false;
            });

            string ingredient = ingredientType + to_string(id * 100 + rand() % 100);
            if (ingredientType == "Flour") {
                flourBuffer.push(ingredient);
            } else {
                waterBuffer.push(ingredient);
            }
            cout << "Miller " << id << " supplied " << ingredient << endl;
            lock.unlock();
            bufferStatus.notify_all();
        }
    }

private:
    int id;
    string type;
    int time;
};

class Baker {
public:
    Baker(int id) : id(id) {}

    void bake() {
        while (isProducing) {
            this_thread::sleep_for(chrono::milliseconds(300));

            unique_lock<mutex> lock(mtx);
            workStatus.wait(lock, [] { return isDay; });
            bufferStatus.wait(lock, [] { return !flourBuffer.empty() && !waterBuffer.empty(); });

            string flour = flourBuffer.front();
            flourBuffer.pop();
            string water = waterBuffer.front();
            waterBuffer.pop();

            cout << "Baker " << id << " used " << flour << " and " << water << " to bake bread" << endl;

            lock.unlock();
            bufferStatus.notify_all();

        }
    }

private:
    int id;
};

class BakeryClock {
public:
    void run() {
        while (true) {
            cout << "It's day time. Keep working!" << endl;
            isDay = true;
            workStatus.notify_all();
            this_thread::sleep_for(chrono::milliseconds(10000));

            cout << "It's night time. Rest!" << endl;
            isDay = false;
            workStatus.notify_all();
            this_thread::sleep_for(chrono::milliseconds(10000));
        }
    }
};

int main() {
    char key = ' ';

    vector<thread> millerThreads;
    vector<thread> bakerThreads;

    BakeryClock bakeryClock;
    thread bakeryClockThread(&BakeryClock::run, &bakeryClock);

    while (isProducing) {
        cin >> key;
        if (key == 'p') {
            millerThreads.emplace_back(&Miller::supply, Miller(millerThreads.size() + 1, "Flour", 1000));
            millerThreads.emplace_back(&Miller::supply, Miller(millerThreads.size() + 1, "Water", 2000));
        } else if (key == 'c') {
            bakerThreads.emplace_back(&Baker::bake, Baker(bakerThreads.size() + 1));
        } else if (key == 'q') {
            isProducing = false;
        }
    }

    bakeryClockThread.join();

    for (thread& millerThread : millerThreads) {
        millerThread.join();
    }
    for (thread& bakerThread : bakerThreads) {
        bakerThread.join();
    }

    return 0;
}