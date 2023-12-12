#include <u.h>
#include <libc.h>
#include <draw.h>
#include <thread.h>
#include <keyboard.h>
#include <mouse.h>
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

Album*
openalbum(void)
{
	int dfd;
	Dir *dirs;
	long n;
	Album *album;
	AlbumImg *img;

	dirs = nil;
	album = mallocz(sizeof(Album), 1);
	if(album == nil)
		goto Error;
	dfd = open(".", OREAD);
	if(dfd < 0)
		goto Error;
	n = dirreadall(dfd, &dirs);
	close(dfd);
	while(n-- > 0){
		if((dirs[n].mode&DMDIR) == 0)
		if(endswith(dirs[n].name, ".thumb.1") != 0){
			img = loadalbumimg(dirs[n].name);
			if(img == nil)
				goto Error;
			img->album = album;
			if(album->images != nil)
				album->images->prev = img;
			img->next = album->images;
			album->images = img;
		}
	}
	return album;
Error:
	free(dirs);
	freealbum(album);
	return nil;
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

	aimg = nil;
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
	freealbumimg(aimg);
	return nil;
}

void
freealbumimg(AlbumImg *img)
{
	free(img->name);
	free(img->thumb);
	free(img);
}

void
freealbum(Album *album)
{
	AlbumImg *img, *tmp;

	img = album->images;
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
	for(i = a->images; i != nil; i = i->next){
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
	}else if(i->album->hover == i){
		draw(screen, r, display->black, nil, ZP);
		draw(screen, insetrect(r, 1), display->white, nil, ZP);
	}
}

void
resized(int new)
{
	Rectangle box;
	AlbumImg *img;

	if(new && getwindow(display, Refnone) < 0)
		sysfatal("getwindow: %r");
	draw(screen, screen->r, display->black, nil, ZP);
	box = screen->r;
	box.min.x += Spacing;
	box.min.y += 40;
	box.min.y -= Spacing;
	box.max.y -= 40;
	reflow(album, box);
	for(img = album->images; img != nil; img = img->next){
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
	for(img = album->images; img != nil; img = img->next){
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

static void
usage(char *argv0)
{
	fprint(2, "usage: %s\n", argv0);
	threadexitsall("usage");
}

void
threadmain(int argc, char **argv)
{
	Keyboardctl *kc;
	Mousectl *mc;
	Rune r;
	int buttons;

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
	album = openalbum();
	if(album == nil)
		sysfatal("openalbum: %r");
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
			}
			break;
		default:
			break;
		}
	}

}
