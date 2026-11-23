#include<cmath>
#include<cstdio>
#include<vector>

#ifdef _WIN32
#include<windows.h>
#else
#include<chrono>
#include<thread>
#endif

using f = double;

struct V3 {
    f x, y, z;
};

const f pi = 3.14159265358979323846;
const f R = 1, r = 0.5, step = 2e-2;    // ring radius, tube radius, sampling step
const int W = 80, H = 28;               // screen size
const f focal = 60, aspect = 0.5;       // perspective scale, character aspect
const char shade[] = ".,-~:;=!*#$@";    // 13 brightness levels

std::vector<V3> pts, nrm;

V3 operator+(V3 a, V3 b) { return {a.x + b.x, a.y + b.y, a.z + b.z}; }
V3 operator-(V3 a, V3 b) { return {a.x - b.x, a.y - b.y, a.z - b.z}; }
V3 operator*(V3 a, f s) { return {a.x*s, a.y*s, a.z*s}; }
f dot(V3 a, V3 b) { return a.x*b.x + a.y*b.y + a.z*b.z; }
V3 cross(V3 a, V3 b) { return {a.y*b.z - a.z*b.y, a.z*b.x - a.x*b.z, a.x*b.y - a.y*b.x}; }
V3 unit(V3 a) { return a * (1 / sqrt(dot(a, a))); }
V3 rotx(V3 v, f a) {
    f c = cos(a), s = sin(a);
    return {v.x, c*v.y - s*v.z, s*v.y + c*v.z};
}

void init_torus() {
    for (f a = 0; a < 2*pi; a += step) {
        for (f b = 0; b < 2*pi; b += step) {
            f ca = cos(a), sa = sin(a), cb = cos(b), sb = sin(b);
            pts.push_back({(R + r*ca)*cb, (R + r*ca)*sb, r*sa});
            nrm.push_back({ca*cb, ca*sb, sa});
        }
    }
}

std::vector<char> render(V3 eye, f tilt, V3 light) {
    std::vector<char> img(W*H, ' ');
    std::vector<f> depth(W*H, 1e30);

    // Work in the torus's own space, so the tilt only rotates the camera
    // and light once per frame instead of every sample point.
    V3 fwd = unit(eye) * -1;
    V3 right = unit(cross(fwd, {0, 1, 0}));

    eye = rotx(eye, -tilt);
    right = rotx(right, -tilt);
    fwd = rotx(fwd, -tilt);
    light = rotx(light, -tilt);

    V3 up = cross(right, fwd);
    V3 half = unit(light + unit(eye));

    for (size_t i = 0; i < pts.size(); i++) {
        V3 rel = pts[i] - eye;
        f x = dot(rel, right), y = dot(rel, up), z = dot(rel, fwd);
        if (z < 0.2) continue;

        int col = int(W/2 + x/z * focal);
        int row = int(H/2 - y/z * focal * aspect);
        if (col < 0 || col >= W || row < 0 || row >= H) continue;

        int k = row*W + col;
        if (z >= depth[k]) continue;

        f diff = 0.5 + 0.5 * dot(nrm[i], light);
        f spec = dot(nrm[i], half);
        spec = spec > 0 ? spec : 0;
        spec *= spec; spec *= spec; spec *= spec;
        spec *= spec; spec *= spec; spec *= spec;   // ^64

        f bright = 0.10 + 0.75*diff + 0.15*spec;
        int level = int(bright * 12 + 0.5);
        if (level > 12) level = 12;

        img[k] = shade[level];
        depth[k] = z;
    }
    return img;
}

void show(const std::vector<char>& img) {
    fputs("\x1b[H", stdout);
    for (int row = 0; row < H; row++) {
        fwrite(&img[row*W], 1, W, stdout);
        if (row + 1 < H) putchar('\n');
    }
    fflush(stdout);
}

#ifdef _WIN32
static void enable_ansi() {
    HANDLE out = GetStdHandle(STD_OUTPUT_HANDLE);
    DWORD mode = 0;
    if (GetConsoleMode(out, &mode))
        SetConsoleMode(out, mode | ENABLE_VIRTUAL_TERMINAL_PROCESSING);
}
static void pause_ms(int ms) { Sleep(ms); }
#else
static void enable_ansi() {}
static void pause_ms(int ms) {
    std::this_thread::sleep_for(std::chrono::milliseconds(ms));
}
#endif

int main() {
    enable_ansi();
    init_torus();
    fputs("\x1b[2J\x1b[?25l", stdout);

    for (f a = 0; a > -1; a += 0.1) {
        f tilt = 0.55 + 0.15 * sin(a*0.5);
        f la = 1.3*a + 1;
        V3 eye = {4*sin(a), 0.9, 4*cos(a)};
        V3 light = unit({0.8*sin(la), 0.9, 0.8*cos(la)});
        show(render(eye, tilt, light));
        pause_ms(30);
    }

    fputs("\x1b[?25h\n", stdout);
}
