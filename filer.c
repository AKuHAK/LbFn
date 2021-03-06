#include "launchelf.h"

// psuファイルヘッダ構造体
typedef struct { // 512 bytes
	unsigned short attr;
	unsigned short unknown1;
	unsigned int size;	//file size, 0 for directory
	unsigned char createtime[8];	//0x00:sec:min:hour:day:month:year
	unsigned int unknown2;
	unsigned int unknown3;
	unsigned char modifytime[8];	//0x00:sec:min:hour:day:month:year
	unsigned char unknown4[32];
	unsigned char name[32];
	unsigned char unknown5[416];
} PSU_HEADER;

//FILEINFO type
enum
{
	TYPE_DEVICE_MC,
	TYPE_DEVICE_HDD,
	TYPE_DEVICE_CD,
	TYPE_DEVICE_MASS,
	TYPE_DEVICE_HOST,
	TYPE_DEVICE_VMC,
	TYPE_MISC,
	TYPE_FILE,
	TYPE_ELF,
	TYPE_DIR,
	TYPE_PS2SAVE,
	TYPE_PS1SAVE,
	TYPE_PSU,
	TYPE_OTHER=15,
};

//menu
enum
{
	COPY,
	CUT,
	PASTE,
	DELETE,
	RENAME,
	NEWDIR,
	GETSIZE,
	EXPORT,
	IMPORT,
//	COMPRESS,
	VIEWER,
	NUM_MENU
};

// ASCIIとSJISの変換用配列
const unsigned char sjis_lookup_81[256] = {
  0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,  // 0x00
  0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,  // 0x10
  0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,  // 0x20
  0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,  // 0x30
   ' ', ',', '.', ',', '.',0xFF, ':', ';', '?', '!',0xFF,0xFF,'\'', '`',0xFF, '^',  // 0x40
  0xFF, '_',0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF, '-', '-', '/',0xFF,  // 0x50
  0xFF,0xFF,0xFF,0xFF,0xFF,'\'','\'', '"', '"', '(', ')', '[', ']', '[', ']', '{',  // 0x60
   '}',0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF, '+', '-',0xFF,'*', 0xFF,  // 0x70
   '/', '=',0xFF, '<', '>',0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,'\\',  // 0x80
   '$',0xFF,0xFF, '%', '#', '&', '*', '@',0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,  // 0x90
  0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,  // 0xA0
  0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,  // 0xB0
  0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,  // 0xC0
  0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,  // 0xD0
  0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,  // 0xE0
  0xFF,0xFF, '#',0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,  // 0xF0
};
const unsigned char sjis_lookup_82[256] = {
  0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,  // 0x00
  0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,  // 0x10
  0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,  // 0x20
  0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,  // 0x30
  0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,'0',   // 0x40
  '1', '2', '3', '4', '5', '6', '7', '8', '9', 0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,  // 0x50
  'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J', 'K', 'L', 'M', 'N', 'O', 'P',   // 0x60
  'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X', 'Y', 'Z', 0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,  // 0x70
  0xFF,'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h', 'i', 'j', 'k', 'l', 'm', 'n', 'o',   // 0x80
  'p', 'q', 'r', 's', 't', 'u', 'v', 'w', 'x', 'y', 'z', 0xFF,0xFF,0xFF,0xFF,0xFF,  // 0x90
  0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,  // 0xA0
  0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,  // 0xB0
  0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,  // 0xC0
  0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,  // 0xD0
  0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,  // 0xE0
  0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,  // 0xF0
};

int cut;
int nclipFiles, nmarks, nparties;
int title;
int sortmode;
char mountedParty[2][MAX_NAME];
char parties[MAX_PARTITIONS][MAX_NAME];
char clipPath[MAX_PATH], LastDir[MAX_NAME], marks[MAX_ENTRY];
FILEINFO clipFiles[MAX_ENTRY];
int fileMode = FIO_S_IRUSR | FIO_S_IWUSR | FIO_S_IXUSR | FIO_S_IRGRP | FIO_S_IWGRP | FIO_S_IXGRP | FIO_S_IROTH | FIO_S_IWOTH | FIO_S_IXOTH;

#ifdef ENABLE_PSB
#define MAX_ARGC 3
int psb_argc;
char psb_argv[MAX_ARGC][MAX_PATH+2];
#endif

//プロトタイプ宣言
void sjis2ascii(const unsigned char *in, unsigned char *out);
int getDir(const char *path, FILEINFO *info);
void setPartyList(void);

//-------------------------------------------------
//拡張子を取得
char* getExtension(const char *path)
{
	return strrchr(path,'.');
}

//-------------------------------------------------
// HDDのパーティションとパスを取得
int getHddParty(const char *path, const FILEINFO *file, char *party, char *dir)
{
	char fullpath[MAX_PATH], *p;

	if(strncmp(path,"hdd",3)) return -1;

	//fullpathを作成
	strcpy(fullpath, path);
	if(file!=NULL){
		strcat(fullpath, file->name);
		if(file->attr & MC_ATTR_SUBDIR)	//フォルダのときスラッシュつける
			strcat(fullpath,"/");
	}
	//パーティション名がないときはエラー
	if((p=strchr(&fullpath[6], '/'))==NULL)
		return -1;
	//dirにpfs0:から始まるパスをコピー 例:pfs0:/BOOT.ELF
	if(dir!=NULL)
		sprintf(dir, "pfs0:%s", p);
	//パスをパーティション名までにする
	*p=0;
	//partyにhdd0:とパーティション名をコピー 例:hdd0:__boot
	if(party!=NULL)
		sprintf(party, "hdd0:%s", &fullpath[6]);

	return 0;
}

//-------------------------------------------------
// パーティションのマウント
int mountParty(const char *party)
{
	//マウントしていたら番号を返す
	if(!strcmp(party, mountedParty[0]))
		return 0;
	else if(!strcmp(party, mountedParty[1]))
		return 1;

	//マウントしていないときpfs0にマウント
	fileXioUmount("pfs0:");
	mountedParty[0][0]=0;
	if(fileXioMount("pfs0:", party, FIO_MT_RDWR) < 0)
		return -1;
	strcpy(mountedParty[0], party);
	return 0;
}

//-------------------------------------------------
// 文字列の比較
int cmpsb(char *src, char *dst)
{
	int i,l,sa,sb;
	unsigned char *si, *di;
	unsigned char a,b;
	si = (unsigned char *) src;
	di = (unsigned char *) dst;
	l = strlen(si);
	if (strlen(di) < l) l = strlen(di);
	l++;
	for (i=0,sa=0,sb=0;i<l;i++) {
		a=si[i];b=di[i];
		if (!sa && (a>=0x81)&&(a<0xFD)&&((a<0xA0)||(a>=0xE0))) sa=2;
		if (!sb && (b>=0x81)&&(b<0xFD)&&((b<0xA0)||(b>=0xE0))) sb=2;
		if ((sa==0) && (a>=0x61)&&(a<=0x7A)) a-=0x20;
		if ((sb==0) && (b>=0x61)&&(b<=0x7A)) b-=0x20;
		if (sa) sa--;
		if (sb) sb--;
		if (a < b) return -1;
		else if (a > b) return 1;
	}
	return 0;
}

char *getext(char *src)
{
	int i,j,k;
	i=strlen(src);
	for(j=0,k=i;j<i;j++){
		if (src[j] == 0x2E) k=j;
	}
	return &src[k];
}

//-------------------------------------------------
// ソート
void sort(FILEINFO *a, int max)
{
	int i,m=1,k;
	FILEINFO b;

	if (sortmode == 0) {	// ソートしない
		for (i=0;i<max-1;i++)
			for (m=i+1;m<max;m++)
				if (a[i].num > a[m].num) {
					b=a[i]; a[i]=a[m]; a[m]=b;
				}
	} else if (sortmode == 1) {	// ファイル名でソート
		for (i=0;i<max-1;i++)
			for (m=i+1;m<max;m++)
				if (cmpsb(a[i].name, a[m].name)>0) {
					b=a[i]; a[i]=a[m]; a[m]=b;
				}
	} else if (sortmode == 2) { // 拡張子でソート
		for (i=0;i<max-1;i++)
			for (m=i+1;m<max;m++)
				if (cmpsb(getext(a[i].name), getext(a[m].name))>0) {
					//printf("file: [%3d] %s <-> [%3d] %s\n", i, getext(a[i].name), m, getext(a[m].name));
					b=a[i]; a[i]=a[m]; a[m]=b;
				}
	} else if (sortmode == 3) { // ゲームタイトルでソート
		for (i=0;i<max-1;i++)
			for (m=i+1;m<max;m++)
				if (cmpsb(a[i].title, a[m].title)>0) {
					b=a[i]; a[i]=a[m]; a[m]=b;
				}
	} else if (sortmode == 4) { // ファイルサイズでソート
		for (i=0;i<max-1;i++)
			for (m=i+1;m<max;m++)
				if (a[i].fileSizeByte > a[m].fileSizeByte) {
					b=a[i]; a[i]=a[m]; a[m]=b;
				}
	} else if (sortmode == 5) { // 更新日時でソート
		for (i=0;i<max-1;i++)
			for (m=i+1;m<max;m++)
				if (a[i].timestamp > a[m].timestamp){
					//printf("file: [%08X] %s <-> [%08X] %s\n", a[i].timestamp, a[i].name, a[m].timestamp, a[m].name);
					b=a[i]; a[i]=a[m]; a[m]=b;
				}
	}
	// ELFをトップに表示
	if (setting->sortext) {
		if (!stricmp(getext(a[0].name), ".ELF"))
			k = 1;
		else
			k = 0;
		for (i=1;i<max;i++) {
			if (!stricmp(getext(a[i].name), ".ELF")) {
				if (i>k) {
					b=a[i];
					for (m=i;m>k;m--)
						a[m]=a[m-1];
					a[k]=b;
				}
				k++;
			}
		}
	}
	// フォルダをトップに表示
	if (setting->sortdir) {
		if (a[0].attr & MC_ATTR_SUBDIR)
			k = 1;
		else
			k = 0;
		for (i=1;i<max;i++) {
			if (a[i].attr & MC_ATTR_SUBDIR) {
				if (i>k) {
					b=a[i];
					for (m=i;m>k;m--)
						a[m]=a[m-1];
					a[k]=b;
				}
				k++;
			}
		}
	}
	return;
}
extern int vmcmount;
//-------------------------------------------------
//psuファイルからゲームタイトル取得
//戻り値
//0以下:失敗
//    0:成功
int getGameTitlePsu(const char *path, const FILEINFO *file, char *out)
{
	char party[MAX_NAME], dir[MAX_PATH];
	int hddin=FALSE;
	int fd;
	int ret;
	int psuSize;
	PSU_HEADER psu_header_dir, psu_header;
	int n,i;
	int seek;
	char tmp[65];
	int fileSize;

	//ルートかパーティションリスト
	if(path[0]==0 || !strcmp(path, "hdd0:/")) return -1;

	//フルパス
	if(!strncmp(path, "hdd", 3)){
		getHddParty(path, file, party, dir);
		ret = mountParty(party);
		if(ret<0) return -1;
		dir[3]=ret+'0';
		hddin=TRUE;
	}
	else{
		sprintf(dir, "%s%s", path, file->name);
		if(file->attr & MC_ATTR_SUBDIR) strcat(dir, "/");
	}

	//psuファイルオープンとサイズ取得
	if(!strncmp(path, "hdd", 3)){
		hddin=TRUE;
		fd = fileXioOpen(dir, O_RDONLY, fileMode);
		if(fd<0){
			ret=-1;
			goto error;
		}
		psuSize = fileXioLseek(fd, 0, SEEK_END);
		fileXioLseek(fd, 0, SEEK_SET);
	}
	else{
		fd = fioOpen(dir, O_RDONLY);
		if(fd<0){
			ret=-1;
			goto error;
		}
		psuSize = fioLseek(fd, 0, SEEK_END);
		fioLseek(fd, 0, SEEK_SET);
	}

	//psuヘッダ読み込む
	if(psuSize<sizeof(PSU_HEADER)){
		ret=-2;
		goto error;
	}
	memset(&psu_header_dir, 0, sizeof(PSU_HEADER));
	if(hddin)
		fileXioRead(fd, (char*)&psu_header_dir, sizeof(PSU_HEADER));
	else
		fioRead(fd, &psu_header_dir, sizeof(PSU_HEADER));
	n = psu_header_dir.size;	//ファイル数
	seek = sizeof(PSU_HEADER);	//ファイルのシーク

	ret=-3;
	//psu_header[0]から読み込む
	for(i=0;i<n;i++){
		//ファイルヘッダ読み込む
		if(psuSize<seek+sizeof(PSU_HEADER)){
			ret=-4;
			goto error;
		}
		memset(&psu_header, 0, sizeof(PSU_HEADER));
		if(hddin)
			fileXioRead(fd, (char*)&psu_header, sizeof(PSU_HEADER));
		else
			fioRead(fd, &psu_header, sizeof(PSU_HEADER));
		seek += sizeof(PSU_HEADER);
		//ゲームタイトル
		if(!strcmp(psu_header.name, "icon.sys")){
			if(hddin){
				fileXioLseek(fd, seek+0xC0, SEEK_SET);
				fileXioRead(fd, tmp, 64);
				out[64]=0;
				fileXioLseek(fd, seek, SEEK_SET);
			}
			else{
				fioLseek(fd, seek+0xC0, SEEK_SET);
				fioRead(fd, tmp, 64);
				out[64]=0;
				fioLseek(fd, seek, SEEK_SET);
			}
			sjis2ascii(tmp, out);
			ret=0;	//成功
			break;
		}
		//次のファイルヘッダの位置にシーク
		if(psu_header.size>0){
			fileSize = (((psu_header.size-1)/0x400)+1)*0x400;
			if(psuSize<seek + fileSize){
				ret=-4;
				goto error;
			}
			seek += fileSize;
			if(hddin)
				fileXioLseek(fd, seek, SEEK_SET);
			else
				fioLseek(fd, seek, SEEK_SET);
		}
	}
error:
	//psuファイルクローズ
	if(hddin)
		fileXioClose(fd);
	else
		fioClose(fd);

	return ret;
}

//-------------------------------------------------
// セーブデータタイトルの取得
//戻り値
//0以下:失敗
//    0:PS2のセーブデータタイトル取得成功
//    1:PS1のセーブデータタイトル取得成功
int getGameTitle(const char *path, const FILEINFO *file, char *out)
{
	char party[MAX_NAME], dir[MAX_PATH];
	int fd=-1, hddin=FALSE, ret;
	char *ext;

	//ルートかパーティションリスト
	if(path[0]==0 || !strcmp(path, "hdd0:/")) return -1;

	//フルパス
	if(!strncmp(path, "hdd", 3)){
		getHddParty(path, file, party, dir);
		ret = mountParty(party);
		if(ret<0) return -1;
		dir[3]=ret+'0';
		hddin=TRUE;
	}
	else{
		sprintf(dir, "%s%s", path, file->name);
		if(file->attr & MC_ATTR_SUBDIR) strcat(dir, "/");
	}

	ret = -1;
	if(file->attr & MC_ATTR_SUBDIR){	//フォルダのとき
		if(hddin){
			//HDD
			strcat(dir, "icon.sys");
			if((fd=fileXioOpen(dir, O_RDONLY, fileMode)) >= 0){
				//icon.sysから取得
				if(fileXioLseek(fd,0,SEEK_END) <= 0x100)
					goto error;
				fileXioLseek(fd, 0xC0, SEEK_SET);
				fileXioRead(fd, out, 16*4);
				out[16*4] = 0;
				fileXioClose(fd); fd=-1;
				ret = 0;	//PS2
			}
		}
		else{
			//HDD以外
			strcat(dir, "icon.sys");
			if((fd=fioOpen(dir, O_RDONLY)) >= 0){
				//icon.sysから取得
				if(fioLseek(fd,0,SEEK_END) <= 0x100)
					goto error;
				fioLseek(fd, 0xC0, SEEK_SET);
				fioRead(fd, out, 16*4);
				out[16*4] = 0;
				fioClose(fd); fd=-1;
				ret = 0;	//PS2
			}
		}
		//icon.sysがないときPS1取得してみる
		if(ret!=0){
			char ps1dir[MAX_PATH];
			FILEINFO fi;
			sprintf(ps1dir, "%s%s/", path, file->name);
			strcpy(fi.name, file->name);
			fi.attr = 0;	//属性はファイル
			if(getGameTitle(ps1dir, &fi, out)>=0)
				ret = 1;	//PS1
			else
				out[0]=0;
		}
	}
	else{	//ファイルのとき
		ext = getExtension(file->name);
		if(ext!=NULL&&!stricmp(ext, ".psu")){
			//psuファイルのとき
			if(getGameTitlePsu(path, file, out)>=0)
				ret = 0;	//PS2
			else
				out[0]=0;
		}
		else{
			//psuファイルじゃないときPS1のゲームタイトル取得してみる
			if(hddin){
				if((fd=fileXioOpen(dir, O_RDONLY, fileMode)) < 0) goto error;
				if(fileXioLseek(fd, 0, SEEK_END) < 0x2000) goto error;
				fileXioLseek(fd, 0, SEEK_SET);
				fileXioRead(fd, out, 2);
				if(strncmp(out, "SC", 2)) goto error;
				fileXioLseek(fd, 4, SEEK_SET);
				fileXioRead(fd, out, 16*4);
				out[16*4] = 0;
				fileXioClose(fd); fd=-1;
				ret=1;	//PS1
			}
			else{
				if((fd=fioOpen(dir, O_RDONLY)) < 0) goto error;
				if(fioLseek(fd, 0, SEEK_END) < 0x2000) goto error;
				fioLseek(fd, 0, SEEK_SET);
				fioRead(fd, out, 2);
				if(strncmp(out, "SC", 2)) goto error;
				fioLseek(fd, 4, SEEK_SET);
				fioRead(fd, out, 16*4);
				out[16*4] = 0;
				fioClose(fd); fd=-1;
				ret=1;	//PS1
			}
		}
	}
error:
	if(fd>=0){
		if(hddin)
			fileXioClose(fd);
		else
			fioClose(fd);
	}
	return ret;
}

//-------------------------------------------------
// メニュー
int menu(const char *path, const char *file)
{
	uint64 color;
	char enable[NUM_MENU], tmp[MAX_PATH];
	int x, y, i, sel, redraw=framebuffers;

	int menu_x = SCREEN_WIDTH-FONT_WIDTH*19;
	int menu_y = FONT_HEIGHT*4;
	int menu_w = FONT_WIDTH*15;
	int menu_h = FONT_HEIGHT*(NUM_MENU+1);

	// メニュー項目有効・無効設定
	memset(enable, TRUE, NUM_MENU);	//全部TRUEにする

	if(!strcmp(path,"hdd0:/") || path[0]==0){
		enable[COPY] = FALSE;
		enable[CUT] = FALSE;
		enable[PASTE] = FALSE;
		enable[DELETE] = FALSE;
		enable[RENAME] = FALSE;
		enable[NEWDIR] = FALSE;
		enable[GETSIZE] = FALSE;
		enable[EXPORT] = FALSE;
		enable[IMPORT] = FALSE;
		//enable[COMPRESS] = FALSE;
		enable[VIEWER] = FALSE;
	}

	if(!strncmp(path, "mc", 2))
		enable[RENAME] = FALSE;

	if(!strncmp(path, "hdd", 3))
		enable[EXPORT] = FALSE;

	if(!strncmp(path,"cdfs",4)){
		enable[CUT] = FALSE;
		enable[PASTE] = FALSE;
		enable[DELETE] = FALSE;
		enable[RENAME] = FALSE;
		enable[NEWDIR] = FALSE;
		enable[EXPORT] = FALSE;
	}
	if(!strncmp(path, "mass", 4)){
		enable[RENAME] = FALSE;
		enable[EXPORT] = FALSE;
	}
	if(!strncmp(path, "host", 4)){
		enable[CUT] = FALSE;
		enable[DELETE] = FALSE;
		enable[NEWDIR] = FALSE;
		enable[RENAME] = FALSE;
		enable[EXPORT] = FALSE;
	}

	//マークしたファイルがない
	if(nmarks==0){
		//R1ボタンを押したときのカーソルの位置が".."のとき
		if(!strcmp(file, "..")){
			enable[COPY] = FALSE;
			enable[CUT] = FALSE;
			enable[DELETE] = FALSE;
			enable[RENAME] = FALSE;
			enable[GETSIZE] = FALSE;
			enable[EXPORT] = FALSE;
			enable[IMPORT] = FALSE;
			//enable[COMPRESS] = FALSE;
			enable[VIEWER] = FALSE;
		}
	}
	else{
		//マークしたファイルがある
		enable[RENAME] = FALSE;
	}

	//クリップボードに記憶したファイルがない
	if(nclipFiles==0)
		enable[PASTE] = FALSE;

	// 初期選択項設定
	for(sel=0; sel<NUM_MENU; sel++)
		if(enable[sel]==TRUE) break;

	while(1){
		waitPadReady(0, 0);
		if(readpad()){
			if(new_pad) redraw = framebuffers;
			if(new_pad & PAD_UP && sel<NUM_MENU){
				do{
					sel--;
					if(sel<0) sel=NUM_MENU-1;
				}while(!enable[sel]);
			}else if(new_pad & PAD_DOWN && sel<NUM_MENU){
				do{
					sel++;
					if(sel==NUM_MENU) sel=0;
				}while(!enable[sel]);
			}else if(new_pad & PAD_CROSS)
				return -1;
			else if(new_pad & PAD_CIRCLE)
				break;
		}

		// 描画開始
		if (redraw) {
			drawDialogTmp(menu_x, menu_y, menu_x+menu_w, menu_y+menu_h, setting->color[COLOR_BACKGROUND], setting->color[COLOR_FRAME]);
			for(i=0,y=74; i<NUM_MENU; i++){
				if(i==COPY) strcpy(tmp, lang->filer_menu_copy);
				else if(i==CUT) strcpy(tmp, lang->filer_menu_cut);
				else if(i==PASTE) strcpy(tmp, lang->filer_menu_paste);
				else if(i==DELETE) strcpy(tmp, lang->filer_menu_delete);
				else if(i==RENAME) strcpy(tmp, lang->filer_menu_rename);
				else if(i==NEWDIR) strcpy(tmp, lang->filer_menu_newdir);
				else if(i==GETSIZE) strcpy(tmp, lang->filer_menu_getsize);
				else if(i==EXPORT) strcpy(tmp, lang->filer_menu_exportpsu);
				else if(i==IMPORT) strcpy(tmp, lang->filer_menu_importpsu);
				//else if(i==COMPRESS) strcpy(tmp, lang->filer_menu_compress);
				else if(i==VIEWER) strcpy(tmp, lang->filer_menu_editor);

				if(enable[i]){
					if(sel==i)
						color = setting->color[COLOR_HIGHLIGHTTEXT];	//強調
					else
						color = setting->color[COLOR_TEXT];	//ノーマル
				}
				else
					color = setting->color[COLOR_GRAYTEXT];	//無効

				printXY(tmp, menu_x+FONT_WIDTH*2, menu_y+FONT_HEIGHT/2+i*FONT_HEIGHT, color, TRUE);
				y+=FONT_HEIGHT;
			}
			if(sel<NUM_MENU)
				printXY(">", menu_x+FONT_WIDTH, menu_y+FONT_HEIGHT/2+sel*FONT_HEIGHT, setting->color[COLOR_HIGHLIGHTTEXT], TRUE);	//強調

			// 操作説明
			x = FONT_WIDTH*1;
			y = SCREEN_MARGIN+(MAX_ROWS+4)*FONT_HEIGHT;
			X_itoSprite(0, y, SCREEN_WIDTH, SCREEN_HEIGHT, 0);
			sprintf(tmp,"○:%s ×:%s", lang->gen_ok, lang->gen_cancel);
			printXY(tmp, x, y, setting->color[COLOR_TEXT], TRUE);
			drawScr();
			redraw--;
		} else {
			itoVSync();
		}
	}

	return sel;
}

//-------------------------------------------------
// ファイルサイズ取得
long getFileSize(const char *path, const FILEINFO *file)
{
	long size, ret;
	FILEINFO files[MAX_ENTRY];
	char dir[MAX_PATH];//, party[MAX_NAME];
	int nfiles, i, fd;

	if(file->attr & MC_ATTR_SUBDIR){
		sprintf(dir, "%s%s/", path, file->name);
		// 対象フォルダ内の全ファイル・フォルダサイズを合計
		nfiles = getDir(dir, files);
		for(i=size=0; i<nfiles; i++){
			ret=getFileSize(dir, &files[i]);
			if(ret < 0) size = -1;
			else		size+=ret;
		}
	}
	else{
	/*//
		// パーティションマウント
		if(!strncmp(path, "hdd", 3)){
			getHddParty(path,file,party,dir);
			ret = mountParty(party);
			if(ret<0) return 0;
			dir[3] = ret+'0';
		}else
			sprintf(dir, "%s%s", path, file->name);
		// ファイルサイズ取得
		if(!strncmp(path, "hdd", 3)){
			fd = fileXioOpen(dir, O_RDONLY, fileMode);
			size = fileXioLseek(fd,0,SEEK_END);
			fileXioClose(fd);
		}else{
			fd = fioOpen(dir, O_RDONLY);
			size = fioLseek(fd,0,SEEK_END);
			fioClose(fd);
		}
	//*/
		sprintf(dir, "%s%s", path, file->name);
		fd = nopen(dir, O_RDONLY);
		if (fd >= 0) {
			size = nseek(fd, 0, SEEK_END);
			nclose(fd);
		} else 
			size = -1;
	}
	return size;
}

//-------------------------------------------------
// ファイル・フォルダ削除
int delete(const char *path, const FILEINFO *file)
{
	FILEINFO files[MAX_ENTRY];
	char party[MAX_NAME], dir[MAX_PATH], hdddir[MAX_PATH];
	int nfiles, i, ret;

	// パーティションマウント
	if(!strncmp(path, "hdd", 3)){
		getHddParty(path,file,party,hdddir);
		ret = mountParty(party);
		if(ret<0) return 0;
		hdddir[3] = ret+'0';
	}
	sprintf(dir, "%s%s", path, file->name);

	if(file->attr & MC_ATTR_SUBDIR){
		strcat(dir,"/");
		// 対象フォルダ内の全ファイル・フォルダを削除
		nfiles = getDir(dir, files);
		for(i=0; i<nfiles; i++){
			ret=delete(dir, &files[i]);
			if(ret < 0) return -1;
		}
		// 対象フォルダを削除
		if(!strncmp(dir, "mc", 2)){
			mcSync(MC_WAIT, NULL, NULL);
			mcDelete(dir[2]-'0', 0, &dir[4]);
			mcSync(MC_WAIT, NULL, &ret);
		}else if(!strncmp(path, "hdd", 3)){
			ret = fileXioRmdir(hdddir);
		}else if(!strncmp(path, "mass", 4)){
			//sprintf(dir, "mass0:%s%s", &path[5], file->name);
			sprintf(dir, "%s%s", path, file->name);
			ret = fioRmdir(dir);
		}else if(!strncmp(path, "host", 4)){
			sprintf(dir, "%s%s", path, file->name);
			ret = fioRmdir(dir);
		}
	} else {
		// 対象ファイルを削除
		if(!strncmp(path, "mc", 2)){
			mcSync(MC_WAIT, NULL, NULL);
			mcDelete(dir[2]-'0', 0, &dir[4]);
			mcSync(MC_WAIT, NULL, &ret);
		}else if(!strncmp(path, "hdd", 3)){
			ret = fileXioRemove(hdddir);
		}else if(!strncmp(path, "mass", 4)){
			ret = fioRemove(dir);
		}else if(!strncmp(path, "host", 4)){
			ret = fioRemove(dir);
		}
	}
	return ret;
}

//-------------------------------------------------
// ファイル・フォルダリネーム
int Rename(const char *path, const FILEINFO *file, const char *name)
{
	char party[MAX_NAME], oldPath[MAX_PATH], newPath[MAX_PATH];
	int ret=0;

	if(!strncmp(path, "hdd", 3)){
		sprintf(party, "hdd0:%s", &path[6]);
		*strchr(party, '/')=0;
		sprintf(oldPath, "pfs0:%s", strchr(&path[6], '/')+1);
		sprintf(newPath, "%s%s", oldPath, name);
		strcat(oldPath, file->name);

		ret = mountParty(party);
		if(ret<0) return -1;
		oldPath[3] = newPath[3] = ret+'0';

		ret=fileXioRename(oldPath, newPath);
	}else if (!strncmp(path, "mc", 2)) {
		//sprintf(dir, "%s%s", path+4, name);
		sprintf(oldPath, "%s%s", path+4, file->name);
		sprintf(newPath, "%s%s", path+4, name);
		mcSync(MC_WAIT, NULL, NULL);
		ret = mcRename(path[2]-'0', 0, oldPath, newPath);
		//printf("mcRename: %d\nold: %s\nnew: %s\n", ret, oldPath, newPath);
		mcSync(MC_WAIT, NULL, &ret);
		//printf("mcSync(rename): %d\n", ret);
	}else{
		// dopenして書き換えれば良いのだろうか
		return -1;
	}
	
	return ret;
}

//-------------------------------------------------
// 新規フォルダ作成
int newdir(const char *path, const char *name)
{
	char party[MAX_NAME], dir[MAX_PATH];
	int ret=0;

	if(!strncmp(path, "hdd", 3)){
		getHddParty(path,NULL,party,dir);
		ret = mountParty(party);
		if(ret<0) return -1;
		dir[3] = ret+'0';
		//fileXioChdir(dir);
		strcat(dir, name);
		ret = fileXioMkdir(dir, fileMode);
	}else if(!strncmp(path, "mc", 2)){
		sprintf(dir, "%s%s", path+4, name);
		mcSync(MC_WAIT, NULL, NULL);
		mcMkDir(path[2]-'0', 0, dir);
		mcSync(MC_WAIT, NULL, &ret);
		if(ret == -4)
			ret = -17;
	}else if(!strncmp(path, "mass", 4)){
		strcpy(dir, path);
		strcat(dir, name);
		ret = fioMkdir(dir);
	}else if(!strncmp(path, "host", 4)){
		ret = -1;
	}else{
		ret = -1;
	}

	return ret;
}

//-------------------------------------------------
// ファイルコピー
int copy(const char *outPath, const char *inPath, FILEINFO file, int n)
{
	FILEINFO files[MAX_ENTRY];
	char out[MAX_PATH], in[MAX_PATH], tmp[MAX_PATH],
		*buff=NULL, inParty[MAX_NAME], outParty[MAX_NAME];
	int hddout=FALSE, hddin=FALSE, nfiles, i;
	size_t size, outsize;
	int ret=-1, pfsout=-1, pfsin=-1, in_fd=-1, out_fd=-1, buffSize;
	int copyret;
	mcTable mcDir __attribute__((aligned(64)));	//mcSetFileInfo()用

	//フォルダ名またはファイル名の文字数
	if(!strncmp(outPath, "mc", 2)){
		if(strlen(file.name)>32){
			return -1;
		}
	}
	else if(!strncmp(outPath, "mass", 4)){
		if(strlen(file.name)>128){
			return -1;
		}
	}
	else if(!strncmp(outPath, "hdd", 3)){
		if(strlen(file.name)>256){
			return -1;
		}
	}
	sprintf(out, "%s%s", outPath, file.name);
	sprintf(in, "%s%s", inPath, file.name);
	
	//入力パスのパーティションがマウントしているかチェックする
	if(!strncmp(inPath, "hdd", 3)){
		hddin = TRUE;
		getHddParty(inPath, &file, inParty, in);
		if(!strcmp(inParty, mountedParty[0]))
			pfsin=0;	//pfs0にマウントしている
		else if(!strcmp(inParty, mountedParty[1]))
			pfsin=1;	//pfs1にマウントしている
		else
			pfsin=-1;	//マウントしていない
	}
	//出力パスのパーティションがマウントしているかチェックする
	if(!strncmp(outPath, "hdd", 3)){
		hddout = TRUE;
		getHddParty(outPath, &file, outParty, out);
		if(!strcmp(outParty, mountedParty[0]))
			pfsout=0;	//pfs0にマウントしている
		else if(!strcmp(outParty, mountedParty[1]))
			pfsout=1;	//pfs1にマウントしている
		else
			pfsout=-1;	//マウントしていない
	}

	//入力パスの設定とマウント
	if(hddin){
		if(pfsin<0){	//マウントしていないとき
			if(pfsout==0)
				pfsin=1;	//マウントする番号
			else
				pfsin=0;	//マウントする番号
			//アンマウント
			sprintf(tmp, "pfs%d:", pfsin);
			if(mountedParty[pfsin][0]!=0){
				fileXioUmount(tmp);
				mountedParty[pfsin][0]=0;
			}
			//マウント
			printf("%s mounting\n", inParty);
			if(fileXioMount(tmp, inParty, FIO_MT_RDWR) < 0)
				return -1;
			strcpy(mountedParty[pfsin], inParty);
		}
		in[3]=pfsin+'0';	//入力パス
	}
	else
		sprintf(in, "%s%s", inPath, file.name);	//入力パス
	//出力パスの設定とマウント
	if(hddout){
		if(pfsout<0){	//マウントしていないとき
			if(pfsin==0)
				pfsout=1;	//マウントする番号
			else
				pfsout=0;	//マウントする番号
			//アンマウント
			sprintf(tmp, "pfs%d:", pfsout);
			if(mountedParty[pfsout][0]!=0){
				fileXioUmount(tmp);
				mountedParty[pfsout][0]=0;
			}
			//マウント
			printf("%s mounting\n", outParty);
			if(fileXioMount(tmp, outParty, FIO_MT_RDWR) < 0)
				return -1;
			strcpy(mountedParty[pfsout], outParty);
		}
		out[3]=pfsout+'0';	//出力パス
	}
	else
		sprintf(out, "%s%s", outPath, file.name);	//出力パス

	// フォルダの場合
	if(file.attr & MC_ATTR_SUBDIR){
		// フォルダ作成
		ret = newdir(outPath, file.name);
		if(ret == -17){
			drawDark();
			itoGsFinish();
			itoSwitchFrameBuffers();
			drawDark();
			ret=-1;
			if(title) ret=getGameTitle(outPath, &file, tmp);
			if(ret<0) sprintf(tmp, "%s%s/", outPath, file.name);
			strcat(tmp, "\n");
			strcat(tmp, lang->filer_overwrite);
			ret = MessageBox(tmp, LBF_VER, MB_YESNO);
			if(ret!=IDYES) return -1;
			drawMsg(lang->filer_pasting);
		}
		else if(ret < 0)
			return -1;

		// フォルダの中身を全コピー
		sprintf(out, "%s%s/", outPath, file.name);
		sprintf(in, "%s%s/", inPath, file.name);
		nfiles = getDir(in, files);
		for(i=0; i<nfiles; i++){
			copyret = copy(out, in, files[i], n+1);
			if(copyret < 0) return -1;
		}
		//フォルダのmcSetFileInfo
		if(!strncmp(inPath, "mc", 2) && !strncmp(outPath, "mc", 2)){
			memset(&mcDir, 0, sizeof(mcTable));
			memcpy(&mcDir._create, &file.createtime, 8);
			memcpy(&mcDir._modify, &file.modifytime, 8);
			mcDir.fileSizeByte = file.fileSizeByte;
			mcDir.attrFile = file.attr;
			strcpy(mcDir.name, file.name);

			//
			sprintf(out, "%s%s", outPath, file.name);
			mcGetInfo(out[2]-'0', 0, NULL, NULL, NULL);	//Wakeup call
			mcSync(MC_WAIT, NULL, NULL);
			mcSetFileInfo(out[2]-'0', 0, &out[4], (char*)&mcDir, 0xFFFF); //Fix file stats
			mcSync(MC_WAIT, NULL, NULL);
		}
		return 0;
	}

	// 入力ファイルオープンとファイルサイズ取得
	if(hddin){
		in_fd = fileXioOpen(in, O_RDONLY, fileMode);
		if(in_fd<0) goto error;
		size = fileXioLseek(in_fd,0,SEEK_END);
		fileXioLseek(in_fd,0,SEEK_SET);
	}
	else{
		in_fd = fioOpen(in, O_RDONLY);
		if(in_fd<0) goto error;
		size = fioLseek(in_fd,0,SEEK_END);
		fioLseek(in_fd,0,SEEK_SET);
	}
	// 出力ファイルオープン
	if(hddout){
		// O_TRUNC が利かないため、オープン前にファイル削除
		fileXioRemove(out);
		out_fd = fileXioOpen(out,O_WRONLY|O_TRUNC|O_CREAT,fileMode);
		if(out_fd<0) goto error;
	}
	else{
		out_fd=fioOpen(out, O_WRONLY | O_TRUNC | O_CREAT);
		if(out_fd<0) goto error;
	}

	// メモリに一度で読み込めるファイルサイズだった場合
	/*
	buff = (char*)malloc(size);
	if(buff==NULL){
		buff = (char*)malloc(32768);
		buffSize = 32768;
	}
	else
		buffSize = size;
	*/
	buff = (char*)malloc(32768);
	buffSize = 32768;

	while(size>0){
		// 入力
		if(hddin) buffSize = fileXioRead(in_fd, buff, buffSize);
		else buffSize = fioRead(in_fd, buff, buffSize);

		// 出力
		if(hddout){
			outsize = fileXioWrite(out_fd,buff,buffSize);
			if(buffSize!=outsize){
				fileXioClose(out_fd); out_fd=-1;
				fileXioRemove(out);
				goto error;
			}
		}
		else{
			outsize = fioWrite(out_fd,buff,buffSize);
			if(buffSize!=outsize){
				fioClose(out_fd); out_fd=-1;
				mcSync(MC_WAIT, NULL, NULL);
				mcDelete(out[2]-'0', 0, &out[4]);
				mcSync(MC_WAIT, NULL, NULL);
				goto error;
			}
		}
		size -= buffSize;
	}
	//ファイルのmcSetFileInfo
	if(!strncmp(inPath, "mc", 2) && !strncmp(outPath, "mc", 2)){
		memset(&mcDir, 0, sizeof(mcTable));
		memcpy(&mcDir._create, &file.createtime, 8);
		memcpy(&mcDir._modify, &file.modifytime, 8);
		mcDir.fileSizeByte = file.fileSizeByte;
		mcDir.attrFile = file.attr;
		strcpy(mcDir.name, file.name);

		//
		mcGetInfo(out[2]-'0', 0, NULL, NULL, NULL);	//Wakeup call
		mcSync(MC_WAIT, NULL, NULL);
		mcSetFileInfo(out[2]-'0', 0, &out[4], (char*)&mcDir, 0xFFFF); //Fix file stats
		mcSync(MC_WAIT, NULL, NULL);
	}
	ret=0;
error:
	free(buff);
	if(in_fd>0){
		if(hddin) fileXioClose(in_fd);
		else fioClose(in_fd);
	}
	if(out_fd>0){	//修正した
		if(hddout) fileXioClose(out_fd);
		else fioClose(out_fd);
	}
	return ret;
}

//-------------------------------------------------
// ペースト
int paste(const char *path)
{
	char tmp[MAX_PATH];
	int i, ret=-1;

	//コピー先とコピー元が同じ
	if(!strcmp(path,clipPath)) return -1;

	//ペースト処理
	for(i=0; i<nclipFiles; i++){
		strcpy(tmp, clipFiles[i].name);
		if(clipFiles[i].attr & MC_ATTR_SUBDIR) strcat(tmp,"/");
		strcat(tmp, " ");
		strcat(tmp, lang->filer_pasting);
		drawMsg(tmp);
		ret=copy(path, clipPath, clipFiles[i], 0);
		//コピー失敗したら中断
		if(ret < 0) break;
		//切り取りフラグが立っていたら削除する
		if(cut){
			ret=delete(clipPath, &clipFiles[i]);
			//削除失敗したら中断
			if(ret<0) break;
		}
	}
	//HDDアンマウント
	if(mountedParty[0][0]!=0){
		fileXioUmount("pfs0:");
		mountedParty[0][0]=0;
	}
	if(mountedParty[1][0]!=0){
		fileXioUmount("pfs1:");
		mountedParty[1][0]=0;
	}

	return ret;
}

#ifdef ENABLE_PSB
//-------------------------------------------------
//psbCommand psbコマンド実行
int psbCommand(void)
{
	int cmd;
	char path[2][MAX_PATH];
	char dir[MAX_PATH];
	char pathtmp[MAX_PATH];
	int len;
	FILEINFO file;
	char *p;
	int fd;
	int ret=0;
	char message[2048];
	FILEINFO clipFilesBackup;
	int nclipFilesBackup;

	//引数が足りない
	if(psb_argc<2)
		return 0;

	//コマンドチェック
	cmd=-1;
	if(!strnicmp(psb_argv[0], "copy", 4))
		cmd=0;
	else if(!strnicmp(psb_argv[0], "move", 4))
		cmd=1;
	else if(!strnicmp(psb_argv[0], "del", 3))
		cmd=2;
	else if(!strnicmp(psb_argv[0], "mkdir", 5))
		cmd=3;
	else if(!strnicmp(psb_argv[0], "rmdir", 5))
		cmd=4;

	if(cmd==-1) return 0;

	strcpy(path[0], psb_argv[1]);
	strcpy(path[1], psb_argv[2]);

	//Module
	if(!strncmp(path[0], "cdfs", 4)||!strncmp(path[1], "cdfs", 4))
		loadCdModules();
	else if(!strncmp(path[0], "hdd", 3)||!strncmp(path[1], "hdd", 3)){
		if(nparties==0){
			loadHddModules();
			setPartyList();
		}
	}
	else if(!strncmp(path[0], "mass", 4)||!strncmp(path[1], "mass", 4))
		loadUsbMassModules();

	//path[0]の最後がスラッシュのとき削除
	len = strlen(path[0]);
	if(len>0){
		if(path[0][len-1]=='/')
			path[0][len-1]='\0';
	}
	//path[1]の最後にスラッシュ無いときつける
	if(psb_argc>1){
		len = strlen(path[1]);
		if(len>0){
			if(path[1][len-1]!='/')
				strcat(path[1], "/");
		}
	}

	//path[0]からFILEINFOを作成する
	//file.name
	if((p = strrchr(path[0], '/')))
		strcpy(file.name, p+1);
	//file.attr
	ret=0;
	if(!strncmp(path[0], "mc", 2)){
		mcTable mcDir __attribute__((aligned(64)));
		int mcret;
		//フォルダとしてオープンしてみる
		strcpy(pathtmp, path[0]+4); strcat(pathtmp,"/*");
		mcGetDir(path[0][2]-'0', 0, pathtmp, 0, 1, &mcDir);
		mcSync(MC_WAIT, NULL, &mcret);
		if(mcret<0){
			//失敗したらファイルとしてオープンしてみる
			fd = fioOpen(path[0], O_RDONLY);
			if(fd<0)
				ret=-1;	//失敗
			else{
				fioClose(fd);
				file.attr=0;	//ファイル
			}
		}
		else{
			file.attr=MC_ATTR_SUBDIR;	//フォルダ
		}
	}
	else if(!strncmp(path[0], "cdfs", 4)){
		//フォルダとしてオープンしてみる
		struct TocEntry TocEntryList;
		char cdfsdir[MAX_PATH];
		int n;
		strcpy(cdfsdir, path[0]+5);
		CDVD_FlushCache();
		n = CDVD_GetDir(cdfsdir, NULL, CDVD_GET_FILES_AND_DIRS, &TocEntryList, 1, cdfsdir);
		if(n<0){
			//失敗したらファイルとしてオープンしてみる
			fd = fioOpen(path[0], O_RDONLY);
			if(fd<0)
				ret=-1;	//失敗
			else{
				fioClose(fd);
				file.attr=0;	//ファイル
			}
		}
		else{
			file.attr=MC_ATTR_SUBDIR;	//フォルダ
		}
		if(setting->discControl) CDVD_Stop();
	}
	else if(!strncmp(path[0], "hdd", 3)){
		char party[MAX_NAME];
		int r;
		//ファイルがhddにあるときパスを変更
		getHddParty(path[0], NULL, party, pathtmp);
		//マウント
		r = mountParty(party);
		pathtmp[3] = r+'0';

		//フォルダとしてオープンしてみる
		fd = fileXioDopen(pathtmp);
		if(fd<0){
			//失敗したらファイルとしてオープンしてみる
			fd = fileXioOpen(pathtmp, O_RDONLY, fileMode);
			if(fd<0){
				ret=-1;	//失敗
			}
			else{
				fileXioClose(fd);
				file.attr=0;	//ファイル
			}
		}
		else{
			fileXioDclose(fd);
			file.attr=MC_ATTR_SUBDIR;
		}
	}
	else if(!strncmp(path[0], "mass", 4)){
		//フォルダとしてオープンしてみる
		strcpy(pathtmp, path[0]);
		fd = fioDopen(pathtmp);
		if(fd<0){
			//失敗したらファイルとしてオープンしてみる
			fd = fioOpen(path[0], O_RDONLY);
			if(fd<0)
				ret=-1;	//失敗
			else{
				fioClose(fd);
				file.attr=0;	//ファイル
			}
		}
		else{
			file.attr=MC_ATTR_SUBDIR;	//フォルダ
			fioDclose(fd);
		}
	}
	if(cmd!=3 && ret==-1) return 0;	//file.attr取得失敗

	//path[0]がある親ディレクトリ名
	strcpy(dir, path[0]);
	if((p=strrchr(dir, '/'))){
		p++;
		*p=0;
	}

	//クリップボード情報をバックアップ
	clipFilesBackup = clipFiles[0];
	nclipFilesBackup = nclipFiles;

	//コマンド
	switch(cmd)
	{
		case 0:	//コピー
		{
			//
			clipFiles[0]=file;
			nclipFiles = 1;
			//コピー元のディレクトリ名(clipPath)
			strcpy(clipPath, dir);
			//
			cut=FALSE;	//コピー
			//ペースト開始
			ret=paste(path[1]);
			if(ret) MessageBox("copy Failed", LBF_VER, MB_OK);
			break;
		}
		case 1:	//移動
		{
			//
			clipFiles[0]=file;
			nclipFiles = 1;
			//コピー元のディレクトリ名(clipPath)
			strcpy(clipPath, dir);
			//
			cut=TRUE;	//移動
			//ペースト開始
			ret=paste(path[1]);
			if(ret) MessageBox("move Failed", LBF_VER, MB_OK);
			break;
		}
		case 2:	//削除
		{
			int ynret;
			sprintf(message, "%s", file.name);
			if(file.attr & MC_ATTR_SUBDIR) strcat(message, "/");
			strcat(message, "\n");
			strcat(message, lang->filer_delete);
			ynret = MessageBox(message, LBF_VER, MB_YESNO);
			if(ynret==IDYES){
				//削除開始
				ret=delete(dir, &file);
			}
			if(ret) MessageBox("del Failed", LBF_VER, MB_OK);
			break;
		}
		case 3:	//フォルダを作成
		{
			//作成開始
			ret=newdir(dir, file.name);
			if(ret) MessageBox("mkdir Failed", LBF_VER, MB_OK);
			break;
		}
		case 4:	//フォルダを削除
		{
			int ynret;
			//ディレクトリ削除
			if(file.attr==MC_ATTR_SUBDIR){
				sprintf(message, "%s", file.name);
				if(file.attr & MC_ATTR_SUBDIR) strcat(message, "/");
				strcat(message, "\n");
				strcat(message, lang->filer_delete);
				ynret = MessageBox(message, LBF_VER, MB_YESNO);
				if(ynret==IDYES){
					//削除開始
					ret=delete(dir, &file);
				}
			}
			else{
				ret=-1;
			}
			if(ret) MessageBox("rmdir Failed", LBF_VER, MB_OK);
			break;
		}
	}

	//クリップボード情報を元に戻す
	clipFiles[0] = clipFilesBackup;
	nclipFiles = nclipFilesBackup;

	return ret;
}

//-------------------------------------------------
//psbParse psbファイルパース
void psbParse(const char *str)
{
	char *p;
	int len;
	int i;
	int flag;
	int l;
	char strtmp[(MAX_PATH+2)*MAX_ARGC];
	char tmp[MAX_PATH+2];

	strcpy(strtmp, str);

	//初期化
	psb_argc=0;
	memset(psb_argv, 0, sizeof(psb_argv));

	//改行コードを消す
	p=strrchr(strtmp,'\n');
	if(p!=NULL) *p='\0';
	p=strrchr(strtmp,'\r');
	if(p!=NULL) *p='\0';

	len=strlen(strtmp);

	//
	if(len==0) return;

	//スペースを\0に変換
	flag=0;
	for(i=0;i<len;i++){
		if(strtmp[i]=='\"')	//ダブルクォーテーションフラグ
			flag=1-flag;
		else if((strtmp[i]==' ')&&(flag==0))
			strtmp[i]='\0';
	}

	//\0区切りを読みとる
	for(i=0;i<len;i++){
		if(strtmp[i]=='\0'){
			//何もしない
		}
		else{
			l=strlen(strtmp+i);
			if(psb_argc<MAX_ARGC){
				if(l<MAX_PATH+2)
					strcpy(psb_argv[psb_argc], strtmp+i);
				psb_argc++;
			}
			i+=l;
		}
	}

	//ダブルクォーテーションを消す
	for(i=0;i<psb_argc;i++){
		if(psb_argv[i][0]=='\"'){
			//前のダブルクォーテーションを消す
			strcpy(tmp, &psb_argv[i][1]);
			strcpy(psb_argv[i],tmp);
			//後ろのダブルクォーテーションを消す
			p=strrchr(psb_argv[i],'\"');
			if(p!=NULL) *p='\0';
		}
	}
}

//-------------------------------------------------
//psbファイル実行
//戻り値     0:成功
//          -1:ファイルオープン失敗
//       0以上:エラーが出た行番号
int psb(const char *psbpath)
{
	int fd;
	char buffer[(MAX_PATH+2)*MAX_ARGC];
	int lineno;
	int ret;

	//
	if(!strncmp(psbpath, "cdfs", 4))
		loadCdModules();
	else if(!strncmp(psbpath, "mass", 4))
		loadUsbMassModules();

	//
	fd=fioOpen(psbpath, O_RDONLY);
	if(fd<0) return -1;

	lineno=0;
	while(1){
		memset(buffer, 0, sizeof(buffer));
		if(fioGets(fd, buffer, (MAX_PATH+2)*MAX_ARGC)==0)
			break;
		if(buffer[0]=='\n'){
		}
		else{
			lineno++;
			//パース
			psbParse(buffer);
			//実行
			if(psbCommand()<0){
				ret=lineno;
				goto psberror;	//エラーでたら停止
			}
		}
	}
	ret=0;	//エラーなし
psberror:
	fioClose(fd);
	return ret;
}
#endif

//-------------------------------------------------
//windowsでファイル名に使えない文字は「_」に変換
void filenameFix(const unsigned char *in, unsigned char *out)
{
	int len;
	int i=0;

	len = strlen(in);
	memcpy(out, in, len+1);
	for(i=0;i<len;i++){
		if(out[i]==0x22) out[i]='_';	// '"'
		if(out[i]==0x2A) out[i]='_';	// '*'
		if(out[i]==0x2C) out[i]='_';	// ','
		if(out[i]==0x2F) out[i]='_';	// '/'
		if(out[i]==0x3A) out[i]='_';	// ':'
		if(out[i]==0x3B) out[i]='_';	// ';'
		if(out[i]==0x3C) out[i]='_';	// '<'
		if(out[i]==0x3E) out[i]='_';	// '>'
		if(out[i]==0x3F) out[i]='_';	// '?'
		if(out[i]==0x5c){	// '\'
			if(i>0){
				if((out[i-1]&0x80)==0) out[i]='_';
			}
		}
		if(out[i]==0x7C) out[i]='_';	// '|'
	}
}

//-------------------------------------------------
//ゲームタイトルのsjisの英数字と記号をASCIIに変換
void sjis2ascii(const unsigned char *in, unsigned char *out)
{
	int i=0;
	int code;
	unsigned char ascii;
	int n=0;

	while(in[i]){
		if(in[i] & 0x80){
			// SJISコードの生成
			code = in[i++];
			code = (code<<8) + in[i++];

			ascii=0xFF;
			if(code>>8==0x81)
				ascii = sjis_lookup_81[code & 0x00FF];
			else if(code>>8==0x82)
				ascii = sjis_lookup_82[code & 0x00FF];

			if(ascii!=0xFF){
				out[n]=ascii;
				n++;
			}
			else{
				//ASCIIに変換できない文字
				out[n]=code>>8&0xFF;
				out[n+1]=code&0xFF;
				n=n+2;
			}
		}
		else{
			out[n]=in[i];
			n++;
			i++;
		}
	}
}

//-------------------------------------------------
//psuファイルからインポート
//戻り値
//0以下:失敗
//    0:mc0にインポート成功
//    1:mc1にインポート成功
int psuImport(const char *path, const FILEINFO *file, int outmc)
{
	//
	int ret = -1;	//戻り値
	int n = 0;
	PSU_HEADER psu_header[MAX_ENTRY];
	PSU_HEADER psu_header_dir;
	char title[16*4+1]="";
	unsigned char disp[40];
	char *buff=NULL;

	int in_fd = -1, out_fd = -1;
	int hddin = FALSE;
	int i;

	int dialog_x;		//ダイアログx位置
	int dialog_y;		//ダイアログy位置
	int dialog_width;	//ダイアログ幅 
	int dialog_height;	//ダイアログ高さ

	//フォルダのときは、psuからインポートできない
	if(file->attr & MC_ATTR_SUBDIR){
		return -1;
	}

	//step1 psuヘッダ読み込み
	{
		char inpath[MAX_PATH];	//選択されたフォルダまたはファイルのフルパス
		char tmp[2048];		//雑用
		char party[MAX_NAME];
		int r;
		int psuSize;
		int seek;
		int fileSize;	//ファイルのサイズ

		//psuファイルがhddのあるときパスを変更
		if(!strncmp(path, "hdd", 3)){
			hddin = TRUE;
			getHddParty(path, NULL, party, inpath);
			//pfs0にマウント
			r = mountParty(party);
			if(r<0) return -2;
			inpath[3] = r+'0';
			//psuファイルのフルパス
			strcat(inpath, file->name);
		}
		else{
			//psuファイルのフルパス
			sprintf(inpath, "%s%s", path, file->name);
		}

		//psuファイルオープンとサイズ取得
		if(hddin){
			in_fd = fileXioOpen(inpath, O_RDONLY, fileMode);
			if(in_fd<0){
				ret=-3;
				goto error;
			}
			psuSize = fileXioLseek(in_fd, 0, SEEK_END);
			fileXioLseek(in_fd, 0, SEEK_SET);
		}
		else{
			in_fd = fioOpen(inpath, O_RDONLY);
			if(in_fd<0){
				ret=-3;
				goto error;
			}
			psuSize = fioLseek(in_fd, 0, SEEK_END);
			fioLseek(in_fd, 0, SEEK_SET);
		}

		//psuヘッダ読み込む
		if(psuSize<sizeof(PSU_HEADER)){
			ret=-4;
			goto error;
		}
		//psuヘッダを読み込む
		memset(&psu_header_dir, 0, sizeof(PSU_HEADER));
		if(hddin) fileXioRead(in_fd, (char*)&psu_header_dir, sizeof(PSU_HEADER));
		else fioRead(in_fd, &psu_header_dir, sizeof(PSU_HEADER));
		n = psu_header_dir.size;	//ファイル数
		seek = sizeof(PSU_HEADER);	//ファイルのシーク
		if (!is_psu((unsigned char*)&psu_header_dir, sizeof(PSU_HEADER))) {
			ret=-90;
			goto error;
		}

		//psu_header[0]から読み込む
		for(i=0;i<n;i++){
			if(psuSize<seek+sizeof(PSU_HEADER)){
				ret=-5;
				goto error;
			}
			memset(&psu_header[i], 0, sizeof(PSU_HEADER));
			if(hddin) fileXioRead(in_fd, (char*)&psu_header[i], sizeof(PSU_HEADER));
			else fioRead(in_fd, &psu_header[i], sizeof(PSU_HEADER));
			seek += sizeof(PSU_HEADER);
			//ゲームタイトル
			if(!strcmp(psu_header[i].name,"icon.sys")){
				if(hddin){
					fileXioLseek(in_fd, seek+0xC0, SEEK_SET);
					fileXioRead(in_fd, tmp, 64);
					title[64]=0;
					fileXioLseek(in_fd, seek, SEEK_SET);
				}
				else{
					fioLseek(in_fd, seek+0xC0, SEEK_SET);
					fioRead(in_fd, tmp, 64);
					title[64]=0;
					fioLseek(in_fd, seek, SEEK_SET);
				}
				sjis2ascii(tmp, title);
			}
			//次のファイルヘッダの位置にシーク
			if(psu_header[i].size>0){
				fileSize = (((psu_header[i].size-1)/0x400)+1)*0x400;
				if(psuSize<seek + fileSize){
					ret=-6;
					goto error;
				}
				seek += fileSize;
				if(hddin) fileXioLseek(in_fd, seek, SEEK_SET);
				else fioLseek(in_fd, seek, SEEK_SET);
			}
		}
		//psuファイルクローズ
		if(hddin){
			hddin = FALSE;
			fileXioClose(in_fd);
		}
		else
			fioClose(in_fd);
		in_fd = -1;
	}
	//step2 インポート開始
	{
		char inpath[MAX_PATH];	//psuファイルのフルパス
		char outpath[MAX_PATH];//セーブデータのフォルダを出力するフォルダのフルパス
		int seek;
		char tmp[2048];		//雑用 表示用
		char out[MAX_PATH];	//セーブデータのフォルダのフルパス
		size_t outsize;
		char party[MAX_NAME];
		int r;
		mcTable mcDir __attribute__((aligned(64)));	//mcSetFileInfo()用

		//セーブデータのフォルダを出力するフォルダのフルパス
		if(outmc==0)
			strcpy(outpath, "mc0:/");
		else if(outmc==1)
			strcpy(outpath, "mc1:/");

		//psuファイル
		if(!strncmp(path, "hdd", 3)){
			hddin = TRUE;
			getHddParty(path, NULL, party, inpath);
			//pfs0にマウント
			r = mountParty(party);
			if(r<0) return -8;
			inpath[3] = r+'0';
			//psuファイルのフルパス
			strcat(inpath, file->name);
		}
		else{
			//psuファイルのフルパス
			sprintf(inpath, "%s%s", path, file->name);
		}

		//セーブデータのフォルダ作成
		if (psu_header_dir.name[0] == 0) {
			ret = -7;
			goto error;
		}
		r = newdir(outpath, psu_header_dir.name);
		/*
		if(r == -17){	//フォルダがすでにあるとき上書きを確認する
			drawDark();
			itoGsFinish();
			itoSwitchFrameBuffers();
			drawDark();
			sprintf(tmp, "%s%s/\n%s", outpath, psu_header_dir.name, lang->filer_overwrite);
			ret = MessageBox(tmp, LBF_VER, MB_YESNO);
			if(ret!=IDYES){
				ret = -7;
				goto error;
			}
		}
		/*/
		if(r == -17) {
		}
		//*/
		else if(r < 0){//フォルダ作成失敗
			ret = -9;
			goto error;
		}

		//psuファイルオープン
		if(hddin){
			in_fd = fileXioOpen(inpath, O_RDONLY, fileMode);
			if(in_fd<0){
				ret=-10;
				goto error;
			}
		}
		else{
			in_fd = fioOpen(inpath, O_RDONLY);
			if(in_fd<0){
				ret=-10;
				goto error;
			}
		}


		//
		dialog_width = FONT_WIDTH*40;
		dialog_height = FONT_HEIGHT*6;
		dialog_x = (SCREEN_WIDTH-dialog_width)/2;
		dialog_y = (SCREEN_HEIGHT-dialog_height)/2;
		/*
		drawDark();
		itoGsFinish();
		itoSwitchFrameBuffers();
		drawDark();
		*/
		
		//
		seek = sizeof(PSU_HEADER);
		if (strlen(file->name) > 39) {
			for(i=0,r=0;i<36;i++) {
				disp[i] = file->name[i];
				if (r==0 && ((disp[i]>0x80 && disp[i] < 0xA0)||(disp[i]>=0xE0 && disp[i]<0xFD))) r=2;
				r--;
				if (disp[i] == 0) break;
			}
			disp[i] = 0;
			if (r) disp[i-1] = 0;
			strcat(disp, "...");
		} else {
			strncpy(disp, file->name, 39);
			disp[39] = 0;
		}
		for(i=0;i<n;i++){
			for (r=0; r<fieldbuffers; r++) {
				//ダイアログ
				drawDialogTmp(dialog_x, dialog_y,
					dialog_x+dialog_width, dialog_y+dialog_height,
					setting->color[COLOR_BACKGROUND], setting->color[COLOR_FRAME]);
				//メッセージ
				printXY(lang->filer_importing, dialog_x+FONT_WIDTH*0.5, dialog_y+FONT_HEIGHT*0.5, setting->color[COLOR_TEXT], TRUE);
				printXY(disp, dialog_x+FONT_WIDTH*0.5, dialog_y+FONT_HEIGHT*1.5, setting->color[COLOR_TEXT], TRUE);
				sprintf(tmp, "%2d / %2d", i+1, n);
				printXY(tmp, dialog_x+FONT_WIDTH*0.5, dialog_y+FONT_HEIGHT*2.5, setting->color[COLOR_TEXT], TRUE);
				//プログレスバーの枠
				drawDialogTmp(dialog_x+FONT_WIDTH*0.5, dialog_y+FONT_HEIGHT*3.5,
					dialog_x+dialog_width-FONT_WIDTH*0.5, dialog_y+FONT_HEIGHT*5.5,
					setting->color[COLOR_BACKGROUND], setting->color[COLOR_FRAME]);
				//プログレスバー
				itoSprite(setting->color[COLOR_FRAME],
					dialog_x+FONT_WIDTH, dialog_y+FONT_HEIGHT*4,
					dialog_x+FONT_WIDTH+(dialog_width-FONT_WIDTH*2)*((i+1)*100/n)/100, dialog_y+FONT_HEIGHT*5, 0);
				drawScr();
			}
			//
			seek += sizeof(PSU_HEADER);
			if(psu_header[i].attr&MC_ATTR_SUBDIR){
				//フォルダのときは何もしない
			}
			else{
				//書き込むデータのメモリを確保
				buff = (char*)malloc(psu_header[i].size);
				if(buff==NULL){
					ret=-11;
					goto error;
				}
				//出力するファイルオープン
				sprintf(out, "%s%s/%s", outpath, psu_header_dir.name, psu_header[i].name);
				out_fd = fioOpen(out, O_WRONLY | O_TRUNC | O_CREAT);
				if(out_fd<0){
					ret=-12;
					goto error;
				}
				//読み込み
				memset(buff, 0, psu_header[i].size);
				if(hddin){
					fileXioLseek(in_fd, seek, SEEK_SET);
					fileXioRead(in_fd, buff, psu_header[i].size);
				}
				else{
					fioLseek(in_fd, seek, SEEK_SET);
					fioRead(in_fd, buff, psu_header[i].size);
				}
				//書き込み
				outsize = fioWrite(out_fd, buff, psu_header[i].size);
				if(outsize!=psu_header[i].size){
					ret=-13;
					goto error;
				}
				//クローズ
				fioClose(out_fd); out_fd=-1;
				free(buff);
			
				{	//original source uLaunchELF 4.12
					memset(&mcDir, 0, sizeof(mcTable));
					memcpy(&mcDir._create, &psu_header[i].createtime, 8);
					memcpy(&mcDir._modify, &psu_header[i].modifytime, 8);
					mcDir.fileSizeByte = psu_header[i].size;
					mcDir.attrFile = psu_header[i].attr;
					strcpy(mcDir.name, psu_header[i].name);

					mcGetInfo(out[2]-'0', 0, NULL,NULL, NULL);	//Wakeup call
					mcSync(MC_WAIT, NULL, NULL);
					mcSetFileInfo(out[2]-'0', 0, &out[4], (char*)&mcDir, 0xFFFF);	//Fix file stats
					mcSync(MC_WAIT, NULL, NULL);
				}
				//
				fioLseek(in_fd, seek, SEEK_SET);	//シークをファイルの先頭にに戻す
				seek += (((psu_header[i].size-1)/0x400)+1)*0x400;
				fioLseek(in_fd, seek, SEEK_SET);	//シークを次のファイルヘッダの先頭に移動
			}
		}
		//psuファイルをクローズ
		if(hddin) fileXioClose(in_fd);
		else fioClose(in_fd);
		in_fd=-1;

		//フォルダのmcSetFileInfo
		{
			memset(&mcDir, 0, sizeof(mcTable));
			memcpy(&mcDir._create, &psu_header_dir.createtime, 8);
			memcpy(&mcDir._modify, &psu_header_dir.modifytime, 8);
			mcDir.fileSizeByte = psu_header_dir.size;
			mcDir.attrFile = psu_header_dir.attr;
			strcpy(mcDir.name, psu_header_dir.name);

			//
			sprintf(out, "%s%s", outpath, psu_header_dir.name);
			mcGetInfo(out[2]-'0', 0, NULL, NULL, NULL);	//Wakeup call
			mcSync(MC_WAIT, NULL, NULL);
			mcSetFileInfo(out[2]-'0', 0, &out[4], (char*)&mcDir, 0xFFFF); //Fix file stats
			mcSync(MC_WAIT, NULL, NULL);
		}
	}
	//
	ret=outmc;
error:
	free(buff);
	if(in_fd>=0){
		if(hddin) fileXioClose(in_fd);
		else fioClose(in_fd);
	}
	if(out_fd>=0){
		fioClose(out_fd);
	}

	return ret;
}

//-------------------------------------------------
// psuファイルにエクスポート
void psuheader(char *dist, unsigned short attr, unsigned int size, PS2TIME *create, PS2TIME *modify, char *name)
{
	PSU_HEADER psu_header;
	
	memset(&psu_header, 0, sizeof(PSU_HEADER));
	psu_header.attr = attr;
	psu_header.size = size;
	memcpy(&psu_header.createtime, create, 8);
	memcpy(&psu_header.modifytime, modify, 8);
	strcpy(psu_header.name, name);
	
	memcpy(dist, &psu_header, sizeof(PSU_HEADER));
}
int psuExport(const char *path, const FILEINFO *file, int sjisout)
{
	int ret = 0;	//戻り値
	int n = 0;

	mcTable mcDir[MAX_ENTRY] __attribute__((aligned(64)));
	int mcret;
	int r;

	char inpath[MAX_PATH];		//選択されたフォルダのフルパス
	char outpath[MAX_PATH];	//出力するpsuファイル名
	char tmps[MAX_PATH], tmp[MAX_PATH];	//列挙用パターン
	char party[MAX_NAME];
	char *buff=NULL;
	int fd, hddout = FALSE, i;

	int dialog_x;		//ダイアログx位置
	int dialog_y;		//ダイアログy位置
	int dialog_width;	//ダイアログ幅
	int dialog_height;	//ダイアログ高さ
	int dialog_l, dialog_t, dialog_r, dialog_b;
	size_t total,total1k,wptr,psusize,readSize,writeSize;
	uint64 oldcount=0;

	//ファイルのときは、psuにエクスポートできない
	if(!(file->attr & MC_ATTR_SUBDIR)){	//ファイル
		return -1;
	}

	//選択されたフォルダのフルパス
	sprintf(inpath, "%s%s", path, file->name);

	//出力するフォルダ名
	if(setting->Exportdir[0])
		strcpy(outpath, setting->Exportdir);
	else
		strcpy(outpath, path);
	
	if(!strncmp(outpath, "cdfs", 2)){
		return -5;
	}

	dialog_width = FONT_WIDTH*40;
	dialog_height = FONT_HEIGHT*6;
	dialog_x = (SCREEN_WIDTH-dialog_width)/2;
	dialog_y = (SCREEN_HEIGHT-dialog_height)/2;
	dialog_l = dialog_x + FONT_WIDTH;
	dialog_t = dialog_y + FONT_HEIGHT * 4;
	dialog_r = dialog_l + dialog_width - FONT_WIDTH * 2;
	dialog_b = dialog_t + FONT_HEIGHT;
	//リスト読み込み
	sprintf(tmps, "%s/*", &inpath[4]);
	memset(&mcDir, 0, sizeof(mcTable)*2);
	mcSync(MC_WAIT, NULL, &mcret);
	mcGetDir(inpath[2]-'0', 0, tmps, 0, MAX_ENTRY-2, mcDir);
	mcSync(MC_WAIT, NULL, &n);	//ファイル数
	//mcDir[0]の情報
	mcDir[0].fileSizeByte=0;
	mcDir[0].attrFile=0x8427;
	strcpy(mcDir[0].name,".");
	memset(&mcDir[1]._modify, 0, 8);

	for (i=0,total=total1k=0; i<n; i++)	total1k += (mcDir[i].fileSizeByte + 1023) & ~1023;
	psusize = total1k + (n+1)*sizeof(PSU_HEADER);

	//printf("psuExport: psusize: %d\n", psusize);
	buff = (char*)malloc(psusize);
	if (!buff) return -10;
	memset(buff, 0, psusize);
	
	wptr = sizeof(PSU_HEADER);
	psuheader(buff, file->attr, n, (PS2TIME*)&file->createtime, (PS2TIME*)&file->modifytime, (char*)&file->name);
	for (i=0;i<n;i++) {
		writeSize = (mcDir[i].fileSizeByte + 1023) & ~1023;
		for (r=0; r<fieldbuffers; r++) {
			//ダイアログ
			drawDialogTmp(dialog_x, dialog_y,
				dialog_x+dialog_width, dialog_y+dialog_height,
				setting->color[COLOR_BACKGROUND], setting->color[COLOR_FRAME]);
			//メッセージ
			printXY(lang->filer_exporting, dialog_x+FONT_WIDTH*0.5, dialog_y+FONT_HEIGHT*0.5, setting->color[COLOR_TEXT], TRUE);
			printXY(file->name, dialog_x+FONT_WIDTH*0.5, dialog_y+FONT_HEIGHT*1.5, setting->color[COLOR_TEXT], TRUE);
			sprintf(tmps, " %3d / %3d ( %3d%% )", i+1, n, (wptr + writeSize + sizeof(PSU_HEADER)) * 50 / psusize);
			printXY(tmps, dialog_x+FONT_WIDTH*0.5, dialog_y+FONT_HEIGHT*2.5, setting->color[COLOR_TEXT], TRUE);
			drawBar(dialog_l, dialog_t, dialog_r, dialog_b, setting->color[COLOR_FRAME], 0, wptr + writeSize, psusize << 1);
			drawScr();
		}
		psuheader(buff+wptr, mcDir[i].attrFile, mcDir[i].fileSizeByte, (PS2TIME*)&mcDir[i]._create, (PS2TIME*)&mcDir[i]._modify, mcDir[i].name);
		wptr += sizeof(PSU_HEADER);
		if (mcDir[i].fileSizeByte > 0) {
			sprintf(tmps, "%s/%s", inpath, mcDir[i].name);
			//ファイルオープン
			fd = fioOpen(tmps, O_RDONLY);
			if (fd < 0) {
				ret = -11;
				break;
			}
			//読み込む
			readSize = fioRead(fd, buff+wptr, mcDir[i].fileSizeByte);
			//クローズ
			fioClose(fd);
			if (readSize!=mcDir[i].fileSizeByte) {
				ret = -12;
				break;
			}
			wptr += writeSize;
		}
	}
	printf("psuExport: ret=%d, wptr=%d / %d, CRC32=%08X\n", ret, wptr, psusize, CRC32Check(buff, psusize));
	if (ret) {
		free(buff);
		return ret;
	}

	//出力するpsuファイル名
	if(sjisout==TRUE){
		if(file->title[0])
			filenameFix(file->title, tmp);	//ファイル名に使えない文字を変換
		else
			filenameFix(file->name, tmp);
	}
	else{
		filenameFix(file->name, tmp);
	}

	//出力先がmcのときにファイル名の文字数を調べる
	if(!strncmp(outpath, "mc", 2)){
		//ファイル名の最大 mc:32byte mass:128byte hdd:256byte
		if(strlen(tmp)>28){	//ファイル名が長いときに短くする
			tmp[28] = 0;
			i = (unsigned char) tmp[27];
			//sjisの1byte目だったら消す
			if( (i >= 0x81) && (i <= 0x9F) ) tmp[27] = 0;
			if( (i >= 0xE0) && (i <= 0xFC) ) tmp[27] = 0;
		}
	} else {
		// 必要に応じてファイル名に更新日時やCRCを追加(mc以外のとき)
		if (setting->exportname & 1) {
			// 更新日時
			// 形式: _YmdHi ( _201007012126 )
			sprintf(tmps, "_%04d%02d%02d%02d%02d", file->modifytime.year, file->modifytime.month, file->modifytime.day, file->modifytime.hour, file->modifytime.min);
			strcat(tmp, tmps);
		}
		if (setting->exportname & 2) {
			// CRC32
			// 形式: _%08X ( _2942CD19 )
			sprintf(tmps, "_%08X", CRC32Check(buff, psusize));
			strcat(tmp, tmps);
		}
	}
	//出力するpsuファイルのフルパス
	strcat(outpath, tmp);
	strcat(outpath, ".psu");

	if(!strncmp(outpath, "mc", 2)){
		int type;
		//
		mcGetInfo(outpath[2]-'0', 0, &type, NULL, NULL);
		mcSync(MC_WAIT, NULL, NULL);
		if(type!=MC_TYPE_PS2) ret = -3;
	}
	//出力するpsuファイルがhddのときパスを変更
	else if(!strncmp(outpath, "hdd", 3)){
		if(nparties==0){
			loadHddModules();
			setPartyList();
		}
		hddout = TRUE;
		getHddParty(outpath, NULL, party, tmp);
		//pfs0にマウント
		r = mountParty(party);
		if(r<0)	ret = -4;
		else {
			strcpy(outpath, tmp);
			outpath[3] = r+0x30;
		}
	}
	else if(!strncmp(outpath, "mass", 4)){
		loadUsbMassModules();
	}

	if (!ret) {
		//psuファイルオープン 新規作成
		if(hddout){
			// O_TRUNC が利かないため、オープン前にファイル削除
			fileXioRemove(outpath);
			fd = fileXioOpen(outpath, O_WRONLY|O_TRUNC|O_CREAT, fileMode);
		} else {	//mc mass
			fd = fioOpen(outpath, O_WRONLY | O_TRUNC | O_CREAT);
		}
		if (fd>=0) {
			//psuファイルに書き込み
			for(i=0,writeSize=0; i<psusize; i+=readSize) {
				if (i+4096>psusize) readSize = psusize - i; else readSize = 4096;
				for (r=0; r<fieldbuffers; r++) {
					//ダイアログ
					if (oldcount != totalcount) {
						drawDialogTmp(dialog_x, dialog_y,
							dialog_x+dialog_width, dialog_y+dialog_height,
							setting->color[COLOR_BACKGROUND], setting->color[COLOR_FRAME]);
						//メッセージ
						printXY(lang->filer_exporting, dialog_x+FONT_WIDTH*0.5, dialog_y+FONT_HEIGHT*0.5, setting->color[COLOR_TEXT], TRUE);
						printXY(file->name, dialog_x+FONT_WIDTH*0.5, dialog_y+FONT_HEIGHT*1.5, setting->color[COLOR_TEXT], TRUE);
						sprintf(tmps, "%5d KB / %d KB ( %3d%% )", (i+readSize + 512) >> 10, (psusize + 512) >> 10, (i+readSize) * 50 / psusize + 50);
						printXY(tmps, dialog_x+FONT_WIDTH*0.5, dialog_y+FONT_HEIGHT*2.5, setting->color[COLOR_TEXT], TRUE);
						drawBar(dialog_l, dialog_t, dialog_r, dialog_b, setting->color[COLOR_FRAME], 0, psusize + i + readSize, psusize << 1);
						itoGsFinish();
						if (ffmode)
							itoSetActiveFrameBuffer(itoGetActiveFrameBuffer()^1);
						else
							itoSwitchFrameBuffers();
					}
				}
				if(hddout)	readSize = fileXioWrite(fd, buff + i, readSize);
				else		readSize = fioWrite(fd, buff + i, readSize);
				writeSize += readSize;
				if ((readSize!=4096)&&(i+readSize!=psusize)) break;
			}
			if(writeSize!=psusize) ret = -13;
			//psuファイルクローズ
			if(hddout)	fileXioClose(fd);
			else		fioClose(fd);
		} else {
			ret = -6;
		}
	}
	free(buff);
	
	if (ret<0) {
		// エクスポート失敗したときpsuファイルを削除
		if(!strncmp(outpath, "mc", 2)){
			mcDelete(outpath[2]-'0', 0, &outpath[4]);
			mcSync(MC_WAIT, NULL, &r);
		}
		else if(!strncmp(outpath, "pfs", 3)){
			r = fileXioRemove(outpath);
		}
		else if(!strncmp(outpath, "mass", 4)){
			r = fioRemove(outpath);
		}
	}
	return ret;
}
//-------------------------------------------------
// ファイルリスト設定
int setFileList(const char *path, const char *ext, FILEINFO *files, int cnfmode)
{
	char *p;
	int nfiles, i, j, ret=0;
	char fullpath[MAX_PATH];

	int checkELFret;
	char party[MAX_NAME];
	char tmp[16*4+1];

	char dummyElf[16][40] = {
		"..",
		"FileBrowser",
		"PS2Browser",
		"PS2Disc",
		"PS2Ftpd",
		"DiscStop",
		"McFormat",
		"PowerOff",
		"ShowFont",
		"SelfUpdate",
		"INFO",
		"IPCONFIG",
		"GSCONFIG",
		"FMCBCONFIG",
		"CONFIG",
	};
	//struct fileXioDevice devs[128];

	// ファイルリスト設定
	if(path[0]==0){
		//ret = fileXioGetDeviceList(devs, 128);
		//printf("filer: Registed %d devices, are...\n", ret);
		//printf("\x09Cnt. type ver.     name, desc\n");    
		//for (i=0;i<ret;i++){
		//	printf("\x09[%02d] %4d %08X %s, %s\n", i, devs[i].type, devs[i].version, devs[i].name, devs[i].desc);
		//}
		//ret = 0;
		if (cnfmode!=FMB_FILE) {
			for(i=0;i<5+setting->usbmdevs+(setting->usbmdevs==1)*3+(boot==HOST_BOOT)+(setting->vmc_flag!=0);i++){
				memset(&files[i].createtime, 0, sizeof(PS2TIME));
				memset(&files[i].modifytime, 0, sizeof(PS2TIME));
				files[i].fileSizeByte = 0;
				files[i].attr = MC_ATTR_SUBDIR;
				files[i].title[0] = 0;
				files[i].timestamp = 0;
				files[i].num = i;
				if(i==0){
					strcpy(files[i].name, "mc0:");
					files[i].type = TYPE_DEVICE_MC;
				}
				if(i==1){
					strcpy(files[i].name, "mc1:");
					files[i].type = TYPE_DEVICE_MC;
				}
				if(i==2){
					strcpy(files[i].name, "hdd0:");
					files[i].type = TYPE_DEVICE_HDD;
				}
				if(i==3){
					strcpy(files[i].name, "cdfs:");
					files[i].type = TYPE_DEVICE_CD;
				}
				if(i==4){
					strcpy(files[i].name, "mass:");
					files[i].type = TYPE_DEVICE_MASS;
				}
				if(i==5&&boot==HOST_BOOT){
					strcpy(files[i].name, "host:");
					files[i].type = TYPE_DEVICE_HOST;
				}
				if((i>=5+(boot==HOST_BOOT)) && (i<5+(boot==HOST_BOOT)+setting->usbmdevs+(setting->usbmdevs==1)*3)){
					sprintf(files[i].name, "mass%d:", i-5-(boot==HOST_BOOT));
					files[i].type = TYPE_DEVICE_MASS;
				}
				if(i==5+(boot==HOST_BOOT)+setting->usbmdevs+(setting->usbmdevs==1)*3) {
					strcpy(files[i].name, "vmc:");
					files[i].type = TYPE_DEVICE_VMC;
				}
			}
		} else {
			for(i=0;i<3+setting->usbmdevs+(setting->usbmdevs==1)*3;i++){
				memset(&files[i].createtime, 0, sizeof(PS2TIME));
				memset(&files[i].modifytime, 0, sizeof(PS2TIME));
				files[i].fileSizeByte = 0;
				files[i].attr = MC_ATTR_SUBDIR;
				files[i].title[0] = 0;
				files[i].timestamp = 0;
				files[i].num = i;
				if (i>=3&&i<3+setting->usbmdevs+(setting->usbmdevs==1)*3){
					sprintf(files[i].name, "mass%d:", i-3);
					files[i].type = TYPE_DEVICE_MASS;
				}
			}
			strcpy(files[0].name, "mc0:");
			strcpy(files[1].name, "mc1:");
			strcpy(files[2].name, "mass:");
			files[0].type = TYPE_DEVICE_MC;
			files[1].type = TYPE_DEVICE_MC;
			files[2].type = TYPE_DEVICE_MASS;
		}
		nfiles = i;
		if(cnfmode==ELF_FILE){
			memset(&files[i].createtime, 0, sizeof(PS2TIME));
			memset(&files[i].modifytime, 0, sizeof(PS2TIME));
			files[i].fileSizeByte = 0;
			files[i].attr = MC_ATTR_SUBDIR;
			strcpy(files[i].name, "MISC");
			files[i].type = TYPE_MISC;
			files[i].title[0] = 0;
			files[i].timestamp = 0;
			files[i].num = i;
			nfiles++;
		}
	}
	else if(!strcmp(path, "MISC/")){
		for(i=0;i<127;i++){
			nfiles = i;
			if (dummyElf[i][0] == 0)
				break;
			memset(&files[i].createtime, 0, sizeof(PS2TIME));
			memset(&files[i].modifytime, 0, sizeof(PS2TIME));
			files[i].fileSizeByte = 0;
			if(i==0)
				files[i].attr = MC_ATTR_SUBDIR;
			else
				files[i].attr = 0;
			/*
			if(i==0) strcpy(files[i].name, "..");
			if(i==1) strcpy(files[i].name, "FileBrowser");
			if(i==2) strcpy(files[i].name, "PS2Browser");
			if(i==3) strcpy(files[i].name, "PS2Disc");
			if(i==4) strcpy(files[i].name, "PS2Ftpd");
			if(i==5) strcpy(files[i].name, "DiscStop");
			if(i==6) strcpy(files[i].name, "McFormat");
			if(i==7) strcpy(files[i].name, "PowerOff");
			if(i==8) strcpy(files[i].name, "INFO");
			if(i==9) strcpy(files[i].name, "CONFIG");
			*/
			strcpy(files[i].name, dummyElf[i]);
			files[i].title[0] = 0;
			files[i].num = i;
			files[i].timestamp = 0;
			if(i==0)
				files[i].type = TYPE_OTHER;
			else
				files[i].type = TYPE_MISC;
		}
	}
	else{
		//files[0]を初期化
		memset(&files[0].createtime, 0, sizeof(PS2TIME));
		memset(&files[0].modifytime, 0, sizeof(PS2TIME));
		files[0].fileSizeByte = 0;
		files[0].attr = MC_ATTR_SUBDIR;
		strcpy(files[0].name, "..");
		files[0].title[0] = 0;
		files[0].type=TYPE_OTHER;

		//ファイルリストとファイル数を取得
		nfiles = getDir(path, &files[1]) + 1;
		if(strcmp(ext, "*")){	//ファイルマスク
			for(i=j=1; i<nfiles; i++){
				if(files[i].attr & MC_ATTR_SUBDIR)
					files[j++] = files[i];
				else{
					p = strrchr(files[i].name, '.');
					if(p!=NULL && !stricmp(ext,p+1))
						files[j++] = files[i];
				}
			}
			nfiles = j;
		}

		//ゲームタイトルとファイルタイプを取得
		for(i=1; i<nfiles; i++){
			memset(tmp, 0, 65);
			//ゲームタイトル取得
			if(!strncmp(path, "cdfs", 4)){
				//cdfs
				if(setting->discPs2saveCheck){
					ret = getGameTitle(path, &files[i], tmp);
					if(ret<0) tmp[0]=0;
				}
				else{
					ret=-1;
					tmp[0]=0;
				}
			}
			else if(!strncmp(path, "mc", 2)){
				//mc
				ret = getGameTitle(path, &files[i], tmp);
				if(ret<0) tmp[0]=0;
			}
			else{
				//hddとmass
				if(setting->filePs2saveCheck){
					ret = getGameTitle(path, &files[i], tmp);
					if(ret<0) tmp[0]=0;
				}
				else{
					ret=-1;
					tmp[0]=0;
				}
			}
			//sjisの英数字と記号をASCIIに変換
			memset(files[i].title, 0, 65);
			sjis2ascii(tmp, files[i].title);

			//タイプ取得
			if(files[i].attr & MC_ATTR_SUBDIR){	//フォルダ
				if(ret<0)
					files[i].type=TYPE_DIR;
				else if(ret==0){
					files[i].type=TYPE_PS2SAVE;	//PS2SAVE
				}
				else if(ret==1)
					files[i].type=TYPE_PS1SAVE;	//PS1SAVE
			}
			else if(!(files[i].attr & MC_ATTR_SUBDIR)){	//ファイル
				sprintf(fullpath, "%s%s", path, files[i].name);
				//ELFヘッダを調べる
				if(!strncmp(path, "cdfs", 4)){
					if(setting->discELFCheck){
						checkELFret = checkELFheader(fullpath);
						//mountedParty[0][0]=0;
						if(checkELFret==1)
							files[i].type=TYPE_ELF;
						else
							files[i].type=TYPE_FILE;
					}
					else{
						char *ext;
						ext = getExtension(files[i].name);
						if(ext!=NULL&&!stricmp(ext, ".elf"))
							files[i].type=TYPE_ELF;
						else
							files[i].type=TYPE_FILE;
					}
				}
				else{
					if (setting->fileELFCheck){
						checkELFret = checkELFheader(fullpath);
						//mountedParty[0][0]=0;
						if(checkELFret==1)
							files[i].type=TYPE_ELF;
						else
							files[i].type=TYPE_FILE;
						if(!strncmp(path, "hdd", 3)){
							//HDDのとき再マウント
							mountedParty[0][0]=0;
							getHddParty(path, NULL, party, NULL);
							mountParty(party);
						}
					}
					else{
						char *ext;
						ext = getExtension(files[i].name);
						if(ext!=NULL&&!stricmp(ext, ".elf"))
							files[i].type=TYPE_ELF;
						else
							files[i].type=TYPE_FILE;
					}
				}
				//psuファイルか調べる
				if(files[i].type==TYPE_FILE){
					char *ext;
					ext = getExtension(files[i].name);
					if(ext!=NULL&&!stricmp(ext, ".psu"))
						files[i].type=TYPE_PSU;
				}
			}
		}
		//ソート
		if(nfiles>1)
			sort(&files[1], nfiles-1);
	}

	return nfiles;
}

//-------------------------------------------------
// 任意のファイルパスを返す
static char *imgext[] = {"*", "jpg", "bmp", "gif", "png"};
void getFilePath(char *out, int cnfmode)
{
	char path[MAX_PATH], oldFolder[MAX_PATH],
		msg0[MAX_PATH], msg1[MAX_PATH],
		tmp[MAX_PATH], c[MAX_PATH], ext[8], nextext[8], *p;
	uint64 color;
	FILEINFO files[MAX_ENTRY];
	int nfiles=0, sel=0, top=0, redraw=2;
	int cd=TRUE, up=FALSE, pushed=TRUE;
	int x, y, y0, y1;
	int i, ret;//, fd;
	long size;
//	int code;
	int detail=0;	//詳細表示 0:なし 1:サイズ 2:更新日時
	size_t freeSpace=0;
	int mcfreeSpace=0;
	int vfreeSpace=FALSE;	//空き容量表示フラグ
	int l2button=FALSE, oldl2=FALSE;
	int showdirsize=FALSE;	//フォルダサイズ表示フラグ
	int extval=0;
	
	if (cnfmode == JPG_FILE) {
		strcpy(ext, imgext[extval]);
		strcpy(nextext, imgext[(extval+1)%5]);
	}

	if(cnfmode==ANY_FILE)
		strcpy(ext, "*");
	else if(cnfmode==ELF_FILE)
		strcpy(ext, "elf");
	else if(cnfmode==DIR)
		strcpy(ext, "");
	else if(cnfmode==FNT_FILE)
		strcpy(ext, "fnt");
	else if(cnfmode==IRX_FILE)
		strcpy(ext, "irx");
	else if(cnfmode==JPG_FILE)
		strcpy(ext, "*");
	else if(cnfmode==TXT_FILE)
		strcpy(ext, "txt");
	else if(cnfmode==FMB_FILE)
		strcpy(ext, "elf");

	strcpy(path, LastDir);
	mountedParty[0][0]=0;
	mountedParty[1][0]=0;
	clipPath[0] = 0;
	nclipFiles = 0;
	cut = 0;
	title=FALSE;

	title = setting->defaulttitle;
	detail = setting->defaultdetail;
	sortmode = setting->sort;
#ifdef ENABLE_ICON
	loadIcon();
#endif

	while(1){
		waitPadReady(0, 0);
		if(readpad()){
			l2button=FALSE;
			if(paddata&PAD_L2) l2button=TRUE;
			if((new_pad & (-PAD_L2-1)) || (l2button != oldl2)) {
				redraw = framebuffers;
				oldl2 = l2button;
			}
			if(l2button){
				if(new_pad & PAD_CIRCLE){
					detail++;
					if(detail==4) detail=0;
				}
				else if(new_pad & PAD_TRIANGLE){
					setting->fileicon = !setting->fileicon;
				}
				else if(new_pad & PAD_CROSS){
					flickerfilter = !flickerfilter;
					mkfontcacheset();
				}
				else if(new_pad & PAD_SQUARE){
					if(!strncmp(path, "mc", 2)){
						for(i=1; i<nfiles; i++){
							//フォルダのサイズ取得
							if(files[i].attr & MC_ATTR_SUBDIR){
								int s;
								s=getFileSize(path, &files[i]);
								if(s<0)
									files[i].fileSizeByte=0;
								else
									files[i].fileSizeByte=s;
							}
						}
						showdirsize=TRUE;
						if (sortmode==4 && path[0]!=0 && strcmp(path,"hdd0:/") && strcmp(path,"MISC/")){
							if(nfiles>1){
								sort(&files[1], nfiles-1);
								//sel=0;
								//top=0;
								nmarks = 0;
								memset(marks, 0, MAX_ENTRY);
							}
						}
						detail|=1;	//詳細表示
					}
				}
				else if(new_pad & PAD_R2){
					sortmode++;
					if (sortmode > 5) sortmode = 0;
					if(path[0]!=0 && strcmp(path,"hdd0:/") && strcmp(path,"MISC/")){
						if(nfiles>1){
							sort(&files[1], nfiles-1);
							//sel=0;
							//top=0;
							nmarks = 0;
							memset(marks, 0, MAX_ENTRY);
						}
					}//*/
				}
				else if(new_pad & PAD_UP) {
					sel = 0;
				}
				else if(new_pad & PAD_DOWN) {
					sel = MAX_ENTRY-1;
				}
			}
			else{
				if(new_pad) pushed=TRUE;
				if(new_pad & PAD_UP)
					sel--;
				else if(new_pad & PAD_DOWN)
					sel++;
				else if(new_pad & PAD_LEFT)
					sel-=MAX_ROWS/2;
				else if(new_pad & PAD_RIGHT)
					sel+=MAX_ROWS/2;
				else if(new_pad & PAD_TRIANGLE)
					up=TRUE;
				else if(new_pad & PAD_CIRCLE){	//change dir
					if(files[sel].attr & MC_ATTR_SUBDIR){
						if(!strcmp(files[sel].name,".."))
							up=TRUE;
						else{
							strcat(path, files[sel].name);
							strcat(path, "/");
							cd=TRUE;
						}
					}
				}
				else if(new_pad & PAD_SELECT){	//戻る
					break;
				}
				else if(new_pad & PAD_L1) {	// タイトル表示切り替え
					title = !title;
					//ソート
					/*
					if(path[0]==0 || !strcmp(path,"hdd0:/") || !strcmp(path,"MISC/")){
					}
					else{
						if(nfiles>1){
							sort(&files[1], 0, nfiles-2);
							//sel=0;
							//top=0;
							nmarks = 0;
							memset(marks, 0, MAX_ENTRY);
						}
					}//*/
				}
				else if(new_pad & PAD_R2){	//GETSIZE
					if(path[0]==0 || !strcmp(path,"hdd0:/") || !strcmp(path,"MISC/")){
					}
					else if(nmarks==0 && !strcmp(files[sel].name, "..")){
					}
					else{
						if(nmarks==0){
							drawMsg("SIZE =");
							size=getFileSize(path, &files[sel]);
						}
						else{
							drawMsg(lang->filer_checkingsize);
							for(i=size=0; i<nfiles; i++){
								if(marks[i])
									size+=getFileSize(path, &files[i]);
								if(size<0) size=-1;
							}
						}
						//
						if(size<0){
							strcpy(msg0, lang->filer_getsizefailed);
						}
						else{
							if(size >= 1048576L * 10240)
								sprintf(msg0, "SIZE = %.1f GByte", (double)size/1024/1024/1024);
							else if(size >= 1024*1024)
								sprintf(msg0, "SIZE = %.1f MByte", (double)size/1024/1024);
							else if(size >= 1024)
								sprintf(msg0, "SIZE = %.1f KByte", (double)size/1024);
							else
								sprintf(msg0, "SIZE = %ld Byte", size);
							//mcのとき属性表示
							if(!strncmp(path, "mc", 2) && nmarks==0){
								sprintf(tmp, ", ATTR = %04X", files[sel].attr);
								strcat(msg0, tmp);
							}
							if ((nmarks==0) && !(files[sel].attr & MC_ATTR_SUBDIR) && (setting->getsizecrc32)) {
								strcat(msg0, ", CRC32 = ");
								drawMsg(msg0);
								sprintf(tmp, "%s%s", path, files[sel].name);
								i = CRC32file(tmp);
							//	if (setting->getsizecrc32 == 1)
									sprintf(tmp, "%08X", i);
							//	else
							//		sprintf(tmp, "%08x", i);
								strcat(msg0, tmp);
							}
						}
						pushed = FALSE;
						drawMsg(msg0);
						redraw = 0;
					}
				}
				//ELF_FILE ELF選択時
				if(cnfmode==ELF_FILE||cnfmode==FMB_FILE){
					if(new_pad & PAD_CIRCLE) {	//ファイルを決定
						if(!(files[sel].attr & MC_ATTR_SUBDIR)){
							char fullpath[MAX_PATH];
							int ret;
							sprintf(fullpath, "%s%s", path, files[sel].name);
							if(!strncmp(path, "MISC/", 5)){
								strcpy(out, fullpath);
								strcpy(LastDir, path);
								break;
							}
							ret = checkELFheader(fullpath);
							if(!strncmp(fullpath, "hdd", 3))
								mountedParty[0][0]=0;
							if(ret==1){
								//ELFファイル選択
								strcpy(out, fullpath);
								strcpy(LastDir, path);
								break;
							}
							else{
								//ELFファイルではないとき
								pushed=FALSE;
								sprintf(msg0, lang->filer_not_elf);
#ifdef ENABLE_PSB
								{
									char *extension;
									extension = getExtension(files[sel].name);
									if(extension!=NULL){
										if(!stricmp(extension, ".psb")){
											strcpy(out, fullpath);
											strcpy(LastDir, path);
											break;
										}
									}
								}
#endif
							}
						}
					}
					else if(new_pad & PAD_SQUARE) {	// ファイルマスク切り替え
						if(!strcmp(ext,"*")) strcpy(ext, "elf");
						else				 strcpy(ext, "*");
						cd=TRUE;
					}
					else if(new_pad & PAD_CROSS){	//戻る
						break;
					}
				}
				//FNT_FILE FNT選択時
				else if(cnfmode==FNT_FILE){
					if(new_pad & PAD_CIRCLE) {//FNTファイルを決定
						if(!(files[sel].attr & MC_ATTR_SUBDIR)){
							sprintf(out, "%s%s", path, files[sel].name);
							//ヘッダチェック
							if(checkFONTX2header(out)<0&&checkMSWinheader(out)<0){
								pushed=FALSE;
								sprintf(msg0, lang->filer_not_fnt);
								out[0] = 0;
							}
							else{
								strcpy(LastDir, path);
								break;
							}
						}
					}
					else if(new_pad & PAD_SQUARE) {	// ファイルマスク切り替え
						if(!strcmp(ext,"*")) strcpy(ext, "fnt");
						else				 strcpy(ext, "*");
						cd=TRUE;
					}
					else if(new_pad & PAD_CROSS){	//戻る
						break;
					}
				}
				//JPG_FILE JPG選択時
				else if(cnfmode==JPG_FILE){
					if(new_pad & PAD_CIRCLE) {//イメージファイルを決定
						if(!(files[sel].attr & MC_ATTR_SUBDIR)){
							sprintf(out, "%s%s", path, files[sel].name);
							//ヘッダチェック
							if(!is_image_file(out)){
								pushed=FALSE;
								sprintf(msg0, lang->filer_not_jpg);
								out[0] = 0;
							}
							else{
								strcpy(LastDir, path);
								break;
							}
						}
					}
					else if(new_pad & PAD_SQUARE) {	// ファイルマスク切り替え
						extval = (extval +1) % 5;
						strcpy(ext, imgext[extval]);
						strcpy(nextext, imgext[(extval+1)%5]);
						cd=TRUE;
					}
					else if(new_pad & PAD_CROSS){	//戻る
						break;
					}
				}
				//DIR ディレクトリ選択時
				else if(cnfmode==DIR){
					if(new_pad & PAD_START) {	//ディレクトリを決定
						if( path[0]!=0 && strcmp(path, "hdd0:/")!=0 && strncmp(path, "cdfs", 4)!=0 ){
							strcpy(out, path);
							break;
						}
					}
					else if(new_pad & PAD_CROSS){	//戻る
						break;
					}
				}
				//IRX_FILE IRX選択時
				if(cnfmode==IRX_FILE){
					if(new_pad & PAD_CIRCLE) {	//ファイルを決定
						if(!(files[sel].attr & MC_ATTR_SUBDIR)){
							char fullpath[MAX_PATH];
							sprintf(fullpath, "%s%s", path, files[sel].name);
							if(!strncmp(path, "MISC/", 5)){
								strcpy(out, fullpath);
								strcpy(LastDir, path);
								break;
							}
							//IRXファイル選択
							strcpy(out, fullpath);
							strcpy(LastDir, path);
							break;
						}
					}
					else if(new_pad & PAD_SQUARE) {	// ファイルマスク切り替え
						if(!strcmp(ext,"*")) strcpy(ext, "irx");
						else				 strcpy(ext, "*");
						cd=TRUE;
					}
					else if(new_pad & PAD_CROSS){	//戻る
						break;
					}
				}
				//ANY_FILE	ファイラーモード	すべてのファイルが対象
				else if(cnfmode==ANY_FILE){
					if(new_pad & PAD_CIRCLE) {
						if(!(files[sel].attr & MC_ATTR_SUBDIR)){	//ファイル
							char fullpath[MAX_PATH];
							int ret;
							sprintf(fullpath, "%s%s", path, files[sel].name);
							ret = checkELFheader(fullpath);
							if(!strncmp(fullpath, "hdd", 3))
								mountedParty[0][0]=0;
							if(ret==1){
								//ELFファイル選択
								strcpy(out, fullpath);
								strcpy(LastDir, path);
								break;
							}
							else{
								//ELFファイルではないとき
								pushed=FALSE;
								sprintf(msg0, lang->filer_not_elf);
#ifdef ENABLE_PSB
								{
									char *extension;
									extension = getExtension(fullpath);
									if(extension!=NULL){
										if(!stricmp(extension, ".psb")){	//psbファイルを実行
											int ynret;
											int psbret;
											ynret = MessageBox(lang->filer_execute_psb, LBF_VER, MB_YESNO);
											if(ynret==IDYES){
												psbret = psb(fullpath);
												if(psbret==0){
													pushed=TRUE;
													cd=TRUE;	//空きスペース再計算
												}
												else if(psbret>0){
													sprintf(msg0, "error line no = %d", psbret);
												}
												else if(psbret<0){
													strcpy(msg0, "psb open error");
												}
											}
										}
									}
								}
#endif
							}
						}
					}
					else if(new_pad & PAD_R1){	// メニュー
						drawDark();
						itoGsFinish();
						itoSwitchFrameBuffers();
						drawDark();

						//メニュー
						ret = menu(path, files[sel].name);

						if(ret==COPY || ret==CUT){	// クリップボードにコピー
							strcpy(clipPath, path);
							if(nmarks>0){
								for(i=nclipFiles=0; i<nfiles; i++)
									if(marks[i])
										clipFiles[nclipFiles++]=files[i];
							}
							else{
								clipFiles[0]=files[sel];
								nclipFiles = 1;
							}
							sprintf(msg0, lang->filer_copy_to_clip);
							pushed=FALSE;
							if(ret==CUT)	cut=TRUE;
							else			cut=FALSE;
						}
						else if(ret==DELETE){	// デリート
							drawDarks(0);
							if(nmarks==0){
								if(title && files[sel].title[0])
									sprintf(tmp,"%s",files[sel].title);
								else{
									sprintf(tmp,"%s",files[sel].name);
									if(files[sel].attr & MC_ATTR_SUBDIR)
										strcat(tmp,"/");
								}
								strcat(tmp, "\n");
								strcat(tmp, lang->filer_delete);
							}
							else{
								strcpy(tmp, lang->filer_deletemarkfiles);
							}
							ret = MessageBox(tmp, LBF_VER, MB_YESNO);
							if(ret==IDYES){
								if(nmarks==0){
									strcpy(tmp, files[sel].name);
									if(files[sel].attr & MC_ATTR_SUBDIR) strcat(tmp,"/");
									strcat(tmp, " ");
									strcat(tmp, lang->filer_deleting);
									drawMsg(tmp);
									ret=delete(path, &files[sel]);
								}
								else{
									for(i=0; i<nfiles; i++){
										if(marks[i]){
											strcpy(tmp, files[i].name);
											if(files[i].attr & MC_ATTR_SUBDIR) strcat(tmp,"/");
											strcat(tmp, " ");
											strcat(tmp, lang->filer_deleting);
											drawMsg(tmp);
											ret=delete(path, &files[i]);
											if(ret<0) break;
										}
									}
								}
								if(ret>=0){
									cd=TRUE;	//空きスペース再計算
								}
								else{
									strcpy(msg0, lang->filer_deletefailed);
									pushed = FALSE;
								}
							}
						}
						else if(ret==RENAME){	// リネーム
							drawDark();
							itoGsFinish();
							itoSwitchFrameBuffers();
							drawDark();
							strcpy(tmp, files[sel].name);
							if(keyboard(SKBD_FILE, tmp, 36)>=0){
								if(Rename(path, &files[sel], tmp)<0){
									pushed=FALSE;
									strcpy(msg0, lang->filer_renamefailed);
								}
								else
									cd=TRUE;
							}
						}
						else if(ret==PASTE){	// クリップボードからペースト
							drawMsg(lang->filer_pasting);
							ret=paste(path);
							if(ret < 0){
								strcpy(msg0, lang->filer_pastefailed);
								pushed = FALSE;
							}
							else{
								if(cut) nclipFiles=0;
							}
							cd=TRUE;
						}
						else if(ret==NEWDIR){	// 新規フォルダ作成
							tmp[0]=0;
							drawDark();
							itoGsFinish();
							itoSwitchFrameBuffers();
							drawDark();
							if(keyboard(SKBD_FILE, tmp, 36)>=0){
								ret = newdir(path, tmp);
								if(ret == -17){
									strcpy(msg0, lang->filer_direxists);
									pushed=FALSE;
								}
								else if(ret < 0){
									strcpy(msg0, lang->filer_newdirfailed);
									pushed=FALSE;
								}
								else{
									strcat(path, tmp);
									strcat(path, "/");
									cd=TRUE;
								}
							}
						}
						else if(ret==GETSIZE){	// サイズ表示
							if(nmarks==0){
								drawMsg("SIZE =");
								size=getFileSize(path, &files[sel]);
							}
							else{
								drawMsg(lang->filer_checkingsize);
								for(i=size=0; i<nfiles; i++){
									if(marks[i])
										size+=getFileSize(path, &files[i]);
									if(size<0) size=-1;
								}
							}
							//
							if(size<0){
								strcpy(msg0, lang->filer_getsizefailed);
							}
							else{
								if(size > 1048576L * 10240) 
									sprintf(msg0, "SIZE = %.1f GByte", (double)size/1024/1024/1024);
								if(size >= 1024*1024)
									sprintf(msg0, "SIZE = %.1f MByte", (double)size/1024/1024);
								else if(size >= 1024)
									sprintf(msg0, "SIZE = %.1f KByte", (double)size/1024);
								else
									sprintf(msg0, "SIZE = %ld Byte", size);
								//mcのとき属性表示
								if(!strncmp(path, "mc", 2) && nmarks==0){
									sprintf(tmp, ", ATTR = %04X", files[sel].attr);
									strcat(msg0, tmp);
								}
								if ((nmarks==0) && !(files[sel].attr & MC_ATTR_SUBDIR) && (setting->getsizecrc32)) {
									strcat(msg0, ", CRC32 = ");
									drawMsg(msg0);
									sprintf(tmp, "%s%s", path, files[sel].name);
									i = CRC32file(tmp);
								//	if (setting->getsizecrc32 == 1)
								//		sprintf(tmp, "%08X", i);
								//	else
										sprintf(tmp, "%08x", i);
									strcat(msg0, tmp);
								}
							}
							pushed = FALSE;
						}
						else if(ret==EXPORT){	// psuファイルにエクスポート
							int sjisout = FALSE;
							if (framebuffers>1) {
								drawDark();
								itoGsFinish();
								itoSwitchFrameBuffers();
							}
							drawDark();

							if(nmarks==0){
								ret = MessageBox(lang->filer_export, LBF_VER, MB_YESNO|MB_USETRIANGLE);
							}
							else{
								ret = MessageBox(lang->filer_exportmarkfiles, LBF_VER, MB_YESNO|MB_USETRIANGLE);
							}
							if(ret==IDYES||ret==(IDYES|IDTRIANGLE)){//○か△でYESを選択したとき
								if(ret&IDTRIANGLE) sjisout = TRUE;	//△でYESを選択したときファイル名をsjisで出力する
								ret=0;
								if(nmarks==0){
									ret = psuExport(path, &files[sel], sjisout);
								}
								else{
									for(i=0; i<nfiles; i++){
										if(marks[i]){
											if(files[i].attr & MC_ATTR_SUBDIR){	//フォルダのとき
												ret = psuExport(path, &files[i], sjisout);
												if(ret<0) break;	//中断する
											}
										}
									}
								}
								//リザルト
								if(ret<0){
									sprintf(msg0, "%s %d", lang->filer_exportfailed, ret);
									pushed = FALSE;
								}
								else{
									if(setting->Exportdir[0])
										strcpy(tmp,setting->Exportdir);
									else
										strcpy(tmp,path);
									sprintf(msg0, "%s %s", lang->filer_exportto, tmp);
									pushed = FALSE;
									cd = TRUE;
								}
							}
						}
						else if(ret==IMPORT){	// psuファイルからインポート
							if (framebuffers>1) {
								drawDark();
								itoGsFinish();
								itoSwitchFrameBuffers();
							}
							drawDark();
	
							if(nmarks==0){
								ret = MessageBox(lang->filer_import, LBF_VER, MB_MC0MC1CANCEL);
							}
							else{
								ret = MessageBox(lang->filer_importmarkfiles, LBF_VER, MB_MC0MC1CANCEL);
							}
							if(ret==IDMC0||ret==IDMC1){
								int outmc=0;
								if(ret==IDMC0) outmc=0;
								if(ret==IDMC1) outmc=1;
								if(nmarks==0){
									ret = psuImport(path, &files[sel], outmc);
								}
								else{
									for(i=0; i<nfiles; i++){
										if(marks[i]){
											if(!(files[i].attr & MC_ATTR_SUBDIR)){	//ファイルのとき
												ret = psuImport(path, &files[i], outmc);
												if((ret!=-90) && (ret<0)) break;	//中断する
											}
										}
									}
								}
								//リザルト
								if(ret<0){
									sprintf(msg0, "%s %d", lang->filer_importfailed, ret);
									pushed = FALSE;
								}
								else{
									if(outmc==0) strcpy(tmp,"mc0:/");
									if(outmc==1) strcpy(tmp,"mc1:/");
									sprintf(msg0, "%s %s", lang->filer_importto, tmp);
									pushed = FALSE;
									cd = TRUE;
								}
							}
						}
	/*
						else if(ret==COMPRESS){	// 圧縮
							char inpath[MAX_PATH];
							char outpath[MAX_PATH];
							
								if(nmarks==0){
									strcpy(inpath, path);
									strcpy(outpath, path);
									strcat(inpath, files[sel].name);
									strcat(outpath, files[sel].name);
									strcat(outpath, ".tk2");
									ret = winmain(2, inpath, outpath);
								}
								else{
									for(i=0; i<nfiles; i++){
										if(marks[i]){
											if(!(files[i].attr & MC_ATTR_SUBDIR)){	//フォルダのとき
												strcpy(inpath, path);
												strcpy(inpath, files[i].name);
												strcpy(outpath, path);
												strcpy(outpath, files[i].name);
												strcpy(outpath, ".tk2");
												ret = winmain(2, inpath, outpath);
												if(ret<0) break;	//中断する
											}
										}
									}
								}
								cd = TRUE;
						}
	*/
						else if(ret==VIEWER) {
							char fullpath[MAX_PATH];
							
							strcpy(fullpath, path);
							ret = -1;
							if(nmarks==0){
								// カーソル位置のファイル
								if((files[sel].type != TYPE_DIR) && (files[sel].type != TYPE_PS1SAVE) && (files[sel].type != TYPE_PS2SAVE)){
									strcat(fullpath, files[sel].name);
									ret = viewer_file(0, fullpath);
								}
							}
							else if (nmarks == 1) {
								// 最初のファイル
								for(i=0; i<nfiles; i++){
									if(marks[i]){
										if((files[i].type != TYPE_DIR) && (files[i].type != TYPE_PS1SAVE) && (files[i].type != TYPE_PS2SAVE)){
											strcat(fullpath, files[i].name);
											ret = viewer_file(0, fullpath);
											break;
										}
									}
								}
							} else {
								// 複数のファイル
								int k,o,r,l;
								for (i=1,k=0,l=0; i<nfiles; i++) {
									if (marks[i]) {
										strcpy(fullpath, path);
										strcat(fullpath, files[i].name);
										//if (((m=formatcheckfile(fullpath)) == FT_BMP) || (m == FT_JPG) || (m == FT_GIF) || (m == FT_PNG) || (m == FT_P2T) || (m == FT_PS1)) {
										if (is_image_file(fullpath)) {
											k = i;
											break;
										}
									}
								}
								i = k; o = k^1; r = 2; l = ret;
								while(i > 0) {
									if (i != o) {
										strcpy(fullpath, path);
										strcat(fullpath, files[i].name);
										l = ret = viewer_file(2, fullpath);
										printf("filer: viewer_return: %d\n", ret);
										o = i;
									}
									if (ret == 0) {
										break;
									} else if (ret < 0) {
										ret = r;
									} else {
										r = ret;
									}
									if (ret == 1) {
										for (k=i-1; k>0; k--) {
											if (marks[k]) {
												strcpy(fullpath, path);
												strcat(fullpath, files[k].name);
												//if (((m=formatcheckfile(fullpath)) == FT_BMP) || (m == FT_JPG) || (m == FT_GIF) || (m == FT_PNG) || (m == FT_P2T) || (m == FT_PS1)) {
												if (is_image_file(fullpath)) {
													i = k;
													break;
												}
											}
										}
										if (o == i) break;
									} else if (ret == 2) {
										for (k=i+1; k<nfiles; k++) {
											if (marks[k]) {
												strcpy(fullpath, path);
												strcat(fullpath, files[k].name);
												//if (((m=formatcheckfile(fullpath)) == FT_BMP) || (m == FT_JPG) || (m == FT_GIF) || (m == FT_PNG) || (m == FT_P2T) || (m == FT_PS1)) {
												if (is_image_file(fullpath)) {
													i = k;
													break;
												}
											}
										}
										if (o == i) break;
									}
								}
								ret = l;
							}
							switch(ret) {
								case -1: {strcpy(msg0, lang->editor_viewer_error1); break;}
								case -2: {strcpy(msg0, lang->editor_viewer_error2); break;}
							}
							cd = FALSE;
							pushed = FALSE;
							if (paddata & PAD_SELECT)
								break;
						}
					}
					else if(new_pad & PAD_CROSS) {	// マーク
						if(sel!=0 && path[0]!=0 && strcmp(path,"hdd0:/")){
							if(marks[sel]){
								marks[sel]=FALSE;
								nmarks--;
							}
							else{
								marks[sel]=TRUE;
								nmarks++;
							}
						}
						sel++;
					}
					else if(new_pad & PAD_SQUARE) {	// マーク反転
						if(path[0]!=0 && strcmp(path,"hdd0:/")){
							for(i=1; i<nfiles; i++){
								if(marks[i]){
									marks[i]=FALSE;
									nmarks--;
								}
								else{
									marks[i]=TRUE;
									nmarks++;
								}
							}
						}
					}
				}
			}
		}
		// 上位フォルダ移動
		if(up){
			if((p=strrchr(path, '/'))!=NULL)
				*p = 0;
			if((p=strrchr(path, '/'))!=NULL){
				p++;
				strcpy(oldFolder, p);
				*p = 0;
			}
			else{
				strcpy(oldFolder, path);
				path[0] = 0;
			}
			cd=TRUE;
		}
		//ファイルリストを取得
		if(cd){
			nfiles = setFileList(path, ext, files, cnfmode);
			// 空き容量取得
			vfreeSpace=FALSE;	//空き容量表示フラグ
			if(cnfmode==ANY_FILE){
				if(!strncmp(path, "mc", 2)){
					int type;
					mcGetInfo(path[2]-'0', 0, &type, &mcfreeSpace, NULL);
					mcSync(MC_WAIT, NULL, NULL);
					if(type==MC_TYPE_PS2)	//ps2 mc
						freeSpace = mcfreeSpace*1024;
					else if(type==MC_TYPE_PSX||type==MC_TYPE_POCKET)	//ps1 mc
						freeSpace = mcfreeSpace*8192;
					else
						freeSpace = 0;
					vfreeSpace=TRUE;
				}
				else if(!strncmp(path,"hdd",3)&&strcmp(path,"hdd0:/")){
					freeSpace = fileXioDevctl("pfs0:",PFSCTL_GET_ZONE_FREE,NULL,0,NULL,0)*fileXioDevctl("pfs0:",PFSCTL_GET_ZONE_SIZE,NULL,0,NULL,0);
					vfreeSpace=TRUE;
				}
			}
			// 変数初期化
			sel=0;
			top=0;
			if(up){
				for(i=0; i<nfiles; i++){
					if(!strcmp(oldFolder, files[i].name)){
						sel=i;
						top=sel-3;
						break;
					}
				}
			}
			nmarks = 0;
			memset(marks, 0, MAX_ENTRY);
			cd=FALSE;
			up=FALSE;
			showdirsize=FALSE;
		}

		//disc stop
		if(strncmp(path,"cdfs",4) && setting->discControl)
			CDVD_Stop();

		if (redraw) {
			int fl,fr;	// ファイル名描画範囲
			// ファイルリスト表示用変数の正規化
			if(top > nfiles-MAX_ROWS)	top=nfiles-MAX_ROWS;
			if(top < 0)			top=0;
			if(sel >= nfiles)		sel=nfiles-1;
			if(sel < 0)			sel=0;
			if(sel >= top+MAX_ROWS)	top=sel-MAX_ROWS+1;
			if(sel < top)			top=sel;

			// 画面描画開始
			clrScr(setting->color[COLOR_BACKGROUND]);
			// ファイルリスト
			x = FONT_WIDTH*3;
			y = SCREEN_MARGIN+FONT_HEIGHT*3;
			for(i=0; i<MAX_ROWS; i++){
				if(top+i >= nfiles)
					break;
				//色とカーソル表示
				if(top+i == sel){
					color = setting->color[COLOR_HIGHLIGHTTEXT];
					printXY(">", x, y, color, TRUE);
				}
				else
					color = setting->color[COLOR_TEXT];
				//マーク表示
				if(marks[top+i]){
					printXY("*", x+FONT_WIDTH, y, setting->color[COLOR_TEXT], TRUE);
				}
				//ファイルリスト表示
				if(title){
					if(files[top+i].title[0]!=0)
						strcpy(tmp,files[top+i].title);	//ゲームタイトル
					else
						strcpy(tmp,files[top+i].name);	//ファイル名
				}
				else
					strcpy(tmp,files[top+i].name);	//ファイル名

				//フォルダのときスラッシュつける
				if((files[top+i].attr & MC_ATTR_SUBDIR)&&(strcmp(files[top+i].name,"..")))
					strcat(tmp,"/");

				//ファイル名が長いときは、短くする
			//	if(strlen(tmp)>MAX_ROWS_X&&MAX_ROWS_X>3){
			//		tmp[MAX_ROWS_X-3]=0;
			//		code=tmp[MAX_ROWS_X-4];
			//		if( (code>=0x81)&&(code<=0x9F) ) tmp[MAX_ROWS_X-4] = 0;
			//		if( (code>=0xE0)&&(code<=0xFC) ) tmp[MAX_ROWS_X-4] = 0;
			//		strcat(tmp, "...");
			//	}

				//ファイル名を表示
				if(setting->fileicon){
#ifdef ENABLE_ICON
					if(files[top+i].type!=TYPE_OTHER){
						drawIcon(
							x+FONT_WIDTH*2, y,
							FONT_WIDTH*2, FONT_HEIGHT - GetFontMargin(LINE_MARGIN),
							files[top+i].type);
					}
#else
					if(files[top+i].type>=TYPE_FILE && files[top+i].type<TYPE_OTHER){
						uint64 iconcolor=0;
						//アイコンの色
						if(files[top+i].type==TYPE_FILE)
							iconcolor=setting->color[COLOR_FILE];
						else if(files[top+i].type==TYPE_ELF)
							iconcolor=setting->color[COLOR_ELF];
						else if(files[top+i].type==TYPE_DIR)
							iconcolor=setting->color[COLOR_DIR];
						else if(files[top+i].type==TYPE_PS2SAVE)
							iconcolor=setting->color[COLOR_PS2SAVE];
						else if(files[top+i].type==TYPE_PS1SAVE)
							iconcolor=setting->color[COLOR_PS1SAVE];
						else if(files[top+i].type==TYPE_PSU)
							iconcolor=setting->color[COLOR_PSU];
						//アイコンを表示
						itoSprite(iconcolor,
							x+FONT_WIDTH*2, y,
							x+FONT_WIDTH*2+FONT_WIDTH, y+(FONT_HEIGHT - GetFontMargin(LINE_MARGIN)), 0);
					}
#endif
					//ファイル名表示
				//	printXY(tmp, x+FONT_WIDTH*4, y, color, TRUE);
					fl = x+FONT_WIDTH*4; 
				}
				else{
					//ファイル名のみ表示
				//	printXY(tmp, x+FONT_WIDTH*2, y, color, TRUE);
					fl = x+FONT_WIDTH*2;
				}
				
				int drawStringWindow(const unsigned char *s, int charset, int sx, int sy, uint64 col, int sl, int sr);
				fr = (MAX_ROWS_X+7-9*(detail & 1)-10*(detail & 2)) * FONT_WIDTH;
				drawStringWindow(tmp, TXT_SJIS, fl, y, color, fl, fr);

				//詳細表示
				if(path[0]==0 || !strcmp(path,"hdd0:/") || !strcmp(path,"MISC/")){
					//何もしない
				}
				else{
					int len = 0;
					tmp[0] = 0;
					if(detail & 1){	//ファイルサイズ表示
						if(files[top+i].attr & MC_ATTR_SUBDIR && showdirsize==FALSE){
							strcpy(c,"  <DIR> ");
						}
						else{
							if(files[top+i].fileSizeByte >= 1024*1024)
								sprintf(c, "%4.1fMB", (float)files[top+i].fileSizeByte/1024/1024);
							else if(files[top+i].fileSizeByte >= 1024)
								sprintf(c, "%4.1fKB", (float)files[top+i].fileSizeByte/1024);
							else
								sprintf(c,"%6ld B",files[top+i].fileSizeByte);
						}
						len+= 9;
						strcpy(tmp, c);
					}
					if(detail & 2){	//更新日時表示
						//cdfsは、更新日時を取得できない
						if(!strncmp(path,"cdfs",4)){
							strcpy(c,"----/--/-- --:--:--");
						}
						else{
							sprintf(c,"%04d/%02d/%02d %02d:%02d:%02d",
								files[top+i].modifytime.year,
								files[top+i].modifytime.month,
								files[top+i].modifytime.day,
								files[top+i].modifytime.hour,
								files[top+i].modifytime.min,
								files[top+i].modifytime.sec);
						}
						if (tmp[0] != 0) strcat(tmp, " ");
						strcat(tmp, c);
						len+= 20;
					}
					if (len > 0) {
						if(strcmp(files[top+i].name,"..")){
						//	itoSprite(setting->color[COLOR_BACKGROUND],
						//		(MAX_ROWS_X+7-len)*FONT_WIDTH, y,
						//		(MAX_ROWS_X+8)*FONT_WIDTH, y+FONT_HEIGHT, 0);
							itoLine(setting->color[COLOR_FRAME], (MAX_ROWS_X+7.5-len)*FONT_WIDTH, y, 0,
								setting->color[COLOR_FRAME], (MAX_ROWS_X+7.5-len)*FONT_WIDTH, y+FONT_HEIGHT, 0);	
							printXY(tmp, FONT_WIDTH*(MAX_ROWS_X+7-strlen(tmp)), y, color, TRUE);
						}
					}
				}
				y += FONT_HEIGHT;
			}
			// スクロールバー
			if(nfiles > MAX_ROWS){
				drawFrame((MAX_ROWS_X+8)*FONT_WIDTH, SCREEN_MARGIN+FONT_HEIGHT*3,
					(MAX_ROWS_X+9)*FONT_WIDTH, SCREEN_MARGIN+FONT_HEIGHT*(MAX_ROWS+3),setting->color[COLOR_FRAME]);
				y0=FONT_HEIGHT*MAX_ROWS*((double)top/nfiles);
				y1=FONT_HEIGHT*MAX_ROWS*((double)(top+MAX_ROWS)/nfiles);
				itoSprite(setting->color[COLOR_FRAME],
					(MAX_ROWS_X+8)*FONT_WIDTH,
					SCREEN_MARGIN+FONT_HEIGHT*3+y0,
					(MAX_ROWS_X+9)*FONT_WIDTH,
					SCREEN_MARGIN+FONT_HEIGHT*3+y1,
					0);
			}
			//
			if(l2button){
				//
				int dialog_x;		//ダイアログx位置
				int dialog_y;		//ダイアログy位置
				int dialog_width;	//ダイアログ幅
				int dialog_height;	//ダイアログ高さ

				dialog_width = FONT_WIDTH*28;
				dialog_height = FONT_HEIGHT*6;
				dialog_x = (SCREEN_WIDTH-dialog_width)/2;
				dialog_y = (SCREEN_HEIGHT-dialog_height)/2;
				// 描画開始
				drawDark();
				drawDialogTmp(dialog_x, dialog_y,
					dialog_x+dialog_width, dialog_y+dialog_height,
					setting->color[COLOR_BACKGROUND], setting->color[COLOR_FRAME]);
				//
				x = dialog_x+FONT_WIDTH*1;
				y = dialog_y+FONT_HEIGHT*0.5;
				//
				sprintf(tmp, "○:%s", lang->filer_l2popup_detail);
				printXY(tmp, x, y, setting->color[COLOR_TEXT], TRUE); y+=FONT_HEIGHT;
				sprintf(tmp, "△:%s", lang->filer_l2popup_icon);
				printXY(tmp, x, y, setting->color[COLOR_TEXT], TRUE); y+=FONT_HEIGHT;
				sprintf(tmp, "×:%s", lang->filer_l2popup_flicker);
				printXY(tmp, x, y, setting->color[COLOR_TEXT], TRUE); y+=FONT_HEIGHT;
				sprintf(tmp, "□:%s", lang->filer_l2popup_dirsize);
				if(!strncmp(path, "mc", 2)){
					printXY(tmp, x, y, setting->color[COLOR_TEXT], TRUE);
				}
				else{
					printXY(tmp, x, y, setting->color[COLOR_GRAYTEXT], TRUE);
				} y+=FONT_HEIGHT;
 				sprintf(tmp, "R2:%s [%s]", lang->filer_l2popup_sort, lang->conf_sort_types[sortmode]);
				printXY(tmp, x, y, setting->color[COLOR_TEXT], TRUE); y+=FONT_HEIGHT;
			}
			// メッセージ
			if(pushed) sprintf(msg0, "Path: %s", path);
			// 操作説明
			if(l2button){
				//sprintf(msg1, "");
				msg1[0]='\0';
			}
			else{
				if(cnfmode==ANY_FILE){
					if(title)
						sprintf(msg1, lang->filer_anyfile_hint1);
					else
						sprintf(msg1, lang->filer_anyfile_hint2);
				}
				else if(cnfmode==ELF_FILE||cnfmode==FMB_FILE){
					if(!strcmp(ext, "*"))
						sprintf(msg1, lang->filer_elffile_hint1);
					else
						sprintf(msg1, lang->filer_elffile_hint2);
				}
				else if(cnfmode==JPG_FILE){
					sprintf(msg1, lang->filer_jpgfile_hint, ext, nextext);
				}
				else if(cnfmode==FNT_FILE){
					if(!strcmp(ext, "*"))
						sprintf(msg1, lang->filer_fntfile_hint1);
					else
						sprintf(msg1, lang->filer_fntfile_hint2);
				}
				else if(cnfmode==IRX_FILE){
					if(!strcmp(ext, "*"))
						sprintf(msg1, lang->filer_irxfile_hint1);
					else
						sprintf(msg1, lang->filer_irxfile_hint2);
				}
				else if(cnfmode==DIR){
					sprintf(msg1, lang->filer_dir_hint);
				}
			}
			setScrTmp(msg0, msg1);
			// ボリュームシリアル番号の表示
			//if ((path[0] == 0x6d) && (path[1] == 0x63)) {
			//	sprintf(tmp, "%04X-%04X", volumeserialnumber & 0xFFFFu, volumeserialnumber >> 16);
			//	printXY(tmp, (MAX_ROWS_X+10-strlen(tmp))*FONT_WIDTH, SCREEN_MARGIN+FONT_HEIGHT, setting->color[COLOR_TEXT], TRUE);
			//}

			// フリースペース表示
			if(vfreeSpace){
				if(freeSpace >= 1024*1024)
					sprintf(tmp, "%.1fMB free", (double)freeSpace/1024/1024);
				else if(freeSpace >= 1024)
					sprintf(tmp, "%.1fKB free", (double)freeSpace/1024);
				else
					sprintf(tmp, "%dB free", freeSpace);
				ret=strlen(tmp);
				//
				printXY(tmp,
					(MAX_ROWS_X+10-ret)*FONT_WIDTH, SCREEN_MARGIN,
					setting->color[COLOR_TEXT], TRUE);
			}
			drawScr();
			redraw--;
		} else {
			itoVSync();
		}
	}
	
	if(mountedParty[0][0]!=0){
		fileXioUmount("pfs0:");
		mountedParty[0][0]=0;
	}
	if(mountedParty[1][0]!=0){
		fileXioUmount("pfs1:");
		mountedParty[1][0]=0;
	}
	if (vmcmount) {
		fileXioUmount("vmc:");
		vmcmount = FALSE;
	}
	return;
}
