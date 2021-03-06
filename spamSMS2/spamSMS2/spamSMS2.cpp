// spamSMS2.cpp : This file contains the 'main' function. Program execution begins and ends there.
//By Fridtjof Lieberwirth

#include "pch.h"
//#include <iostream>

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

#include <stdio.h>
#include <ctype.h>

#include <chrono>

using namespace std;

struct MessageWord {
	MessageWord() { word = "-"; }
	MessageWord(string _word)
	{
		word = _word;
	}
	string word;
};

struct MessageWordTF {
	MessageWordTF() { word = "-"; tf = -1; tfIdf = -1; }
	MessageWordTF(string _word, double _tf = -1, double _tfIdf = -1)
	{
		word = _word;
		tf = _tf;
		tfIdf = _tfIdf;
	}
	string word;
	double tf;
	double tfIdf;
};

struct Message {
	Message() {}
	Message(bool _cat, unsigned int _noOfWords, list<MessageWord> _messageWords, list<MessageWordTF> _messageWordTF = {}, double _sumTFIDF = 0.0)
	{
		cat = _cat;
		noOfWords = _noOfWords;
		sumTFIDF = _sumTFIDF;
		messageWords = _messageWords;
		messageWordTF = _messageWordTF;
	}
	auto beginWords() {
		return messageWords.begin();
	}
	auto endWords() {
		return messageWords.end();
	}
	auto beginWordUni() {
		return messageWordTF.begin();
	}
	auto endWordUni() {
		return messageWordTF.end();
	}
	bool cat;
	unsigned int noOfWords;
	list<MessageWord> messageWords;
	list<MessageWordTF> messageWordTF;
	double sumTFIDF;
};

struct WordEquals
{
	bool operator () (MessageWordTF & rhs) const
	{
		return (word.compare(rhs.word) == 0);
	}

	WordEquals(string n) : word(n) {}

private:

	string word;
};

struct Message_moreThan
{
	bool operator()(Message const & a, Message const & b) const
	{
		return a.sumTFIDF > b.sumTFIDF;
	}
};

void printProgBar(int percent, const int limit, double time) {
	std::string bar;

	double x = 100 / limit;
	for (int i = 0; i < limit; i++) {
		if (i < (percent / x)) {
			bar.replace(i, 1, "=");
		}
		else if (i == (percent / x)) {
			bar.replace(i, 1, ">");
		}
		else {
			bar.replace(i, 1, " ");
		}
	}

	std::cout << "\r" "[" << bar << "] " << percent << "%  " << time/1000 << "s  " << flush;
	//std::cout.width(3);
	//std::cout << percent << "% " << std::flush;
}

list<Message> sms;
unsigned int totalWords = 0;
map<string, double> dict;
vector<string> stopWord;

void StopWordList() {
	
	string wordbuff;
	ifstream ip("stopWordlist.txt");
	if (!ip.is_open()) std::cout << "ERROR: File could not be opened" << '\n';
	while (ip.good()) {
		getline(ip, wordbuff, '\n');
		stopWord.push_back(wordbuff);
	}
	ip.close();
}

void ReadFile() {
	////// Read CSV File into sms list
	char charStop[] = { ' ',
							'\t',
							'\r',
							'\f',
							',',
							'.',
							':',
							';',
							'?',
							'!',
							'[',
							']',
							'|',
							39, // '''
							'(',
							')',
							'*',
							'-',
							92, // '\'
							'"',
							'/'};
	char ch;
	fstream fin("spam.csv", fstream::in);
	map<string, double>::iterator itDic;

	bool category;
	
	while (fin >> noskipws >> ch) {
		string cat;
		cat += ch;
		while (fin >> noskipws >> ch) {
			if (ch != ' ')
				cat += ch;
			else    break;
		}
		if (cat.compare("ham") == 0) category = true;
		else if (cat.compare("spam") == 0) category = false;

		list<MessageWord> wordsInMessage;
		list<MessageWordTF> wordsUniMessage;
		unsigned int numberOfWords = 0;
		bool messageComplete = false;
		while (!messageComplete && fin >> noskipws >> ch) {
			string word;
			char * p;
			p = find(charStop, charStop + sizeof(charStop), ch);
			if (p == charStop + sizeof(charStop)) {
				if (ch != '\n') {
					if (ch >= 65 && ch <= 90)
						ch = tolower(ch);
					word += ch;
				}
				else break;
			}

			
			while (fin >> noskipws >> ch) {
				p = find(charStop, charStop + sizeof(charStop), ch);
				if (p == charStop + sizeof(charStop)) {
					if (ch != '\n') {
						if(ch >= 65 && ch <= 90){
								ch = tolower(ch);
						}
						word += ch;
					}
					else {
						messageComplete = true;
						break;
					}
				}
				else
					break;
			}
			if (word.size() > 0) {
				vector<string>::iterator it;
				it = find(stopWord.begin(), stopWord.end(), word);
				if (it == stopWord.end()) {
					numberOfWords++;
					totalWords++;
					wordsInMessage.push_back(word);

					// Add word to Unique Wordlist
					if (wordsUniMessage.size() > 0) {
						list<MessageWordTF>::iterator it;
						it = find_if(wordsUniMessage.begin(), wordsUniMessage.end(), WordEquals(word));
						if (it == wordsUniMessage.end())
							wordsUniMessage.push_back({ word });
					}
					else
						wordsUniMessage.push_back({ word });

					//Add word to Unique Dictionary
					if (dict.size() > 0) {
						itDic = dict.find(word);
						if (itDic == dict.end()) {
							dict.insert(pair<string, double>(word, -1));
						}
					}
					else
						dict.insert(pair<string, double>(word, -1));
				}
			}
		}

		sms.push_back({ category, numberOfWords, wordsInMessage, wordsUniMessage });
	}
	fin.close();
}

void IDFScore() {
	// IDF Score
	unsigned int count = 0;
	const int limit = dict.size();

	auto begin = chrono::high_resolution_clock::now();

	for (auto& word : dict) {
		unsigned int worDoc = 0;
		for (auto& message : sms) {
			list<MessageWordTF>::iterator it;
			for (it = message.beginWordUni(); it != message.endWordUni(); it++) {
				if (word.first.compare(it->word) == 0) {
					worDoc++;
					break;
				}
			}
		}
		word.second = log10(sms.size() / worDoc);
		//cout << word.first << " " << word.second << " Number of Docs: " << worDoc << endl;
		count++;
		cout << "\r" << word.first << setw(50-word.first.size()) << "IDF[" << word.second << "]";
		cout.width(150);
		auto end = chrono::high_resolution_clock::now();
		auto dur = end - begin;
		auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(dur).count();
		printProgBar(((double)count / (double)limit)*100, 100, ms);
		//cout << "\n";
		//cout << "\r\n Count: " << count << "/" << limit << " " << ((double)count/(double)limit)*100 << "% " << flush;
	}
	
}

void TFScore(){
	// TF Score
	unsigned int count = 0;
	const int limit = sms.size();

	auto begin = chrono::high_resolution_clock::now();

	for (auto& message : sms) {

		list<MessageWordTF>::iterator uniWord;
		for (uniWord = message.beginWordUni(); uniWord != message.endWordUni(); uniWord++) {
			unsigned int wordCount = 0;
			list<MessageWord>::iterator word;
			for (word = message.beginWords(); word != message.endWords(); word++) {
				if (uniWord->word.compare(word->word) == 0)
					wordCount++;
			}
			uniWord->tf = (double)wordCount / (double)message.noOfWords;
		}
		count++;
		auto end = chrono::high_resolution_clock::now();
		auto dur = end - begin;
		auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(dur).count();
		printProgBar(((double)count / (double)limit) * 100, 100, ms);
	}
}

void TFIDFScore() {
	//TFIDF Score
	unsigned int count = 0;
	const int limit = sms.size();

	auto begin = chrono::high_resolution_clock::now();

	for (auto& message : sms) {
		list<MessageWordTF>::iterator wordUni;
		for (wordUni = message.beginWordUni(); wordUni != message.endWordUni(); wordUni++) {
			map<string, double>::iterator itDict;
			itDict = dict.find(wordUni->word);
			if (itDict != dict.end()) {
				wordUni->tfIdf = wordUni->tf * itDict->second;
				//cout << wordUni->word << " TF[" << wordUni->tf << "] TFIDF[" << wordUni->tfIdf << "] ";
			}
			else
				cout << "\n*********ERROR [" << wordUni->word << "] not found***********" << endl;
		}
		count++;
		auto end = chrono::high_resolution_clock::now();
		auto dur = end - begin;
		auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(dur).count();
		printProgBar(((double)count / (double)limit) * 100, 100, ms);
	}
}

void SumTfIdfScore() {
	// SUM TFIDF Score
	unsigned int count = 0;
	const int limit = sms.size();

	auto begin = chrono::high_resolution_clock::now();

	for (auto& message : sms) {
		double sumTfIdf = 0;
		list<MessageWordTF>::iterator uniWord;
		for (uniWord = message.beginWordUni(); uniWord != message.endWordUni(); uniWord++) {
			sumTfIdf += uniWord->tfIdf;
		}
		message.sumTFIDF = sumTfIdf;
		count++;
		auto end = chrono::high_resolution_clock::now();
		auto dur = end - begin;
		auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(dur).count();
		printProgBar(((double)count / (double)limit) * 100, 100, ms);
	}
}

int main() {

	StopWordList();

	cout << "Reading file..." << endl;
	auto begin = chrono::high_resolution_clock::now();
	ReadFile();
	auto end = chrono::high_resolution_clock::now();
	auto dur = end - begin;
	auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(dur).count();
	cout << "File succesfully read in " << ms << "ms" << endl;

	IDFScore();
	TFScore();
	TFIDFScore();
	SumTfIdfScore();

	sms.sort(Message_moreThan());

	cout << "\n" << endl;
	int spamComp = 0, hamComp = 0, i = 0;
	while(spamComp < 10 || hamComp < 10) {
		list<Message>::iterator MeIt = sms.begin();

		advance(MeIt, i);
		if (MeIt->cat == true && hamComp < 10) {
			cout << MeIt->sumTFIDF << "\tham: ";
			list<MessageWord>::iterator WoIt;
			for (WoIt = MeIt->beginWords(); WoIt != MeIt->endWords(); WoIt++)
				cout << WoIt->word << " ";
			cout << "\n";
			hamComp++;
		}
		else if (MeIt->cat == false && spamComp < 10) {
			cout << MeIt->sumTFIDF << "\tspam: ";
			list<MessageWord>::iterator WoIt;
			for (WoIt = MeIt->beginWords(); WoIt != MeIt->endWords(); WoIt++)
				cout << WoIt->word << " ";
			cout << "\n";
			spamComp++;
		}
		i++;
	}
	
	
	
	return 0;
}
