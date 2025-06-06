#include "stdafx.h"
#include "common.h"

#include <opencv2/core.hpp>
#include <opencv2/highgui.hpp>
#include <opencv2/imgproc.hpp>
#include <opencv2/core/utils/logger.hpp>

#include <vector>
#include <iostream>
#include <cmath>
#include <cfloat>

using namespace cv;
using namespace std;

wchar_t* projectPath;

// variabile globale pentru Pasul 3
static Mat sctImg, dispImg;
static Mat segMask;
static int delta = 15;
const int MAX_DELTA = 128;

// --- CALLBACK PENTRU CLICK-URI PE IMAGINE (Cub RGB) ---
void onSegmentClick(int event, int x, int y, int flags, void* userdata)
{
    if (event != EVENT_LBUTTONDOWN) return;

    Vec3b center = sctImg.at<Vec3b>(y, x);
    Vec3b lower, upper;
    for (int c = 0; c < 3; ++c) {
        int v = center[c];
        lower[c] = saturate_cast<uchar>(v - delta);
        upper[c] = saturate_cast<uchar>(v + delta);
    }

    Mat m;
    inRange(sctImg, lower, upper, m);
    segMask |= m;

    // refacem dispImg
    sctImg.copyTo(dispImg);
    for (int i = 0; i < dispImg.rows; ++i)
        for (int j = 0; j < dispImg.cols; ++j)
            if (segMask.at<uchar>(i, j))
                dispImg.at<Vec3b>(i, j) = Vec3b(255, 0, 0);

    imshow("Rezultat SCT-Center (15x15 = 225 culori)", dispImg);
}

// --- PAS 1: FILTRU MEDIAN ---
Mat aplicaFiltruMedian(const Mat& src, int k = 5) {
    Mat dst;
    if (k <= 0 || k % 2 == 0) {
        cout << "Avertisment: kernel impar. Folosesc 5.\n";
        k = 5;
    }
    medianBlur(src, dst, k);
    cout << "Filtru median " << k << "×" << k << " aplicat.\n";
    return dst;
}

// --- PAS 2: TRANSFORMATĂ SCT-CENTER 15×15 ---
Mat aplicaSCTCenter(const Mat& src) {
    Mat dst = src.clone();
    if (src.empty() || src.channels() != 3) {
        cout << "Eroare: imagine RGB necesară!\n";
        return dst;
    }

    int rows = src.rows, cols = src.cols, N = 15;
    vector<vector<double>> angleA(rows, vector<double>(cols)), angleB(rows, vector<double>(cols));
    double minA = DBL_MAX, maxA = -DBL_MAX, minB = DBL_MAX, maxB = -DBL_MAX;

    // 1) calcul unghiuri & min/max
    for (int i = 0; i < rows; i++) for (int j = 0; j < cols; j++) {
        Vec3b p = src.at<Vec3b>(i, j);
        double B = p[0], G = p[1], R = p[2], L = sqrt(B * B + G * G + R * R);
        if (L > 0) {
            angleA[i][j] = atan2(sqrt(G * G + R * R), B);
            angleB[i][j] = atan2(G, R);
            minA = min(minA, angleA[i][j]); maxA = max(maxA, angleA[i][j]);
            minB = min(minB, angleB[i][j]); maxB = max(maxB, angleB[i][j]);
        }
    }
    double stepA = (maxA - minA) / N, stepB = (maxB - minB) / N;

    // 2) acumulare medii
    vector<vector<Vec3d>> rgbMeans(N, vector<Vec3d>(N, Vec3d(0, 0, 0)));
    vector<vector<int>> counts(N, vector<int>(N, 0));
    for (int i = 0; i < rows; i++) for (int j = 0; j < cols; j++) {
        int ia = min(int((angleA[i][j] - minA) / stepA), N - 1),
            ib = min(int((angleB[i][j] - minB) / stepB), N - 1);
        Vec3b p = src.at<Vec3b>(i, j);
        rgbMeans[ia][ib] += Vec3d(p[0], p[1], p[2]);
        counts[ia][ib]++;
    }

    // 3) finalizează medii
    for (int a = 0; a < N; a++) for (int b = 0; b < N; b++)
        if (counts[a][b] > 0) rgbMeans[a][b] /= counts[a][b];

    // 4) reasignează
    for (int i = 0; i < rows; i++) for (int j = 0; j < cols; j++) {
        int ia = min(int((angleA[i][j] - minA) / stepA), N - 1),
            ib = min(int((angleB[i][j] - minB) / stepB), N - 1);
        if (counts[ia][ib] > 0) {
            Vec3d m = rgbMeans[ia][ib];
            dst.at<Vec3b>(i, j) = Vec3b((uchar)m[0], (uchar)m[1], (uchar)m[2]);
        }
    }

    cout << "SCT-Center finalizat (15×15 = 225 culori).\n";
    return dst;
}

// --- FLUX PRINCIPAL ---
void proiect() {
    char fname[MAX_PATH];
    while (openFileDlg(fname)) {
        Mat src = imread(fname);
        if (src.empty()) { cout << "Eroare la încărcare!\n"; continue; }

        // Pasul 1
        Mat pre = aplicaFiltruMedian(src, 3);
        imshow("Dupa Preprocessing (Median 3x3)", pre);

        // Pasul 2
        sctImg = aplicaSCTCenter(pre);

        // Pregătim Pasul 3
        segMask = Mat::zeros(sctImg.size(), CV_8U);
        dispImg = sctImg.clone();
        const char* win = "Rezultat SCT-Center (15x15 = 225 culori)";
        namedWindow(win, WINDOW_AUTOSIZE);
        createTrackbar("Delta", win, &delta, MAX_DELTA);
        setMouseCallback(win, onSegmentClick, nullptr);
        imshow(win, dispImg);
        cout << "\nPasul 3: Click stânga pentru a adăuga ROI în cubul RGB.\n";
        cout << "Ajustează Delta cu slider și apasă ESC când termini.\n";

        // așteptăm ESC
        while (true) {
            if (waitKey(0) == 27) break;
        }
        destroyWindow(win);

        // --- Pasul 4: MORFOLOGIE ---
        Mat kernel = getStructuringElement(MORPH_CROSS, Size(3, 3));
        Mat morphMask;
        erode(segMask, morphMask, kernel);
        dilate(morphMask, morphMask, kernel);

        // imagine binarizată cu ROI în negru pe fundal alb
        Mat binImg(src.size(), CV_8UC3, Scalar(255, 255, 255));
        for (int i = 0; i < morphMask.rows; ++i)
            for (int j = 0; j < morphMask.cols; ++j)
                if (morphMask.at<uchar>(i, j))
                    binImg.at<Vec3b>(i, j) = Vec3b(0, 0, 0);
        imshow("Pasul 4 – Binarizare morfologică", binImg);

        // overlay albastru pe imaginea inițială
        Mat overlay = src.clone();
        for (int i = 0; i < morphMask.rows; ++i)
            for (int j = 0; j < morphMask.cols; ++j)
                if (morphMask.at<uchar>(i, j))
                    overlay.at<Vec3b>(i, j) = Vec3b(255, 0, 0);
        imshow("Pasul 4 – ROI Overlay", overlay);

        // Pasul 5: informare zonă și procent
        int area = countNonZero(morphMask);
        double pct = 100.0 * area / (morphMask.rows * morphMask.cols);
        cout << "\nPasul 5: Zona segmentată = " << area
            << " pixeli (" << fixed << setprecision(2) << pct << "%)\n";
        cout << "Apasă orice tastă pentru următoarea imagine...\n";

        waitKey(0);
        destroyAllWindows();
    }
}

int main() {
    utils::logging::setLogLevel(utils::logging::LOG_LEVEL_FATAL);
    projectPath = _wgetcwd(nullptr, 0);
    proiect();
    return 0;
}
