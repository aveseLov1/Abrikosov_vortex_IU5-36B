
#include <iostream>
#include <vector>
#include <cmath>
#include <random>
#include <chrono>
#include <thread>
#include <algorithm>

using namespace std;

const int WIDTH = 60;
const int HEIGHT = 20;
const int COUNT = 15;

const double DT = 0.04;
const double STRENGTH = 2.0;
const double LAMBDA = 5.0;
const double MOBILITY = 0.8;

struct Vortex {
    double x, y;
    double vx, vy;
};

vector<Vortex> vortices;

void initialize() {
    random_device rd;
    mt19937 gen(rd());

    uniform_real_distribution<double> rx(5, WIDTH - 5);
    uniform_real_distribution<double> ry(3, HEIGHT - 3);

    for (int i = 0; i < COUNT; i++) {
        vortices.push_back({rx(gen), ry(gen), 0, 0});
    }
}

// Расчёт сил взаимодействия вихрей
void calculateForces() {
    vector<double> fx(COUNT, 0);
    vector<double> fy(COUNT, 0);

    for (int i = 0; i < COUNT; i++) {
        for (int j = i + 1; j < COUNT; j++) {

            double dx = vortices[i].x - vortices[j].x;
            double dy = vortices[i].y - vortices[j].y;

            double r2 = dx * dx + dy * dy + 0.01;
            double r = sqrt(r2);

            // Экранированное отталкивание
            double force = STRENGTH *
                exp(-r / LAMBDA) / r;

            double forceX = force * dx / r;
            double forceY = force * dy / r;

            fx[i] += forceX;
            fy[i] += forceY;

            fx[j] -= forceX;
            fy[j] -= forceY;
        }
    }

    // Обновление скоростей и координат
    for (int i = 0; i < COUNT; i++) {

        vortices[i].vx = MOBILITY * fx[i];
        vortices[i].vy = MOBILITY * fy[i];

        vortices[i].x += vortices[i].vx * DT;
        vortices[i].y += vortices[i].vy * DT;

        // Отражение от границ
        if (vortices[i].x < 1) {
            vortices[i].x = 1;
            vortices[i].vx *= -1;
        }

        if (vortices[i].x >= WIDTH - 1) {
            vortices[i].x = WIDTH - 2;
            vortices[i].vx *= -1;
        }

        if (vortices[i].y < 1) {
            vortices[i].y = 1;
            vortices[i].vy *= -1;
        }

        if (vortices[i].y >= HEIGHT - 1) {
            vortices[i].y = HEIGHT - 2;
            vortices[i].vy *= -1;
        }
    }
}

// Отображение вихрей
void draw() {
    vector<string> screen(
        HEIGHT, string(WIDTH, ' ')
    );

    // Границы сверхпроводника
    for (int x = 0; x < WIDTH; x++) {
        screen[0][x] = '-';
        screen[HEIGHT - 1][x] = '-';
    }

    for (int y = 0; y < HEIGHT; y++) {
        screen[y][0] = '|';
        screen[y][WIDTH - 1] = '|';
    }

    // Размещение вихрей
    for (const auto& v : vortices) {
        int x = static_cast<int>(round(v.x));
        int y = static_cast<int>(round(v.y));

        if (x > 0 && x < WIDTH - 1 &&
            y > 0 && y < HEIGHT - 1) {
            screen[y][x] = '@';
        }
    }

    cout << "\033[H";

    cout << "ABRIKOSOV VORTEX SIMULATION\n";
    cout << "Vortices: " << COUNT << "\n\n";

    for (const auto& line : screen) {
        cout << line << '\n';
    }

    cout << "\n@ - Abrikosov vortices\n";
    cout.flush();
}

int main() {
    initialize();

    cout << "\033[2J\033[H";

    while (true) {
        calculateForces();
        draw();

        this_thread::sleep_for(
            chrono::milliseconds(60)
        );
    }

    return 0;
}
