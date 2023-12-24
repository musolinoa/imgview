typedef struct Album Album;
typedef struct Img Img;
typedef struct ImgList ImgList;
typedef struct YearIdx YearIdx;
typedef struct ImgDB ImgDB;
typedef struct TagIdx TagIdx;
typedef struct TagPvt TagPvt;
typedef struct TagDB TagDB;

struct ImgList
{
	Img *head;
	Img *tail;
};

struct Album
{
	YearIdx *up;
	int month;
	char *path;
	ImgList *images;
	Img *hover;
};

struct Img
{
	Album *up;
	Img *next;
	Img *prev;
	char *name;
	Image *thumb;
	Rectangle box;
	char selected;
};

struct YearIdx
{
	ImgDB *up;
	YearIdx *next;
	YearIdx *prev;
	char *name;
	Album *months[12];
};

struct TagIdx
{
	char *key;
	int idx;
};

struct TagPvt
{
	int tag;
	int img;
};

struct TagDB
{
	TagIdx *tags;
	TagIdx *imgs;
	TagPvt *pivots;
};

struct ImgDB
{
	YearIdx *years;
	TagDB *tags;
};
