#include <sstream>
#include "board.h"

using namespace std;

Board::Board(int size): size(size), tick(0) {
	id = genid();
}

void Board::nextTick(void) {
	for (Bot &bot : bots) {
		bot.nextTick();
	}

	tick++;
}

string Board::render(void) {
	stringstream ss;
	int x,y;
	ss<<'+';
	for(x=0;x<size;x++)ss<<"--+";
	ss<<endl;
	for(y=0;y<size;y++){
		ss<<'|';
		for(x=0;x<size;x++)ss<<"  |";
		ss<<endl<<'|';
		for(x=0;x<size;x++)ss<<"  |";
		ss<<endl;
		ss<<'+';
		for(x=0;x<size;x++)ss<<"--+";
		ss<<endl;
	}
	string s(move(ss.str()));
	int idx,dir;
	pair<int,int> pos;
	const char dirchars[4][4]={
		{'/' ,' ' ,' ' ,'/' },
		{'\\','\\',' ' ,' ' },
		{' ' ,' ' ,'\\','\\'},
		{' ' ,'/' ,'/' ,' ' }
	};
	for(Bot &b : bots){
		pos=b.getPos();
		dir=b.getDir();
		idx=3*size+1+3*(3*size+1)*pos.second+3*pos.first+1;
		s[idx]=dirchars[dir][0];
		s[idx+1]=dirchars[dir][1];
		s[idx+3*size+1]=dirchars[dir][2];
		s[idx+3*size+1+1]=dirchars[dir][3];
	}
	return s;
}

bool Board::canMoveTo(int x, int y) {
	for (Bot &bot : bots) {
		pair<int, int> location = bot.getPos();

		if (location.first == x && location.second == y) {
			return false;
		}
	}
	return true;
}