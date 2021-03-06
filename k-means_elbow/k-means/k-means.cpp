// k-means.cpp : This file contains the 'main' function. Program execution begins and ends there.
// By Fridtjof Lieberwirth

#include "pch.h"
#include <iostream>

#include <fstream>
#include <string>
#include <sstream>
#include <list>
#include <math.h>
#include <map>
#include <vector>
#include <algorithm>
#include <iomanip>
#include <cmath>
#include <random>

#include <stdio.h>
#include <ctype.h>

#include <chrono>

using namespace std;

struct Datapoint
{
	Datapoint() { _x = -1; _y = -1; _dist = 0; }
	Datapoint(double x, double y, double dist = 0) {
		_x = x;
		_y = y;
		_dist = dist;
	}
	double _x;
	double _y;
	double _dist;
};

struct Cluster {
	Cluster(Datapoint centroid, list<Datapoint> datapoints) {
		_centroid = centroid;
		_datapoints = datapoints;
	}
	Datapoint _centroid;
	list<Datapoint> _datapoints;

	auto begin() {
		return _datapoints.begin();
	}
	auto end() {
		return _datapoints.end();
	}
};

list<Datapoint> points;
void readFile() {
	char ch;
	fstream fin("gauss.txt", fstream::in);
	string p, x;
	while (fin >> noskipws >> ch) {
		if (ch != ' ' && ch != '\n') {
			p += ch;
		}
		else if (ch == ' ') {
			x = p;
			p.clear();
		}
		else {
			points.push_back({ stod(x), stod(p) });
			p.clear();
			x.clear();
		}

	}
	fin.close();
}

double dist(Datapoint p, Datapoint q) {
	double res = sqrt(pow((p._x - q._x),2.0) + pow((p._y - q._y),2));
	return res;
}
double** getMinMax() {
	double minX = 0, minY = 0, maxX = 0, maxY = 0;
	for (Datapoint it : points) {
		if (it._x < minX)
			minX = it._x;
		else if (it._x > maxX)
			maxX = it._x;
		if (it._y < minY)
			minY = it._y;
		else if (it._y > maxY)
			maxY = it._y;
	}
	double** res = 0;
	res = new double*[2];
	res[0] = new double[2];
	res[1] = new double[2];
	res[0][0] = minX;
	res[0][1] = minY;
	res[1][0] = maxX;
	res[1][1] = maxY;

	return res;
}

double sseDist(Datapoint p, Datapoint q) {
	return pow(dist(p, q), 2.0);
}

Datapoint randomCentroids(double** minMax) {
	double x[2];
	std::random_device rd;  //Will be used to obtain a seed for the random number engine
	std::mt19937 gen(rd()); //Standard mersenne_twister_engine seeded with rd()
	std::uniform_real_distribution<> dis(minMax[0][0], minMax[1][0]);
	for (int i = 0; i < 2; ++i) 
		x[i] = (double)dis(gen);

	return {x[0], x[1]};
}

list<Cluster> clusters;
void distToCentroidsStore(const int k, map<int, Datapoint> centroid) {
	map<int, list<Datapoint>> dataInCluster;
	// create map elements according to number of centroids
	for (int i = 0; i < k; ++i)
		dataInCluster.insert(pair<int, list<Datapoint>>( i, NULL ));

	list<Datapoint> tempList;

	// calc distance of every point to the differnt centroids
	for (Datapoint itP : points) {
		double minDist[2];
		minDist[0] = 0;
		minDist[1] = dist(itP, centroid[0]);
		for (int i = 1; i < k; ++i) {
			if (dist(itP, centroid[i]) < minDist[1]) {
				minDist[0] = i;
				minDist[1] = dist(itP, centroid[i]);
			}
		}
		// add point to according map element
		map<int, list<Datapoint>>::iterator itDataInCluster;
		itDataInCluster = dataInCluster.find((int)minDist[0]);
		if (itDataInCluster != dataInCluster.end()) {
			itDataInCluster->second.push_back({ itP._x, itP._y, minDist[1] });
		}
	}
	//add point maps to clusters
	clusters.clear();
	for (int i = 0; i < k; ++i) {
		clusters.push_back({ centroid[i], dataInCluster[i] });
	}
}

Datapoint calcNewCentroid(Cluster itCluster) {
	Datapoint newCentroid;
	int countPoints = 0;
	double sumX = 0, sumY = 0;
	for (Datapoint itP : itCluster) {
		sumX += itP._x;
		sumY += itP._y;
		++countPoints;
	}
	newCentroid = { sumX / countPoints, sumY / countPoints };
	return newCentroid;
}

int main() {
	const int maxIt = 500;

	cout.precision(17);

	cout << "Reading file..." << endl;
	auto begin = chrono::high_resolution_clock::now();
	readFile();
	auto end = chrono::high_resolution_clock::now();
	auto dur = end - begin;
	auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(dur).count();
	cout << "File succesfully read in " << ms << "ms" << endl;

	double avDistortion[9];
	for (int k = 2; k <= 10; k++) {
		cout << "\n----Config----" << endl;
		string strK = "K = ", strM = "Max Iterations = ";
		cout << strK << setw(20 - strK.size()) << k << endl;
		cout << strM << setw(20 - strM.size()) << maxIt << endl;

		map<int, Datapoint> newcentroid, oldcentroid;
		cout << "\n----Random Centroids----" << endl;
		double** minMax = getMinMax();
		cout << "MinX: " << minMax[0][0] << "   MinY: " << minMax[0][1] << endl;
		cout << "MaxX: " << minMax[1][0] << "   MaxY: " << minMax[1][1] << "\n" << endl;
		////Random Centroids
		for (int i = 0; i < k; i++) {
			Datapoint raCen = randomCentroids(minMax);
			//cout << "\r" << word.first << setw(50 - word.first.size()) << "IDF[" << word.second << "]";
			ostringstream sstr;
			sstr << raCen._x;
			string sizeD = sstr.str();
			cout << "\r" << i << ": " << raCen._x << setw(40 - sizeD.size()) << raCen._y << endl;
			cout.width(5);
			newcentroid.insert({ i, raCen });
		}
		cout << endl;

		cout << "\n----Distances----" << endl;
		for (int i = 0; i < maxIt; ++i) {
			cout << "Iteration: " << i << endl;
			////Distance to Centroids and store in Clusters
			distToCentroidsStore(k, newcentroid);
			oldcentroid = newcentroid;
			int j = 0;
			for (Cluster it : clusters) {
				newcentroid[j] = calcNewCentroid(it);
				++j;
			}

			/////// Break Action
			int breaksum = 0;
			for (int i = 0; i < k; ++i) {
				if (dist(oldcentroid[i], newcentroid[i]) < 0.0001)
					++breaksum;
				cout << i << ": " << dist(oldcentroid[i], newcentroid[i]) << endl;
			}
			cout << "\n";
			if (breaksum >= k) {
				cout << "Centroids did not change more than 0.0001" << endl;
				cout << "Iteration: " << i << endl;
				cout << "Breaksum: " << breaksum << " " << endl;
				for (int i = 0; i < k; ++i) {
					cout << "old centroid: " << oldcentroid[i]._x << " " << oldcentroid[i]._y << endl;
					cout << "new centroid: " << newcentroid[i]._x << " " << newcentroid[i]._y << endl;
				}
				break;
			}
			////////
		}

		////// Ellbow Method
		avDistortion[k - 2] = 0;
		map<int, double> distortion;
		int i = 0;
		for (Cluster it : clusters) {
			distortion.insert({ i, 0 });
			map<int, double>::iterator itDistortion;
			itDistortion = distortion.find(k - 2);
			for (auto& itP : points) {
				if (itDistortion != distortion.end())
					itDistortion->second += sseDist(itP, it._centroid);
			}
			++i;
		}
		int count = 0;
		double sum = 0;
		for (auto& it : distortion) {
			sum += it.second;
			++count;
		}
		avDistortion[k - 2] = sum / count;

	}

	double perc = 0;
	double oldDistortion = 0;

	cout << "\n----Distortion per K----" << endl;
	for (int i = 2; i <= 10; ++i) {
		double res;
		oldDistortion > 0 ? res = avDistortion[i - 2] * 100 / oldDistortion : res = 0;
			
		cout << setprecision(17) << "K = " << i << " | D = " << avDistortion[i - 2];
		cout << setprecision(5) << " | P = " << res << " %" << endl;
		oldDistortion = avDistortion[i - 2];
	}

	
	
	/*
	for (auto& itCl : clusters) {
		cout << "---" <<  itCl._centroid._x << " " << itCl._centroid._y << "---" << endl;
		for (auto& itP : itCl) {
			cout << itP._x << " " << itP._y << " " << itP._dist << endl;
		}
		cout << "\n" << endl;
	}*/
	return 0;
}
