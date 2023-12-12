typedef struct Album Album;
typedef struct AlbumImg AlbumImg;

struct Album
{
	AlbumImg *images;
	AlbumImg *hover;
};

struct AlbumImg
{
	Album *album;
	AlbumImg *next;
	AlbumImg *prev;
	char *name;
	Image *thumb;
	Rectangle box;
	char selected;
};
