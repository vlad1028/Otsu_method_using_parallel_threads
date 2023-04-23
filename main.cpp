#include <iostream>
#include <fstream>
#include <vector>
#include <algorithm>
#include <string>
#include <stdio.h>
#include "omp.h"

#define SCHDL static, 4

using namespace std;

const int L = 256;
const int M = 4;

int threadsNum;

vector<int> prefQ;
vector<int> prefM;

vector<unsigned char> intensity;

bool isPNM(ifstream &fin) {
    char c, t;
    fin >> c >> t;
    if (c != 'P' || t != '5') {
        cout << "Wrong file format." << endl;
        return false;
    }
    return true;
}

ifstream openPNM(const string &inputFileName) {
    ifstream fin(inputFileName);

    if (!isPNM(fin)) {
        cout << "input file name: '" << inputFileName << "'\n";
        exit(0);
    }

    return fin;
}

vector<unsigned char> getIntensity(ifstream &fin, int size) {
    int t;
    fin >> t;
    intensity.resize(size);
    for (auto &c : intensity)
        fin >> noskipws >> c;
    return intensity;
}

void writeIntensity(const string &fileName, int weight, int height) {
    ofstream fout(fileName);

    fout << "P5\n" << weight << ' ' << height << "\n255\n";
    for (auto i : intensity)
        fout << i;

    fout.flush();
    fout.close();
}

vector<int> calcCntOmp() {
    vector<int> cnt(L, 0);
#pragma omp parallel num_threads(threadsNum) shared(intensity)
    {
        vector<int> local_cnt(L, 0);
#pragma omp for schedule(SCHDL)
        for (int i = 0; i < intensity.size(); ++i) {
            ++local_cnt[intensity[i]];
        }

        for (int i = 0; i < L; ++i) {
#pragma omp atomic
            cnt[i] += local_cnt[i];
        }
    }
    return cnt;
}

vector<int> calcCnt() {
    vector<int> cnt(L, 0);
    for (auto i : intensity)
        ++cnt[i];
    return cnt;
}

int calcIntensity(const vector<int> &l, int val) {
    if (val <= l[0])
        return 0;
    if (val <= l[1])
        return 84;
    if (val <= l[2])
        return 170;
    return 255;
}

vector<unsigned char> convertOmp(const vector<int> &l) {
    vector<unsigned char> converted(intensity.size());

#pragma omp parallel for shared(converted, intensity)
    for (int i = 0; i < intensity.size(); ++i)
        converted[i] = calcIntensity(l, intensity[i]);

    return converted;
}

vector<unsigned char> convert(const vector<int> &l) {
    vector<unsigned char> converted(intensity.size());

    for (int i = 0; i < intensity.size(); ++i)
        converted[i] = calcIntensity(l, intensity[i]);

    return converted;
}

void calcPrefQ(const vector<int> &cnt) {
    prefQ.resize(cnt.size(), 0);
    prefQ[0] = cnt[0];
    for (int i = 1; i < prefQ.size(); ++i)
        prefQ[i] = prefQ[i - 1] + cnt[i];
}

void calcPrefM(const vector<int> &cnt) {
    prefM.resize(cnt.size(), 0);
    for (int i = 1; i < prefM.size(); ++i)
        prefM[i] = prefM[i - 1] + i * cnt[i];
}

int calcQ(int l, int r) { // ans == (q * N)
    if (l == 0)
        return prefQ[r];
    return prefQ[r] - prefQ[l - 1];
}

int calcM(int l, int r) { // ans == (m * q)
    if (l == 0)
        return prefM[r];
    return prefM[r] - prefM[l - 1];
}

double calcSig(const vector<int> &levels) {
    int left = 0;
    double sig = 0;
    for (auto right : levels) {
        int q = calcQ(left, right);
        if (q == 0)
            return 0;
        int m = calcM(left, right);
        sig += (double)m * m / q;

        left = right + 1;
    }
    return sig;
}

void parseArgs(int argc, const char* argv[]) {
    if (argc != 4) {
        cout << "Wrong number of arguments. Need 4, found " << argc << endl;
        exit(0);
    }

//   string s = string(argv[0], argv[1] - 1);
//   if (s.size() < 5 || s.substr(s.size() - 5) != "/omp4") {
//       cout << "arg[0] must be 'omp4'. It is '" << s << "'" << endl;
//       exit(0);
//   }

    try {
        threadsNum = stoi(string(argv[1], argv[2] - 1));
    } catch (invalid_argument const &ex) {
        cout << "arg[1] must be integer." << endl;
        exit(0);
    }
}

vector<int> ompSolution() {
    vector<int> cnt = calcCntOmp();

#pragma omp parallel sections
    {
#pragma omp section
        {
            calcPrefQ(cnt);
        }
#pragma omp section
        {
            calcPrefM(cnt);
        }
    }

    double bestSig = 0;
    vector<int> bestRes(M - 1, 0);
#pragma omp parallel num_threads(threadsNum)
    {
        double localBestSig = 0;
        vector<int> localBestRes(M - 1, 0);
#pragma omp for schedule(SCHDL)
        for (int l0 = 0; l0 < L; ++l0) {
            for (int l1 = l0 + 1; l1 < L; ++l1) {
                for (int l2 = l1 + 1; l2 < L; ++l2) {
                    double sig = calcSig({l0, l1, l2, 255});

                    if (localBestSig < sig) {
                        localBestSig = sig;
                        localBestRes = {l0, l1, l2};
                    }
                }
            }
        }

#pragma omp critical (updateSig)
        {
            if (bestSig < localBestSig) {
                bestSig = localBestSig;
                bestRes = localBestRes;
            }
        }
    }

    intensity = convertOmp(bestRes);

    return bestRes;
}

vector<int> noOmpSolution() {
    vector<int> cnt = calcCnt();
    calcPrefQ(cnt);
    calcPrefM(cnt);

    double bestSig = 0;
    vector<int> bestRes(M - 1, 0);
    for (int l0 = 0; l0 < L; ++l0) {
        for (int l1 = l0 + 1; l1 < L; ++l1) {
            for (int l2 = l1 + 1; l2 < L; ++l2) {
                double sig = calcSig({l0, l1, l2, 255});

                if (bestSig < sig) {
                    bestSig = sig;
                    bestRes = {l0, l1, l2};
                }
            }
        }
    }

    intensity = convert(bestRes);

    return bestRes;
}

int main(int argc, const char* argv[]) {
    parseArgs(argc, argv);

    if (threadsNum == 0) {
        threadsNum = omp_get_max_threads();
    }

    ifstream fin = openPNM(string(argv[2], argv[3] - 1));

    int weight, height;
    fin >> weight >> height;
    intensity = getIntensity(fin, weight * height); // histogram
    fin.close();

    vector<int> bestRes;
    double start = omp_get_wtime();

    if (threadsNum == -1) {
        bestRes = noOmpSolution();
    } else {
        bestRes = ompSolution();
    }

    double time = (omp_get_wtime() - start) * 1000;

    cout << "Time (" << threadsNum << " thread(s)): " << time << " ms" << endl;
    cout << bestRes[0] << ' ' << bestRes[1] << ' ' << bestRes[2] << endl;

    writeIntensity(string(argv[3]), weight, height);

    return 0;
}