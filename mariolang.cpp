/**********
"MarioLANG is a two-dimensional programming language made by User:Wh1teWolf, based on Super Mario."
(quoting http://esolangs.org/wiki/MarioLANG)
This is an interpreter for the language by Tom Smeding.
**********/

#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <cstdlib>
#include <cstring>
#include <climits>
#include <cmath>
#include <termios.h>
#include <unistd.h>
#include <signal.h>

#define FATAL_FALSE(s) {SHOW_FATAL(s);return false;}
#define ERROR_FALSE(s) {SHOW_ERROR(s);return false;}
#define SHOW_FATAL(...) {if(debuglevel>=0){fprintf(stderr,"\x1B[31;1m");fprintf(stderr,__VA_ARGS__);fprintf(stderr,"\x1B[0m\n");}}
#define SHOW_ERROR(...) {if(debuglevel>=1){fprintf(stderr,"\x1B[31m");fprintf(stderr,__VA_ARGS__);fprintf(stderr,"\x1B[0m\n");}}
#define SHOW_MESSAGE(...) {if(debuglevel>=2){fprintf(stderr,"\x1B[36m");fprintf(stderr,__VA_ARGS__);fprintf(stderr,"\x1B[0m\n");}}
#define SHOW_DEBUG(...) {if(debuglevel>=3){fprintf(stderr,"\x1B[37m");fprintf(stderr,__VA_ARGS__);fprintf(stderr,"\x1B[0m\n");}}
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

char getch(){char b=0;struct termios o={};fflush(stdout);if(tcgetattr(0,&o)<0)
perror("tcsetattr()");o.c_lflag&=~ICANON;o.c_lflag&=~ECHO;o.c_cc[VMIN]=1;o.c_cc[
VTIME]=0;if(tcsetattr(0,TCSANOW,&o)<0)perror("tcsetattr ICANON");if(read(0,&b,1)
<0)perror("read()");o.c_lflag|=ICANON;o.c_lflag|=ECHO;if(tcsetattr(0,TCSADRAIN,&
o)<0)perror("tcsetattr ~ICANON");return b;}

void moveto(int x,int y){
	printf("\x1B[%d;%dH",y+1,x+1); fflush(stdout);
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
		char printf_format[20];
		if(!inited){
			inited=true;
			minidx=0;
			maxidx=0;
		}
		if(m->memp<minidx)minidx=m->memp;
		if(m->memp>maxidx)maxidx=m->memp;
		intwidth=max(1+log10(minidx==0?0.1:-minidx),log10(maxidx==0?1:maxidx))+1;
		printf("\x1B[34m"); fflush(stdout); //blue
		snprintf(printf_format,20,"%%%dd%%c %%d",intwidth);
		// fprintf(stderr,"%s",printf_format);
		// exit(0);
		for(i=minidx;i<=maxidx;i++){
			moveto(tapex,i-minidx);
			printf("\x1B[0K"); fflush(stdout);
			printf(printf_format,i,m->memp==i?'#':'.',memory[i]); fflush(stdout);
		}
		printf("\x1B[0m"); fflush(stdout);
		moveto(outputx,outputy);
	}

	void animationFrameStart(int x,int y,char c){
		moveto(x,y);
		printf("\x1B[41;1m%c\x1B[0m",c); fflush(stdout);
		moveto(outputx,outputy);
	}

	void animationFrameEnd(mario *m,int x,int y,char c){
		usleep(animatedelay);
		moveto(x,y);
		printf("%c",c); fflush(stdout);
		moveto(outputx,outputy);
		if(animateShowTape)drawTape(m);
	}

public:
	Level(void){Level(cin);}
	Level(ifstream &cf){
		code.resize(10); //some nice starting amount
		int i,maxlen=0;
		for(i=0;cf.good();i++){
			if(i+1>(int)code.size())code.resize(code.size()*2);
			getline(cf,code[i]);
			if((int)code[i].size()>maxlen)maxlen=code[i].size();
		}
		if(&cf!=&cin)cf.close();
		code.resize(i);
		for(i=0;i<(int)code.size();i++)code[i].resize(maxlen,' ');
	}
	int getoutputx(void){return outputx;}
	int getoutputy(void){return outputy;}
	void print(void){
		vector<string>::const_iterator it;
		for(it=code.begin();it!=code.end();it++)printf("%s\n",it->c_str());
	}
	bool execcommandStep(mario *m){
		static bool lastTurnWasStanding=false;
		SHOW_MESSAGE("EC\tinstr=%c\tip=%d,%d\tmemp=%d\tmem[memp]=%d\tdir=%d\twalking=%d\tskip=%d",code[m->ipy][m->ipx],m->ipx,m->ipy,m->memp,memory[m->memp],m->dir,m->walking,m->skip);
		/*if(m->ipx<0||m->ipx>=code[m->ipy].size()){
			ERROR_FALSE("Mario walked out of the world.");
		}*/
		while(m->ipy<(int)code.size()-1
			&&code[m->ipy+1][m->ipx]!='='
			&&code[m->ipy+1][m->ipx]!='|'
			&&code[m->ipy+1][m->ipx]!='#'
			&&code[m->ipy+1][m->ipx]!='"'){
			if(!execcommandSingle(m))return false;
			m->ipy++;
		}
		if(m->ipy==(int)code.size()-1)ERROR_FALSE("Mario fell out of the world.");
		if(code[m->ipy+1][m->ipx]=='#'&&!m->walking){
			int newipy=m->ipy;
			while(newipy>0&&code[newipy][m->ipx]!='"')newipy--;
			if(newipy>0){
				newipy--;
				for(m->ipy-=1;m->ipy>newipy+1;m->ipy--)if(!execcommandSingle(m))return false;
				m->ipy=newipy;
			} else {
				newipy=m->ipy+2;
				while(newipy<(int)code.size()&&code[newipy][m->ipx]!='"')newipy++;
				if(newipy==(int)code.size())FATAL_FALSE("Elevator without ending.");
				newipy--;
				for(m->ipy+=2;m->ipy<newipy;m->ipy++)if(!execcommandSingle(m))return false;
				//now m->ipy == newipy
			}
		}
		if(!execcommandSingle(m))return false;
		if(m->walking){
			if(m->dir==DIRRIGHT){
				if(m->ipx<(int)code[m->ipy].size()-1)m->ipx++;
				else ERROR_FALSE("Mario walked out of the world.");
			} else if(m->dir==DIRLEFT){
				if(m->ipx>0)m->ipx--;
				else ERROR_FALSE("Mario walked out of the world.");
			} else FATAL_FALSE("The world glitched!");
			lastTurnWasStanding=false;
		} else {
			if(!lastTurnWasStanding)lastTurnWasStanding=true;
			else ERROR_FALSE("Mario got tired of standing still.");
		}
		return true;
	}
	bool execcommandSingle(mario *m){
		int num;
		char c;
		SHOW_DEBUG("ECS at %d,%d: '%c'",m->ipx,m->ipy,code[m->ipy][m->ipx]);
		if(animate){
			animationFrameStart(m->ipx,m->ipy,code[m->ipy][m->ipx]);
		}
		//if(animate)cout<<"\x1B[s\x1B["<<m->ipy+1<<";"<<m->ipx+1<<"H\x1B[41;1m"<<code[m->ipy][m->ipx]<<"\x1B[0m\x1B[u"<<flush;
		char codechar=code[m->ipy][m->ipx];
		if(m->skip){
			if(strchr("=|#\")(+-.:,;><^![@w",codechar)!=NULL)m->skip=false;
			codechar='\0';
		}
		switch(codechar){
		case '=': case '|': case '#': case '"':
			FATAL_FALSE("Mario somehow got stuck.");
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
				else if(c>=0&&c<32){printf("^%c",'@'+c);outputx+=2;}
				else if(c==127){printf("^?");outputx+=2;}
				else {printf("%c",c);outputx++;}
			} else printf("%c",(char)memory[m->memp]);
			fflush(stdout);
			break;
		case ':':
			num=printf("%d ",memory[m->memp]);
			if(num<0)ERROR_FALSE("FATAL: IO error, printf returned a negative value!");
			fflush(stdout);
			outputx+=num;
			break;
		case ',':
			if(animate){
				moveto(0,inputy);
				printf("\x1B[34mInput char?\x1B[0m "); fflush(stdout);
				memory[m->memp]=getch();
			} else {
				memory[m->memp]=getchar();
			}
			if(animate){
				printf("\x1B[2K"); fflush(stdout);
				moveto(outputx,outputy);
			}
			SHOW_MESSAGE("Read input: char %c",(unsigned char)memory[m->memp]);
			break;
		case ';':
			if(animate){
				moveto(0,inputy);
				printf("\x1B[34mInput number?\x1B[0m "); fflush(stdout);
				c=getch();
				num=0;
				while(c!='\r'&&c!='\n'){
					if(c>='0'&&c<='9'){
						num=10*num+c-'0';
						printf("%c",c); fflush(stdout);
					} else if(c==127&&num!=0){ //backspace
						printf("\x1B[D \x1B[D"); fflush(stdout);
						num/=10;
					}
					c=getch();
				}
				memory[m->memp]=num;
				printf("\x1B[2K"); fflush(stdout);
				moveto(outputx,outputy);
			} else {
				scanf("%d",&memory[m->memp]);
			}
			SHOW_MESSAGE("Read input: number %d",(int)memory[m->memp]);
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
				if(m->ipx>=(int)code[m->ipy].size())ERROR_FALSE("Mario walked out of the world.");
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
			break;
		case 'w':
			animatedelay+=200000;
			break;
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
			printf("\x1B[1J");
			moveto(0,0);
			print();
			inputy=code.size(); //input comes first line under the code in animation mode
			outputx=0;
			outputy=inputy+2; //output comes under the input line, under "Output:"
			tapex=code[0].size()+1; //tape comes to the right of the code
			moveto(0,outputy-1);
			printf("\x1B[34;1mOutput:\x1B[0m"); fflush(stdout);
			moveto(outputx,outputy);
		}
		while(execcommandStep(m));
	}
};

Level *L;

void signalhandler(int sig){
	switch(sig){
	case SIGINT:
		if(animate)moveto(L->getoutputx(),L->getoutputy());
		exit(2);
	case SIGUSR1:
		animatedelay+=100000; //100ms extra
		break;
	}
}

int main(int argc,char **argv){
	if(argc==1){
		printf(
		 "Usage: %s [-a] [-d level] <file>\n"
		 "\t-a        Turn on \x1B[46ma\x1B[0mnimation mode.\n"
		 "\t-d level  Set \x1B[46md\x1B[0mebug level to 'level', where 'level' can be:\n"
		 "\t            -1: No fatal errors.\n"
		 "\t             0: No debug output. Default.\n"
		 "\t             1: Show errors.\n"
		 "\t             2: Plus one debug line per command.\n"
		 "\t             3: Frantic. Useless. Adds even more shit per executed command.\n"
		 "\t-T        Show the \x1B[46mT\x1B[0mape in animation mode.\n"
		 "\t-w msecs  Sets the time to \x1B[46mw\x1B[0mait between commands in milliseconds if animating. Useless otherwise.\n"
		 ,argv[0]);
		return 0;
	}
	signal(SIGINT,signalhandler);
	signal(SIGUSR1,signalhandler);
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
				else {fprintf(stderr,"Unrecognised option %c\n",argv[i][j]);return 0;}
			}
		} else {
			fprintf(stderr,"Unrecognised argument %s\n",argv[i]);
			return 0;
		}
	}
	if(strcmp(argv[argc-1],"-")==0){
		L=new Level();
	} else {
		ifstream cf; //CodeFile
		cf.open(argv[argc-1]);
		if(!cf.is_open()){
			fprintf(stderr,"Could not open file '%s'\n",argv[argc-1]);
			fprintf(stderr,"Run %s without arguments for usage information.\n",argv[0]);
			return 0;
		}
		L=new Level(cf);
	}
	L->play();
	delete L;
	return 0;
}
