#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <cstring>
#include <unistd.h>
#include <sys/time.h>
#include "lang/parameters.h"
#include "lang/parser.h"
#include "board.h"
#include "bot.h"
#include "botdist.h"

using namespace std;

extern Params params;

void clearScreen(void) {
	cout << "\x1B[2J\x1B[H" << flush;
}

vector<string> readFile(const char *const fname) {
	ifstream f(fname);
	vector<string> lines;
	if (!!f) {
		/*f.seekg(0,ios_base::end);
		int filelen=f.tellg();
		f.seekg(0,ios_base::beg);
		string buf;
		buf.resize(filelen);
		f.read(buf.data(),filelen);
		f.close();*/
		string line;
		while (f) {
			getline(f, line);
			lines.push_back(move(line));
		}
		return lines;
	} else {
		char *message;
		asprintf(&message, "Can't read program '%s', are you sure it exists?", fname);
		throw message;
	}
}

void printusage(int argc, char **argv) {
	cerr << "Usage: " << argv[0] << " [<options>] <botprograms>" << endl;
	cerr << "Options:" << endl;
	cerr << "\t--boardsize=<int> (5) | Sets the size of the game board." << endl;
	cerr << "\t--maxbotmemory=<int> (50) | Sets the max memory a bot can store." << endl;
	cerr << "\t--maxpages=<int> (16) | Sets the max pages a program can have." << endl;
	cerr << "\t--sleeptime/waittime=<int> (20) | Sets the amount of time to wait between each tick in milliseconds." << endl;
	cerr << "\t--maxticks=<int> (10000) | Sets the amount of ticks that may elapse before the game is stopped as a tie. -1 for unlimited" << endl;
	cerr << "\t--resultonly | Only print the result of the match on stdout (Other stuff will still be printed on cerr)." << endl;
	cerr << "\t--parseonly | Quits after parsing the program(s)." << endl;
	cerr << "\t--allowdebug | Allow bots to call debugging functions." << endl;
}

bool parseFlagOption(const string &s) { // True if flag, otherwise false.
	if (s.find("--") == 0) {
		string trimmed = trim(s.substr(2, s.size() - 2));
		vector<string> splitted = split(trimmed, '=', 1);

		auto requireParam = [&splitted] () {
			if (splitted.size() != 2) {
				char *message;
				asprintf(&message, "Flag '%s' requires a parameter.", splitted[0].c_str());
				throw message;
			}
		};

		if (splitted[0] == "maxbotmemory") {
			requireParam();
			params.maxBotMemory = stoi(splitted[1]);
		} else if (splitted[0] == "boardsize") {
			requireParam();
			params.boardSize = stoi(splitted[1]);
		} else if (splitted[0] == "maxpages") {
			requireParam();
			params.maxPages = stoi(splitted[1]);
		} else if (splitted[0] == "sleeptime" || splitted[0] == "waittime") {
			requireParam();
			params.sleepTime = stoi(splitted[1]);
		} else if (splitted[0] == "maxticks") {
			requireParam();
			params.maxTicks = stoi(splitted[1]);
		} else if (splitted[0] == "parseonly") {
			params.parseOnly = true;
		} else if (splitted[0] == "resultonly") {
			params.resultOnly = true;
		} else if (splitted[0] == "allowdebug") {
			params.allowDebug = true;
		} else {
			char *message;
			asprintf(&message, "Unknown flag '%s'.", splitted[0].c_str());
			throw message;
		}

		return true;
	} else {
		return false;
	}
}

int main(int argc, char **argv) {
	if (argc == 1) {
		printusage(argc, argv);
		return 1;
	}

	try {
		vector<string> programNames;
		int i;
		bool ignoreFlags = false;
		for (i = 1; i < argc; i++) {
			try {
				if (string(argv[i]) == string("--")) {
					ignoreFlags = true;
					continue;
				} else if (!ignoreFlags && parseFlagOption(argv[i])) {
					// Correct flag given. Continue since this isn't a bot.
					continue;
				}
			} catch (const char *str) { // Unknown flag given.
				cerr << str << endl << endl;
				printusage(argc, argv);
				return 1;
			}

			programNames.push_back(argv[i]);
		}

		Board board(params.boardSize);
		vector<Parser::Program> programs;
		programs.reserve(programNames.size());

		for (i = 0; i < (int)programNames.size(); i++) {
			const string &programName = programNames[i];

			cerr << "Reading in program " << i << ": '" << programName << "'... ";

			try {
				vector<string> contents;
				try {
					contents = readFile(programName.c_str());
				} catch (const char *str) {
					cerr << str << endl << endl;

					printusage(argc, argv);
					return 1;
				}

				cerr << "Read contents... ";
				Parser::Program program = Parser::parse(programName.c_str(), contents);
				cerr << "Parsed... ";
				programs.push_back(program);
			} catch (const char *str) {
				cerr << "ERROR: " << str << endl;
				return 1;
			}
			board.bots.emplace_back(&programs[programs.size() - 1], &board, make_pair(0, 0), board.bots.size());
			cerr << "Done." << endl;
		}
		const int numprogs = programs.size();

		cerr << "Done reading in programs" << endl;

		struct timeval tv;
		gettimeofday(&tv, NULL);
		srand(tv.tv_sec * 1000000 + tv.tv_usec);
		vector<int> botDist = makeBotDistribution(params.boardSize, params.boardSize, board.bots.size());
		for (i = 0; i < (int)botDist.size(); i++) {
			int loc = botDist[i];
			Bot &bot = board.bots[i];

			bot.x = loc % params.boardSize;
			bot.y = loc / params.boardSize;
			bot.dir = rand() % 4;
		}

		if (params.parseOnly) {
			return 0;
		}

		int progid;
		bool stillthere[numprogs];

		if (!params.resultOnly) {
			clearScreen();
			cout << board.render() << endl;
			usleep(1000 * 1000);
		}

		while (true) {
			memset(stillthere, 0, numprogs * sizeof(bool));
			bool anyNotAsleep = false;
			for (Bot &b : board.bots) {
				if (!b.isAsleep) anyNotAsleep = true;
				progid = b.program->id;

				for (i = 0; i < numprogs; i++) {
					if (programs[i].id == progid) {
						stillthere[i] = true;
						break;
					}
				}
			}

			int stillthereCount = 0;
			for (i = 0; i < numprogs; i++) {
				if (stillthere[i]) {
					stillthereCount++;
				} else {
					//cerr | update << "Program " << i << '(' << programs[i].name << ") has no bots left!" << endl;
				}
			}

			if (params.maxTicks > 0 && board.currentTick() >= params.maxTicks) {
				cerr << "Game elapsed for more than the max amount of ticks (" << params.maxTicks << "). Game ended as a tie." << endl;
				cerr << "Left bots:" << endl;
				for (i = 0; i < numprogs; i++) {
					if (stillthere[i]) {
						cerr << "> " << programs[i].name << endl;
					}
				}

				if (params.resultOnly) {
					cout << "T" << endl;
				}

				return 0;
			}

			if (stillthereCount <= 1) {
				cerr << "Only " << stillthereCount << " program" << (stillthereCount == 1 ? "" : "s") << " left, closing game" << (stillthereCount == 0 ? '.' : ':') << endl;
				for (i = 0; i < numprogs; i++) {
					if (stillthere[i]) {
						cerr << "> " << programs[i].name << endl;
					}
				}

				if (params.resultOnly) {
					if (stillthereCount == 1) {
						for (i = 0; i < numprogs; i++) {
							if (stillthere[i]) {
								cout << "W-" << i << endl;
							}
						}
					} else {
						cout << "T" << endl;
					}
				}

				break;
			}

			if (!anyNotAsleep) {
				cerr << "All bots are sleeping. Closing game due to inactivity." << endl;
				if (params.resultOnly) {
					cout << "T" << endl;
				}
				break;
			}

			board.nextTick();

			if (!params.resultOnly) {
				clearScreen();
				cout << board.render() << endl;
				cout << "tick " << board.currentTick() << endl;
				usleep(params.sleepTime * 1000);
			}
		}
	} catch (char *msg) {
		cerr << "ERROR CAUGHT: " << msg << endl;
		return 1;
	}

	return 0;
}
