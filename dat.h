typedef struct Album Album;
typedef struct AlbumImg AlbumImg;
typedef struct YearIdx YearIdx;
typedef struct ImgDB ImgDB;
typedef struct TagIdx TagIdx;
typedef struct TagPvt TagPvt;
typedef struct TagDB TagDB;

struct Album
{
	YearIdx *up;
	int month;
	struct
	{
		AlbumImg *head;
		AlbumImg *tail;
	} images;
	AlbumImg *hover;
};

struct AlbumImg
{
	Album *up;
	AlbumImg *next;
	AlbumImg *prev;
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
