#!/usr/bin/env node
function array_init(len,init){
	return Array.apply(null,new Array(len)).map(function(){return init;});
}

function genChar(thechar){
	var i,a,b,b1,b2,mina,minb,ascii,single;
	single=array_init(7,[]);
	ascii=thechar.charCodeAt(0);
	mina=minb=1e9;
	for(a=1;a<=ascii;a++){
		for(b=1;b<=ascii;b++){
			if(a+b+Math.abs(ascii-a*b)<mina+minb+Math.abs(ascii-mina*minb)){
				mina=a;
				minb=b;
			}
		}
	}
	a=mina;
	b=minb;
	if(b>a){
		a=a+b;
		b=a-b;
		a=a-b;
	}
	single[0]=array_init(a," ").concat(" "," ");
	single[0][~~(a/2)]=thechar;
	single[1]=[")"].concat(array_init(a,"+"));
	single[2]=array_init(a+1,"=");
	if(b/2%1==0){
		b1=b2=b/2;
	} else {
		b1=Math.ceil(b/2);
		b2=Math.floor(b/2);
	}
	single[3]=[" "].concat(array_init(b1,"+"),"("," ",array_init(a+1-3-b1," "),"<"," ");
	single[4]=[" "].concat(array_init(a+1,"=")," ");
	//single[4][b1+2]='"';
	single[5]=[">"].concat(
		array_init(b2,"+"),
		")","-","[","!"
	);
	if(ascii==a*b){
		single[5]=single[5].concat(array_init(a-1-b2<0?0:a-1-b2," "));
	} else {
		single[5]=single[5].concat("(",
			array_init(Math.abs(ascii-a*b),ascii>a*b?"+":"-"),
			")",
			array_init(a-3-b2-Math.abs(ascii-a*b)<0?0:a-3-b2-Math.abs(ascii-a*b)," "),
			" "
		);
	}
	single[6]=array_init(single[5].length,"=");
	single[6][b2+4]="#";
	single[3][b2+4]="<";
	single[4][b2+4]='"';
	if(single[5].length>single[0].length)single[0]=single[0].concat(array_init(single[5].length-single[0].length," "));
	if(single[5].length>single[1].length)single[1]=single[1].concat(array_init(single[5].length-single[1].length," "));
	if(single[5].length>single[2].length)single[2]=single[2].concat(array_init(single[5].length-single[2].length," "));
	if(single[5].length>single[3].length)single[3]=single[3].concat(array_init(single[5].length-single[3].length," "));
	if(single[5].length>single[4].length)single[4]=single[4].concat(array_init(single[5].length-single[4].length," "));
	return single;
}

function genLine(line){
	var code,single,i;
	code=array_init(7,[]);
	for(i=0;i<line.length;i++){
		single=genChar(line[i]);
		code=code.map(function(r,i){
			if(i==1)return r.concat(">",single[i]);
			if(i==2)return r.concat('"',single[i]);
			if(i==5)return r.concat("!",single[i]);
			if(i==6)return r.concat("#",single[i]);
			return r.concat(" ",single[i]);
		});
	}
	code=code.map(function(r){return r.slice(1);});
	code[3]=code[3].concat(" "," ","(","<"," "," "," "," "," ","<"," ");
	code[4]=code[4].concat(" "," ","=",'"'," "," ","=","=","=",'"'," ");
	code[5]=code[5].concat("(",">","[","!",")",">",".",")","[","!"," ");
	code[6]=code[6].concat("=","=","=","#","=","=","=","=","=","#","=");
	return code;
}

function lineToStrings(line){
	return line.map(function(r){return r.join("");}).join("\n");
}

process.stderr.write("Text? ");
process.stdin.resume();
process.stdin.once("data",function(data){
	var code;
	data=data.toString().slice(0,-1); //remove the newline
	code=genLine(data);
	console.log(lineToStrings(code));
	process.exit();
});
