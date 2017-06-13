// original author: Sophie Kirschner
// ported to C by: stasoid
// https://esolangs.org/wiki/Surface
// http://dl.dropbox.com/u/10116881/esoteric/Surface.zip
// License: GPL
/*
Differences from the original:
This version doesn't display messages "Executing <file>", "Execution complete."
This version doesn't wait for input when execution is complete.
This version has stack limit of 1024, which means it can handle 512 nested parens (each paren pushes two values on the stack).
  (original version has no stack limit)
Counting instructions and savecode() are not implemented.

Note: code space is 32 x 16 chars (like in original).
Note: memory space is 32 x 16 cells (like in original).

Functions that are not present in the original: readfile, fsize, getlinelen, main
*/
#include <stdio.h>
#include <stdlib.h>
#include <malloc.h>
#include <string.h>
#include <ctype.h>
#include <sys/stat.h>
#define min(x,y)  ((x)<(y) ? (x) : (y))
char* readfile(const char* fname, int* size);

void push(int val);
int pop();

//`cmds` not used in the original
enum{ cw=32,ch=16 }; // code width/height
int cx=0,cy=0,cr=0;
enum dir{ e,w,n,s } cdir = e;
char code[cw][ch];

enum{ memw=32,memh=16 };
int memx=0,memy=0,memr=0;
int mem[memw][memh];

enum{stacksize = 1024}; // max stack size. unlike in original, stack has limited size here
int stack[stacksize]; // called `parens` in the original
int stktop = -1; // index of the value at the top of the stack, ==-1 when stack is empty, ==stacksize-1 when stack is full.
				// corresponds to p.parens.a.length-1 in the original, where p is local var in execute (Local p:prog)
int skips=0;
int done=0;
//`output` - I don't use it, print directly to console
//`generation` not used in the original
//`run_time` - I do not count instructions in this version

void movemem(int xmod, int ymod)
{
	memx+=xmod; memy+=ymod*(memr*2-1);
	if (memx<0) memx+=memw;
	memx%=memw;
	if (memy<0)      {memx=(memx+memw/2) % memw; memy=0;      memr=!memr;}
	if (memy>memh-1) {memx=(memx+memw/2) % memw; memy=memh-1; memr=!memr;}
}

void moveip(int xmod, int ymod)
{
	cx+=xmod; cy+=ymod*((!cr)*2-1);
	if (cx<0) cx+=cw;
	cx%=cw;
	if (cy<0)    {cx=(cx+cw/2) % cw; cy=0;    cr=!cr;}
	if (cy>ch-1) {cx=(cx+cw/2) % cw; cy=ch-1; cr=!cr;}
}

void rotateip(int clockwise)
{
	//                e w n s  ->
	enum dir cw[]  = {s,n,e,w};
	enum dir ccw[] = {n,s,w,e};
	cdir = clockwise ? cw[cdir] : ccw[cdir];
}

void updateip()
{
	switch(cdir) {
		case e: moveip(1,0); break;
		case w: moveip(-1,0); break;
		case n: moveip(0,-1); break;
		case s: moveip(0,1); break;
	}
}

void update()
{
	//if (done) return; // no need - see run()
	if (skips==0)
	{
		switch (code[cx][cy])
		{
			case '<': cdir=w; movemem(-1,0); break;
			case '>': cdir=e; movemem(1,0); break;
			case '^': cdir=n; movemem(0,-1); break;
			case 'v': cdir=s; movemem(0,1); break;
			case '+': mem[memx][memy]++; break;
			case '-': mem[memx][memy]--; break;
			case 'o': rotateip(1); break;
			case 'e': rotateip(0); break;
			case 'c': rotateip(1); code[cx][cy]='z'; break;
			case 'z': rotateip(0); code[cx][cy]='c'; break;
			case '\\':
				code[cx][cy]='/';
				//             e w n s  ->
				cdir = (int[]){s,n,w,e}[cdir];
				break;
			case '/':
				code[cx][cy]='\\';
				//             e w n s  ->
				cdir = (int[]){n,s,e,w}[cdir];
				break;
			case '?': if (mem[memx][memy]<1) skips++; break;
			case '!': if (mem[memx][memy]>0) skips++; break;
			case '*': skips=mem[memx][memy]; break;
			case '(': push(cx);push(cy); break;
			case ')': cy=pop();cx=pop();push(cx);push(cy); break;
			case ']': if (mem[memx][memy]>0) {cy=pop();cx=pop();push(cx);push(cy);} break;
			case 'x': pop();pop(); break;
			case '.': putchar(mem[memx][memy]); break; // modulo 256 is unnecessary here because putchar converts its argument into unsigned char anyway, see http://www.cplusplus.com/reference/cstdio/putchar/
			case ':': printf("%d", mem[memx][memy]); break;
			case ',': scanf("%d", &mem[memx][memy]); break;
			case '@': done=1; return; // we can just exit(0) here and get rid of `done`
		}
	}
	else
		skips--;
	updateip();
}

int getlinelen(char* str, int len)
{
	int i;
	for(i=0; i<len && str[i]!='\n'; i++);
	// if we searched entire str and did not find any newlines (i==len) then number of chars in the line is len (entire str)
	// otherwise it is i+1 (\n included)
	return i==len ? len : i+1;
}

// this should work with files containing null bytes
// readfile adds null byte, but readcode does not rely on that
void readcode(char* str, int len)
{
	int y=0;
	char* line = str;
	int linelen = getlinelen(str, len);
	while(y<ch)
	{
		int maxx = min(linelen,cw);
		for(int x=0; x<maxx; x++)
			code[x][y] = tolower(line[x]);
		y++;
		line += linelen;
		len -= linelen;
		if(len <= 0) break;
		linelen = getlinelen(line, len);
	}
}

// does nothing useful, all vars are already initialized with correct values at this point
/*void reset()
{
	cx=0;cy=0;cr=0;cdir=e;
	memx=0;memy=0;memr=0;
	memset(mem, 0, sizeof(mem));
	memset(stack, 0, sizeof(stack));
	stktop = -1;
	skips=0;done=0;
}*/

void run()
{
	//reset();
	while (!done) update();
}

void open(char* filename)
{
	int len=0;
	char* str = readfile(filename, &len);
	if (!str) { printf("Cannot read file %s", filename); exit(1); }
	readcode(str, len);
}

void execute(char* filename)
{
	open(filename);
	run();
}

//savecode() - not implemented

void push(int val)
{
	// unlike in original, stack has limited size here, so it can overflow
	if(stktop == stacksize-1) { printf("Stack overflow"); exit(1); }
	stack[++stktop] = val;
}

int pop()
{
	if(stktop == -1) return 0; // no stack underflow, see original
	return stack[stktop--];
}


int main(int argc, char* argv[])
{
	if(argc == 1) { printf("Usage: surface <filename>"); return 1; }
	execute(argv[1]);
	return 0;
}


//====================================================================================================================================
// utility functions

static int fsize(const char *filename)
{
	struct stat st;

	if (stat(filename, &st) == 0)
		return st.st_size;

	return -1;
}

// adds null byte, but also optionally returns size
char* readfile(const char* fname, int* _size)
{
	int size;
	char* buf;

	FILE* f = fopen(fname, "rb");
	if (!f) return 0;
	size = fsize(fname);
	buf = (char*)malloc(size + 1);
	fread(buf, 1, size, f);
	buf[size] = 0;
	fclose(f);
	if(_size) *_size = size;
	return buf;
}

//====================================================================================================================================
/*
Original is written in BlitzMax (dialect of Basic).
BlitzMax cheatsheet:

Sigils: (https://en.wikipedia.org/wiki/Sigil_(computer_programming))
  $ string
  % integer
  @ I don't know, probably char or byte

(local BlitzMax docs):
Operators: 
  :+  +=
Expressions:
  =  ==
  Asc returns the character value of the first character of a string, or -1 if the length of the string is 0.
  Chr constructs a 1 character string with the specified character value.
Functions:
  Function name:rettype(params)
  ...
  End Function
  If ReturnType is omitted, the function defaults to returning an Int.
Program flow:
  'one liner' style of If/Then statement:
  If Expression Then Statements Else Statements
Data types:
  Int - 32 bit signed integer
*/

