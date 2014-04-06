/**********
"MarioLANG is a two-dimensional programming language made by User:Wh1teWolf, based on Super Mario."
(quoting http://esolangs.org/wiki/MarioLANG)
This is an interpreter for the language by Tom Smeding.
**********/

#include <iostream>
#include <iomanip>
#include <fstream>
#include <vector>
#include <string>
#include <climits>
#include <cmath>
#include <termios.h>
#include <unistd.h>

#define ERROR_FALSE(s) {SHOW_ERROR(s);return false;}
#define SHOW_ERROR(s) {if(debuglevel>=1)cerr<<"\x1B[31m"<<s<<"\x1B[0m"<<endl;}
#define SHOW_MESSAGE(s) {if(debuglevel>=2)cerr<<"\x1B[36m"<<s<<"\x1B[0m"<<endl;}
#define SHOW_DEBUG(s) {if(debuglevel>=3)cerr<<"\x1B[37m"<<s<<"\x1B[0m"<<endl;}
#define DIRRIGHT (0)
#define DIRUP (1)
#define DIRLEFT (2)
#define DIRNONE (3)
#define TABWIDTH (8)

using namespace std;

int debuglevel,animatedelay;
bool animate,animateShowTape;

template<class T>class negvector;
class mario;

char getch(){char b=0;struct termios o={0};fflush(stdout);if(tcgetattr(0,&o)<0)
perror("tcsetattr()");o.c_lflag&=~ICANON;o.c_lflag&=~ECHO;o.c_cc[VMIN]=1;o.c_cc[
VTIME]=0;if(tcsetattr(0,TCSANOW,&o)<0)perror("tcsetattr ICANON");if(read(0,&b,1)
<0)perror("read()");o.c_lflag|=ICANON;o.c_lflag|=ECHO;if(tcsetattr(0,TCSADRAIN,&
o)<0)perror("tcsetattr ~ICANON");return b;}

void moveto(int x,int y){
	cout<<"\x1B["<<y+1<<";"<<x+1<<"H"<<flush;
}

int max(int a,int b){return a<b?b:a;}

template<class T>class negvector{
	vector<T> pos,neg;
public:
	int size(void){return pos.size()+neg.size();}
	int sizepos(void){return pos.size();}
	int sizeneg(void){return neg.size();}
	void resizepos(int n,T val=T()){pos.resize(n,val);}
	void resizeneg(int n,T val=T()){neg.resize(n,val);}
	T& operator[](int idx){return idx<0?neg[-idx-1]:pos[idx];}
};

class mario{
public:
	int ipx,ipy,memp,dir;
	bool walking,skip;
};

class Level{
private:
	negvector<int> memory;
	vector<string> code;
	int outputx,outputy,inputy,tapex;

	void drawTape(mario *m){
		static int minidx=-1,maxidx=-1;
		static bool inited=false;
		int i,intwidth;
		if(!inited){
			inited=true;
			minidx=0;
			maxidx=0;
		}
		if(m->memp<minidx)minidx=m->memp;
		if(m->memp>maxidx)maxidx=m->memp;
		intwidth=max(1+log10(minidx),log10(maxidx));
		cout<<"\x1B[34m"<<flush; //blue
		for(i=minidx;i<=maxidx;i++){
			moveto(tapex,i-minidx);
			cout<<"\x1B[0K"<<flush;
			cout<<setw(intwidth)<<i<<(m->memp==i?'#':'.')<<' '<<memory[i]<<flush;
		}
		cout<<"\x1B[0m"<<flush;
		moveto(outputx,outputy);
	}

	void animationFrameStart(int x,int y,char c){
		moveto(x,y);
		cout<<"\x1B[41;1m"<<c<<"\x1B[0m"<<flush;
		moveto(outputx,outputy);
	}

	void animationFrameEnd(mario *m,int x,int y,char c){
		usleep(animatedelay);
		moveto(x,y);
		cout<<c<<flush;
		moveto(outputx,outputy);
		if(animateShowTape)drawTape(m);
	}

public:
	Level(void){Level(cin);}
	Level(ifstream &cf){
		code.resize(10); //some nice starting amount
		int i,maxlen=0;
		for(i=0;cf.good();i++){
			if(i+1>code.size())code.resize(code.size()*2);
			getline(cf,code[i]);
			if(code[i].size()>maxlen)maxlen=code[i].size();
		}
		cf.close();
		code.resize(i);
		for(i=0;i<code.size();i++)code[i].resize(maxlen,' ');
	}
	void print(void){
		vector<string>::const_iterator it;
		for(it=code.begin();it!=code.end();it++)cout<<*it<<endl;
	}
	bool execcommandStep(mario *m){
		SHOW_MESSAGE("EC\tinstr="<<code[m->ipy][m->ipx]<<"\tip="<<m->ipx<<","<<m->ipy<<"\tmemp="<<m->memp<<"\tmem[memp]="<<memory[m->memp]<<"\tdir="<<m->dir<<"\twalking="<<m->walking<<"\tskip="<<m->skip);
		/*if(m->ipx<0||m->ipx>=code[m->ipy].size()){
			ERROR_FALSE("Mario walked out of the world.");
		}*/
		while(m->ipy<code.size()-1
			&&code[m->ipy+1][m->ipx]!='='
			&&code[m->ipy+1][m->ipx]!='|'
			&&code[m->ipy+1][m->ipx]!='#'
			&&code[m->ipy+1][m->ipx]!='"'){
			execcommandSingle(m);
			m->ipy++;
		}
		if(m->ipy==code.size()-1)ERROR_FALSE("Mario fell out of the world.");
		if(code[m->ipy+1][m->ipx]=='#'&&!m->walking){
			m->ipy--;
			while(m->ipy!=0&&code[m->ipy][m->ipx]!='"'){
				execcommandSingle(m);
				m->ipy--;
			}
			m->ipy--;
			if(m->ipy==-1){
				while(m->ipy<(int)code.size()-1&&code[m->ipy+1][m->ipx]!='"')m->ipy++;
				if(m->ipy==code.size()-1)ERROR_FALSE("Elevator without ending.");
			}
		}
		if(m->skip){
			m->skip=false;
			if(m->dir==DIRRIGHT){
				if(m->ipx<code[m->ipy].size()-1)m->ipx++;
				else ERROR_FALSE("Mario walked out of the world.");
			} else if(m->dir==DIRLEFT){
				if(m->ipx>0)m->ipx--;
				else ERROR_FALSE("Mario walked out of the world.");
			} else ERROR_FALSE("The world glitched.");
		} else {
			if(!execcommandSingle(m))return false;
			if(m->walking){
				if(m->dir==DIRRIGHT){
					if(m->ipx<code[m->ipy].size()-1)m->ipx++;
					else ERROR_FALSE("Mario walked out of the world.");
				} else if(m->dir==DIRLEFT){
					if(m->ipx>0)m->ipx--;
					else ERROR_FALSE("Mario walked out of the world.");
				} else ERROR_FALSE("Mario got tired of standing still.");
			}
		}
		return true;
	}
	bool execcommandSingle(mario *m){
		int ret;
		char c;
		SHOW_DEBUG("ECS at "<<m->ipx<<","<<m->ipy<<": '"<<code[m->ipy][m->ipx]<<"'");
		if(animate){
			animationFrameStart(m->ipx,m->ipy,code[m->ipy][m->ipx]);
		}
		//if(animate)cout<<"\x1B[s\x1B["<<m->ipy+1<<";"<<m->ipx+1<<"H\x1B[41;1m"<<code[m->ipy][m->ipx]<<"\x1B[0m\x1B[u"<<flush;
		switch(code[m->ipy][m->ipx]){
		case '=': case '|': case '#': case '"':
			ERROR_FALSE("Mario somehow got stuck.");
		case ')':
			m->memp++;
			if(m->memp>memory.sizepos()-1){
				SHOW_MESSAGE("Resizing memory to the right...");
				memory.resizepos(memory.sizepos()*2);
			}
			break;
		case '(':
			m->memp--;
			if(m->memp<-memory.sizeneg()){
				SHOW_MESSAGE("Resizing memory to the left...");
				memory.resizeneg(memory.sizeneg()*2);
			}
			break;
		case '+':
			memory[m->memp]++;
			break;
		case '-':
			memory[m->memp]--;
			break;
		case '.':
			if(animate){
				c=memory[m->memp];
				if(c=='\r')outputx=0;
				else if(c=='\n'){outputx=0;outputy++;}
				else if(c=='\t'){outputx=TABWIDTH*(outputx/TABWIDTH+1);}
				else if(c<32){printf("^%c",'@'+c);outputx+=2;}
				else if(c==127){printf("^?");outputx+=2;}
				else {putchar(c);outputx++;}
			} else putchar(memory[m->memp]);
			break;
		case ':':
			ret=printf("%d ",memory[m->memp]);
			if(ret<0)ERROR_FALSE("FATAL: IO error, printf returned a negative value!");
			outputx+=ret;
			break;
		case ',':
			if(animate){
				moveto(0,inputy);
				cout<<"\x1B[34;1mInput?\x1B[0m "<<flush;
			}
			memory[m->memp]=getch();
			if(animate){
				cout<<"\x1B[2K"<<flush;
				moveto(outputx,outputy);
			}
			SHOW_MESSAGE("Read input: char "<<memory[m->memp]);
			break;
		case ';':
			scanf("%d",&memory[m->memp]);
			break;
		case '>':
			m->dir=DIRRIGHT;
			m->walking=true;
			break;
		case '<':
			m->dir=DIRLEFT;
			m->walking=true;
			break;
		case '^':
			if(m->ipy!=0)m->ipy--;
			if(code[m->ipy][m->ipx]=='<'){
				m->dir=DIRLEFT;
				if(m->ipx==0)ERROR_FALSE("Mario walked out of the world.");
			} else if(code[m->ipy][m->ipx]=='>'){
				m->dir=DIRRIGHT;
				if(m->ipx>=code[m->ipy].size())ERROR_FALSE("Mario walked out of the world.");
			} else ERROR_FALSE("Mario is exhausted of jumping infinitely.");
			m->walking=true;
			break;
		case '!':
			m->walking=false;m->dir=DIRNONE;
			break;
		case '[':
			if(memory[m->memp]==0)m->skip=true;
			break;
		case '@':
			if(m->dir==DIRRIGHT)m->dir=DIRLEFT;
			else if(m->dir==DIRLEFT)m->dir=DIRRIGHT;
			else ERROR_FALSE("The world glitched.");
		default:
			break;
		}
		if(animate){
			animationFrameEnd(m,m->ipx,m->ipy,code[m->ipy][m->ipx]);
		}
		//if(animate)cout<<"\x1B[s\x1B["<<m->ipy+1+(code[m->ipy+1][m->ipx]=='^')<<";"<<m->ipx+1<<"H"<<code[m->ipy+(code[m->ipy+1][m->ipx]=='^')][m->ipx]<<"\x1B[u"<<flush;
		return true;
	}
	void play(void){
		mario *m=new mario;
		memory.resizepos(300);
		memory.resizeneg(300);
		m->ipx=0; m->ipy=0;
		m->dir=DIRRIGHT;
		m->walking=true;
		m->skip=false;
		m->memp=0;
		if(animate){
			cout<<"\x1B[1J";
			moveto(0,0);
			print();
			inputy=code.size(); //input comes first line under the code in animation mode
			outputx=0;
			outputy=inputy+1; //output comes directly under the input line
			tapex=code[0].size()+1; //tape comes to the right of the code
		}
		while(execcommandStep(m));
	}
};

int main(int argc,char **argv){
	if(argc==1){
		cerr<<"Usage: "<<argv[0]<<" [-a] [-d level] <file>"<<endl;
		cerr<<"\t-a        Turn on \x1B[46ma\x1B[0mnimation mode."<<endl;
		cerr<<"\t-d level  Set \x1B[46md\x1B[0mebug level to 'level', where 'level' can be:"<<endl;
		cerr<<"\t             0: No debug output. Default."<<endl;
		cerr<<"\t             1: Show errors."<<endl;
		cerr<<"\t             2: Plus one debug line per command."<<endl;
		cerr<<"\t             3: Frantic. Useless. Adds even more shit per executed command."<<endl;
		cerr<<"\t-T        Show the \x1B[46mT\x1B[0mape in animation mode."<<endl;
		cerr<<"\t-w msecs  Sets the time to \x1B[46mw\x1B[0mait between commands in milliseconds if animating. Useless otherwise."<<endl;
		return 0;
	}
	debuglevel=0;
	animatedelay=250000;
	animate=false;
	animateShowTape=false;
	int i,j;
	bool skipNextArg=false;
	for(i=1;i<argc-1;i++){
		if(skipNextArg){skipNextArg=false;continue;}
		if(argv[i][0]=='-'){
			for(j=1;argv[i][j]!='\0';j++){
				if(argv[i][j]=='a')animate=true;
				else if(argv[i][j]=='d'){debuglevel=strtol(argv[i+1],NULL,10);skipNextArg=true;}
				else if(argv[i][j]=='T')animateShowTape=true;
				else if(argv[i][j]=='w'){animatedelay=strtol(argv[i+1],NULL,10)*1000;skipNextArg=true;}
				else {cerr<<"Unrecognised option "<<argv[i][j]<<endl;return 0;}
			}
		} else {
			cerr<<"Unrecognised argument "<<argv[i]<<endl;
			return 0;
		}
	}
	ifstream cf; //CodeFile
	cf.open(argv[argc-1]);
	if(!cf.is_open()){
		cerr<<"Could not open file '"<<argv[argc-1]<<"'"<<endl;
		cerr<<"Run "<<argv[0]<<" without arguments for usage information."<<endl;
		return 0;
	}
	Level L(cf);
	L.play();	
	return 0;
}
