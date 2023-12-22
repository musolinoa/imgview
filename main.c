#include <u.h>
#include <libc.h>
#include <draw.h>
#include <thread.h>
#include <keyboard.h>
#include <mouse.h>
#include <ctype.h>
#include "dat.h"
#include "fns.h"

static int
endswith(char *s, char *suf)
{
	long slen, suflen;

	slen = strlen(s);
	suflen = strlen(suf);
	if(slen < suflen)
		return 0;
	return strcmp(s + slen - suflen, suf) == 0;
}

static char*
trimsuffix(char *s, char *suf)
{
	char *p, *q, *r;

	p = s + strlen(s);
	q = suf + strlen(suf);
	for(;;){
		if(p == s)
			return strdup("");
		if(q == suf){
			r = mallocz(p - s + 1, 1);
			return strncpy(r, s, p - s);
		}
		p--;
		q--;
		if(*p != *q)
			return strdup(s);
		p--;
		q--;
	}
}

static int
isyear(char *s)
{
	int i;

	for(i = 0; i < 4; i++){
		if(!isdigit(*s++))
			return 0;
	}
	return *s == 0;
}

static int
month(char *s)
{
	switch(*s++){
	case '0':
		if('1' <= *s && *s <= '9')
			return *s - '0' - 1;
		break;
	case '1':
		if('0' <= *s && *s <= '2')
			return *s - '0' + 9;
		break;
	}
	return -1;
}

static int
dircmp(void *va, void *vb)
{
	Dir *da, *db;

	da = va;
	db = vb;
	return strcmp(da->name, db->name);
}

static int
rdircmp(void *va, void *vb)
{
	return -dircmp(va, vb);
}

static int
dirforeach(char *path, int (*fn)(char *path, Dir*, void*), void *aux)
{
	int fd;
	Dir *entries, *e;
	long n;

	fd = open(path, OREAD);
	if(fd < 0)
		return -1;
	n = dirreadall(fd, &entries);
	close(fd);
	qsort(entries, n, sizeof(*entries), dircmp);
	e = entries;
	while(n-- > 0){
		if(fn(path, e, aux) < 0)
			goto Error;
		e++;
	}
	return 0;
Error:
	free(entries);
	return -1;
}

static int
openimgdb1(char *path, Dir *e, void *a)
{
	ImgDB *db;
	YearIdx *year;
	char buf[1024];

	db = a;
	if((e->mode&DMDIR) == 0)
		return 0;
	if(!isyear(e->name))
		return 0;
	snprint(buf, sizeof(buf), "%s/%s", path, e->name);
	year = openyearidx(buf);
	if(year == nil)
		return -1;
	if(db->years != nil)
		db->years->next = year;
	year->prev = db->years;
	db->years = year;
	return 1;
}

ImgDB*
openimgdb(char *path)
{
	ImgDB *db;

	if((db = mallocz(sizeof(ImgDB), 1)) == nil)
		return nil;
	if(dirforeach(path, openimgdb1, db) < 0){
		freeimgdb(db);
		return nil;
	}
	return db;
}

void
freeimgdb(ImgDB *db)
{
	YearIdx *y, *tmp;

	if(db == nil)
		return;
	y = db->years;
	while(y != nil){
		tmp = y;
		y = y->next;
		freeyearidx(tmp);
	}
	free(db);
}

static int
openyearidx1(char *path, Dir *e, void *a)
{
	int m;
	YearIdx *year;
	Album *album;
	char buf[1024];

	year = a;
	if((e->mode&DMDIR) == 0)
		return 0;
	if((m = month(e->name)) < 0)
		return 0;
	snprint(buf, sizeof(buf), "%s/%s", path, e->name);
	album = openalbum(buf);
	if(album == nil)
		return -1;
	album->up = year;
	album->month = m;
	year->months[m] = album;
	return 1;
}

YearIdx*
openyearidx(char *path)
{
	YearIdx *year;

	if((year = mallocz(sizeof(YearIdx), 1)) != nil)
	if((year->name = strdup(path)) != nil)
	if(dirforeach(path, openyearidx1, year) >= 0)
		return year;
	freeyearidx(year);
	return nil;
}

void
freeyearidx(YearIdx *year)
{
	int i;

	if(year == nil)
		return;
	for(i = 0; i < 12; i++)
		freealbum(year->months[i]);
	free(year);
}

static int
openalbum1(char *path, Dir *e, void *a)
{
	Album *album;
	AlbumImg *img;
	char buf[1024];

	album = a;
	if((e->mode&DMDIR) != 0)
		return 0;
	if(!endswith(e->name, ".thumb.1"))
		return 0;
	snprint(buf, sizeof(buf), "%s/%s", path, e->name);
	img = loadalbumimg(buf);
	if(img == nil)
		return -1;
	img->up = album;
	if(album->images.head == nil){
		album->images.head = img;
	}else{
		album->images.tail->next = img;
		img->prev = album->images.tail;
	}
	album->images.tail = img;
	return 1;
}

Album*
openalbum(char *path)
{
	Album *album;

	album = mallocz(sizeof(Album), 1);
	if(album == nil)
		return nil;
	if(dirforeach(path, openalbum1, album) < 0){
		freealbum(album);
		return nil;
	}
	return album;
}

Image *
load9img(char *path)
{
	int fd;
	Image *img;

	fd = open(path, OREAD);
	if(fd < 0)
		return nil;
	img = readimage(display, fd, 0);
	close(fd);
	return img;
}

AlbumImg *
loadalbumimg(char *path)
{
	AlbumImg *aimg;
	Image *img;

	img = load9img(path);
	if(img == nil)
		goto Error;
	aimg = mallocz(sizeof(AlbumImg), 1);
	if(aimg == nil)
		goto Error;
	aimg->name = trimsuffix(path, ".thumb.1");
	aimg->thumb = img;
	return aimg;
Error:
	free(img);
	return nil;
}

void
freealbumimg(AlbumImg *img)
{
	if(img == nil)
		return;
	free(img->name);
	free(img->thumb);
	free(img);
}

void
freealbum(Album *album)
{
	AlbumImg *img, *tmp;

	if(album == nil)
		return;
	img = album->images.head;
	while(img != nil){
		tmp = img;
		img = img->next;
		free(tmp);
	}
	free(album);
}

enum{
	Spacing = 4,
};

static void
reflow(Album *a, Rectangle bbox)
{
	int w, h;
	AlbumImg *i;
	Rectangle r;

	r = bbox;
	for(i = a->images.head; i != nil; i = i->next){
		if(i->thumb == nil)
			continue;
		w = Dx(i->thumb->r);
		h = Dy(i->thumb->r);
		if(r.min.x + w + Spacing > bbox.max.x){
			r.min.x = bbox.min.x;
			r.min.y += h + Spacing;
		}
		r.max.x = r.min.x + w;
		r.max.y = r.min.y + h;
		if(r.max.y + Spacing > bbox.max.y)
			break;
		i->box = r;
		r.min.x += w + Spacing;
	}
}

static ImgDB *db;
static Album *album;

void
drawthumb(AlbumImg *i)
{
	static Image *fill;
	Rectangle r;

	if(fill == nil)
		fill = allocimage(display, r, screen->chan, 1, 0x229900ff);
	r = insetrect(i->box, 2);
	r.min.x = r.max.x - 16;
	r.max.y = r.min.y + 16;
	draw(screen, i->box, i->thumb, nil, ZP);
	if(i->selected != 0){
		draw(screen, r, display->black, nil, ZP);
		draw(screen, insetrect(r, 1), display->white, nil, ZP);
		draw(screen, insetrect(r, 3), fill, nil, ZP);
	}else if(i->up->hover == i){
		draw(screen, r, display->black, nil, ZP);
		draw(screen, insetrect(r, 1), display->white, nil, ZP);
	}
}

void
resized(int new)
{
	Rectangle box;
	AlbumImg *img;
	char buf[1024];

	if(new && getwindow(display, Refnone) < 0)
		sysfatal("getwindow: %r");
	draw(screen, screen->r, display->black, nil, ZP);
	box = screen->r;
	box.min.x += Spacing;
	snprint(buf, sizeof(buf), "%s/%02d", album->up->name, album->month + 1);
	string(screen, box.min, display->white, ZP, display->defaultfont, buf);
	box = screen->r;
	box.min.x += Spacing;
	box.min.y += 40;
	box.min.y -= Spacing;
	box.max.y -= 40;
	reflow(album, box);
	for(img = album->images.head; img != nil; img = img->next){
		if(img->thumb == nil)
			continue;
		drawthumb(img);
	}
	flushimage(display, 1);
}

void
hover(Point p)
{
	AlbumImg *img, *prev;
	Rectangle r;

	prev = album->hover;
	album->hover = nil;
	if(prev != nil)
		drawthumb(prev);
	for(img = album->images.head; img != nil; img = img->next){
		if(img->thumb == nil)
			continue;
		if(ptinrect(p, img->box)){
			album->hover = img;
			drawthumb(img);
			break;
		}
	}
	r = screen->r;
	r.min.x += Spacing;
	r.min.y = r.max.y - 16;
	draw(screen, r, display->black, nil, ZP);
	if(img != nil)
		string(screen, r.min, display->white, ZP, display->defaultfont, img->name);
	flushimage(display, 1);
}

void
togglesel(AlbumImg *i)
{
	i->selected = !i->selected;
	drawthumb(i);
	flushimage(display, 1);
}

void
clicked(int btns)
{
	if((btns&1) != 0)
	if(album->hover != nil)
		togglesel(album->hover);
}

static Album*
findnextalbum1(Album *a, int step)
{
	int i, m;
	YearIdx *y;

	y = a->up;
	m = a->month + step;
	while(y != nil){
		for(i = m; 0 <= i && i < 12; i += step){
			if(y->months[i] != nil)
				return y->months[i];
		}
		if(step > 0){
			y = y->next;
			m = 0;
		}else{
			y = y->prev;
			m = 11;
		}
	}
	return nil;
}

static Album*
findnextalbum(Album *a)
{
	return findnextalbum1(a, +1);
}

static Album*
findprevalbum(Album *a)
{
	return findnextalbum1(a, -1);
}

static void
usage(char *argv0)
{
	fprint(2, "usage: %s\n", argv0);
	threadexitsall("usage");
}

void
threadmain(int argc, char **argv)
{
	int i;
	YearIdx *y;
	Keyboardctl *kc;
	Mousectl *mc;
	Rune r;
	int buttons;
	Album *next;

	ARGBEGIN{
	default:
		usage(argv[0]);
	}ARGEND;
	if(initdraw(nil, nil, "imgview") < 0)
		sysfatal("initdraw: %r");
	if((kc = initkeyboard(nil)) == nil)
		sysfatal("initkeyboard: %r");
	if((mc = initmouse(nil, screen)) == nil)
		sysfatal("initmouse: %r");
	db = openimgdb(".");
	if(db == nil)
		sysfatal("openimgdb: %r");
	for(y = db->years; y != nil; y = y->next){
		for(i = 11; i >= 0; i--){
			if(y->months[i] != nil)
			if(y->months[i]->images.head != nil){
				album = db->years->months[i];
				goto AlbumSelected;
			}
		}
	}
AlbumSelected:
	if(album == nil)
		sysfatal("no album");
	if(album->images.head == nil)
		sysfatal("no images");
	resized(0);
	buttons = 0;
	for(;;){
		enum{
			Aresize,
			Amouse,
			Akbd,
		};
		Alt a[] = {
			{mc->resizec, nil, CHANRCV},
			{mc->c, nil, CHANRCV},
			{kc->c, &r, CHANRCV},
			{nil, nil, CHANEND},
		};
		switch(alt(a)){
		case Aresize:
			resized(1);
			break;
		case Amouse:
			hover(mc->xy);
			clicked(~mc->buttons & buttons);
			buttons = mc->buttons;
			break;
		case Akbd:
			switch(r){
			case Kdel:
			case 'q':
				threadexitsall(nil);
				break;
			case Kleft:
				if((next = findprevalbum(album)) != nil){
					album = next;
					resized(0);
				}
				break;
			case Kright:
				if((next = findnextalbum(album)) != nil){
					album = next;
					resized(0);
				}
				break;
			}
			break;
		default:
			break;
		}
	}

}
