ImgDB *openimgdb(char*);
void freeimgdb(ImgDB*);

YearIdx *openyearidx(char*);
void freeyearidx(YearIdx*);

Album *openalbum(char*);
void freealbum(Album*);

Img *loadimg(char*);
void freeimg(Img*);

void freeimglist(ImgList*);
