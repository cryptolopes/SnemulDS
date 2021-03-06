#include <nds.h>
#include <stdio.h>
#include <unistd.h>
#include "frontend.h"

#include "diskio.h"
#include "ff.h"

size_t ucs2tombs(unsigned char* dst, const unsigned short* src, size_t len) {
	size_t i=0,j=0;
	for (;src[i];i++){
		if(src[i] <= 0x007f){
			if(!dst)j++;else{
				if(len-j<2)break;
				dst[j++] = ((src[i] & 0x007f)      );
			}
		}else if(src[i] <= 0x07ff){
			if(!dst)j+=2;else{
				if(len-j<3)break;
				dst[j++] = ((src[i] & 0x07c0) >>  6) | 0xc0;
				dst[j++] = ((src[i] & 0x003f)      ) | 0x80;
			}
		}else if((src[i] & 0xdc00) == 0xd800 && (src[i+1] & 0xdc00) == 0xdc00){
			if(!dst)j+=4;else{
				unsigned short z = (src[i]&0x3ff)+0x40;
				if(len-j<5)break;
				dst[j++] = ((z      & 0x0300) >>  8) | 0xf0;   //2
				dst[j++] = ((z      & 0x00fc) >>  2) | 0x80;   //6
				dst[j++] = ((z      & 0x0003) <<  4)           //2
					  | ((src[i+1] & 0x03c0) >>  6) | 0x80; //4
				dst[j++] = ((src[i+1] & 0x003f)      ) | 0x80; //6
			}i++;
		}else{
			if(!dst)j+=3;else{
				if(len-j<4)break;
				dst[j++] = ((src[i] & 0xf000) >> 12) | 0xe0;
				dst[j++] = ((src[i] & 0x0fc0) >>  6) | 0x80;
				dst[j++] = ((src[i] & 0x003f)      ) | 0x80;
			}
		}
	}
	if(dst)dst[j]=0;
	return j;
}

static u8 buf[768]; //lol

typedef u16 UnicodeChar;
#define ExtLinkBody_MaxLength (256)
#define ExtLinkBody_ID (0x30545845) // EXT0
typedef struct {
  u32 ID,dummy1,dummy2,dummy3; // dummy is ZERO.
  char DataFullPathFilenameAlias[ExtLinkBody_MaxLength];
  char DataPathAlias[ExtLinkBody_MaxLength];
  char DataFilenameAlias[ExtLinkBody_MaxLength];
  char NDSFullPathFilenameAlias[ExtLinkBody_MaxLength];
  char NDSPathAlias[ExtLinkBody_MaxLength];
  char NDSFilenameAlias[ExtLinkBody_MaxLength];
  UnicodeChar DataFullPathFilenameUnicode[ExtLinkBody_MaxLength];
  UnicodeChar DataPathUnicode[ExtLinkBody_MaxLength];
  UnicodeChar DataFilenameUnicode[ExtLinkBody_MaxLength];
  UnicodeChar NDSFullPathFilenameUnicode[ExtLinkBody_MaxLength];
  UnicodeChar NDSPathUnicode[ExtLinkBody_MaxLength];
  UnicodeChar NDSFilenameUnicode[ExtLinkBody_MaxLength];
} TExtLinkBody;

/*
char* myfgets(char *buf,int n,FILE *fp){ //accepts LF/CRLF
	char *ret=fgets(buf,n,fp);
	if(!ret)return NULL;
	if(strlen(buf)&&buf[strlen(buf)-1]=='\n')buf[strlen(buf)-1]=0;
	if(strlen(buf)&&buf[strlen(buf)-1]=='\r')buf[strlen(buf)-1]=0;
	return ret;
}
*/

char* myfgets(char *buf,int n,FIL *fp){ //accepts LF/CRLF
	char *ret=f_gets(buf,n,fp);
	if(!ret)return NULL;
	if(strlen(buf)&&buf[strlen(buf)-1]=='\n')buf[strlen(buf)-1]=0;
	if(strlen(buf)&&buf[strlen(buf)-1]=='\r')buf[strlen(buf)-1]=0;
	return ret;
}


void SplitItemFromFullPathAlias(const char *pFullPathAlias,char *pPathAlias,char *pFilenameAlias){
	u32 SplitPos=0;
	{
		u32 idx=0;
		while(1){
			char uc=pFullPathAlias[idx];
			if(uc==0) break;
			if(uc=='/') SplitPos=idx+1;
			idx++;
		}
	}

	if(pPathAlias){
		if(SplitPos<=1){
			pPathAlias[0]='/';
			pPathAlias[1]=0;
		}else{
			u32 idx=0;
			for(;idx<SplitPos-1;idx++){
				pPathAlias[idx]=pFullPathAlias[idx];
			}
			pPathAlias[SplitPos-1]=0;
		}
	}
	if(pFilenameAlias)strcpy(pFilenameAlias,&pFullPathAlias[SplitPos]);
}

bool _readFrontend(char *target){
	FIL f;
	//if(f_open(&pathFile, "luma/path.txt", FA_READ) == FR_OK)
	
	if(f_open(&f,"/loadfile.dat",FA_READ) == FR_OK){
		int i=0;
		myfgets((char*)buf,768,&f);
		//fclose(f);
		f_close(&f);
		
		//unlink("/loadfile.dat");
		f_unlink("/loadfile.dat");
		
		//if(!memcmp((char*)buf,"fat:",4))i+=4;
		if(!memcmp((char*)buf+i,"//",2))i+=1;
		if(!memcmp((char*)buf+i,"/./",3))i+=2; //menudo dir handling is buggy?
		strcpy(target,(char*)buf+i);
		if(strlen(target)<4||(strcasecmp(target+strlen(target)-4,".smc")&&strcasecmp(target+strlen(target)-4,".sfc")))return false;
		return true;
	}
	//f=fopen("/plgargs.dat","rb");
	if(f_open(&f,"/plgargs.dat",FA_READ) == FR_OK){
		int i=0;
		myfgets((char*)buf,768,&f);
		myfgets((char*)buf,768,&f); //second line
		//fclose(f);
		f_close(&f);
		
		//unlink("/plgargs.dat");
		f_unlink("/plgargs.dat");
		
		//if(!memcmp((char*)buf,"fat:",4))i+=4;
		//if(!memcmp((char*)buf+i,"//",2))i+=1;
		//if(!memcmp((char*)buf+i,"/./",3))i+=2;
		strcpy(target,(char*)buf+i);
		if(strlen(target)<4||(strcasecmp(target+strlen(target)-4,".smc")&&strcasecmp(target+strlen(target)-4,".sfc")))return false;
		return true;
	}
	//f=fopen("/moonshl2/extlink.dat","r+b");
	if(f_open(&f,"/moonshl2/extlink.dat",FA_READ | FA_WRITE) == FR_OK){
		TExtLinkBody extlink;
		memset(&extlink,0,sizeof(TExtLinkBody));
		
		//fread(&extlink,1,sizeof(TExtLinkBody),f);
		unsigned int read_so_far;
		f_read(&f, &extlink, sizeof(TExtLinkBody), &read_so_far);
		
		if(extlink.ID!=ExtLinkBody_ID){
			//fclose(f);
			f_close(&f);
			return false;
		}
		
		//strcpy(target,extlink.DataFullPathFilenameAlias);
		
		ucs2tombs((unsigned char*)target,extlink.DataFullPathFilenameUnicode,768);
		
		//fseek(f,0,SEEK_SET);
		f_lseek (&f, 0);
		
		//fwrite("____",1,4,f);
		unsigned int written;
		f_write(&f, "____", 4, &written);
		f_truncate(&f);
		
		//fclose(f);
		f_close(&f);
		
		if(strlen(target)<4||(strcasecmp(target+strlen(target)-4,".smc")&&strcasecmp(target+strlen(target)-4,".sfc")))return false;
		return true;
	}
	return false; //it is your choice to boot GUI or to halt.
}

extern int argc;
extern char **argv;
static char target[768],name[768],dir[768];
bool readFrontend(char **_name, char **_dir)
{
	*_name=NULL;
	*_dir=NULL;
	if(argc>1){
		strcpy(target,argv[1]);
	}else{
		if(!_readFrontend(target))return false;
	}
	SplitItemFromFullPathAlias(target,dir,name);
	//chdir(dir);
	*_name=name;
	*_dir=dir;
	return true;
}

