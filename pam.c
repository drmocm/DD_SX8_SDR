/*
ddsx8-spec is an **example** program written in C to show how to use 
the SDR mode of the DigitalDevices MAX SX8 to get IQ data.

Copyright (C) 2021  Marcus Metzler

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "pam.h"

void plot(bitmap *bm, int x, int y,
	  uint8_t R,
	  uint8_t G,
	  uint8_t B)
{
    int width = bm->width;
    int height = bm->height;
    int d = bm->depth;
    uint8_t *p = bm->data;
    
    if ( x > width || x < 0 ||	 y > height || y < 0) return; 
    int k = d*(x+width*y);

    p[k] = R;
    p[k+1] = G;
    p[k+2] = B;
}

void plotline(bitmap *bm, int x, int y, int x2, int y2, 
	      uint8_t r,
	      uint8_t g,
	      uint8_t b)
{
    int dx;
    int dy;
    int stepx = 0;
    int start = 0;
    int stop = 0;
    int inc = 0;
    int neg = 1;
    int step = 1;
    int xyadd = 0;
    int dd = 0;
    int width = bm->width;
    int height = bm->height;
    int d = bm->depth;
    uint8_t *p= bm->data;

    if ( x > width || x < 0 || x2 > width || x2 < 0 || 
	 y > height || y < 0 || y2 > height || y2 < 0) return; 
    dx = abs(x2 - x);
    dy = abs(y2 - y);

    if (dx > dy) stepx = 1;

    start = d*(x+y*width);
    if (stepx){
	stop = d*(x2+y*width);
	step = d;
	if (x > x2) {
	    step = -d;
	}
	inc = dy;
	dd = dx;
	if (y > y2) neg = -d*width;
	else neg = d*width;
    } else {
	stop = d*(x+y2*width);
	step = d*width;
	if (y > y2){
	    step = -d*width;
	}
	inc = dx;
	dd = dy;
	if (x > x2) neg = -d;
	else neg = d;
    }

    int fac = 0;
    for (int i=start; i != stop; i += step){
	int k = i+fac;
	xyadd += inc;
	if (xyadd > dd) {
	    fac += neg;
	    xyadd -= dd;
	}
	p[k]= r;
	p[k+1]= g;
	p[k+2]= b;
    }
}

void box(bitmap *bm, int x0, int y0, int x1, int y1,
	 uint8_t R,uint8_t G, uint8_t B, int f)
{
    if (!f){
	plotline(bm, x0, y0, x0, y1, R, G, B);
	plotline(bm, x0, y1, x1, y1, R, G, B);
	plotline(bm, x1, y1, x0, y1, R, G, B);
	plotline(bm, x0, y1, x0, y0, R, G, B);
    } else {
	for (int i=x0; i < x1; i++){
	    plotline(bm, i, y0, i, y1, R, G, B);
	}
    }
}

void circle(bitmap *bm, int x0, int y0, int r,
	    uint8_t R,uint8_t G, uint8_t B)
{
    int x = r;
    int y = 0;
    int d = 1 - r;
     
    while(x > y){
	y++;
	if (d <= 0){
	    d = d+2*y+ 1;
	}  else {
            x--;
            d = d+2*y-2*x+1;
        }
	if (x < y) break;
	plot(bm, x+x0,y+y0, R,G,B);
	plot(bm, -x+x0,y+y0, R,G,B);
	plot(bm, x+x0,-y+y0, R,G,B);
	plot(bm, -x+x0,-y+y0, R,G,B);
	if (x != y){
	    plot(bm, y+x0,x+y0, R,G,B);
	    plot(bm, -y+x0,x+y0, R,G,B);
	    plot(bm, y+x0,-x+y0, R,G,B);
	    plot(bm, -y+x0,-x+y0, R,G,B);
	}
    }
}
void squircle(bitmap *bm, int x0, int y0, int r, int ry, double n,
	    uint8_t R, uint8_t G, uint8_t B)
{
    int x = r;
    int y = 0;
    int d = 1 - r;
     
    int xl = (int)(pow((double)x/r,2/n)*r);
    int yl = (int)(pow((double)y/r,2/n)*r);
    while(x > y){
	y++;
	if (d <= 0){
	    d = d+2*y+ 1;
	}  else {
            x--;
            d = d+2*y-2*x+1;
        }
	if (x < y) break;
	int xx = (int)(pow((double)x/r,2/n)*r);
	int yy = (int)(pow((double)y/r,2/n)*r);
	
	int pyy = yy*ry/r;
	int pyl = yl*ry/r;
	plotline(bm, xl+x0,pyl+y0, xx+x0,pyy+y0, R,G,B);
	plotline(bm, -xl+x0,pyl+y0, -xx+x0,pyy+y0, R,G,B);
	plotline(bm, xl+x0,-pyl+y0, xx+x0,-pyy+y0, R,G,B);
	plotline(bm, -xl+x0,-pyl+y0, -xx+x0,-pyy+y0, R,G,B);

	if (x != y){
	    int pxx = xx*ry/r;
	    int pxl = xl*ry/r;
	    plotline(bm, yl+x0,pxl+y0, yy+x0,pxx+y0, R,G,B);
	    plotline(bm, -yl+x0,pxl+y0, -yy+x0,pxx+y0, R,G,B);
	    plotline(bm, yl+x0,-pxl+y0, yy+x0,-pxx+y0, R,G,B);
	    plotline(bm, -yl+x0,-pxl+y0, -yy+x0,-pxx+y0, R,G,B);
	}
	xl = xx;
	yl = yy;
    }
}

void ellipse(bitmap *bm, int x0, int y0 ,int rx, int ry,
	     uint8_t R,uint8_t G, uint8_t B)
{
    int64_t dx, dy, d;
    int x, y;
    int64_t r2x = rx*rx;
    int64_t r2y = ry*ry;
    x = 0;
    y = ry;
 
    d = r2y - r2x*ry + r2x/4.0;
    dx = 2*r2y*x;
    dy = 2*r2x*y;
        
    do {
 	plot(bm, x+x0,y+y0, R,G,B);
 	plot(bm, -x+x0,y+y0, R,G,B);
 	plot(bm, x+x0,-y+y0, R,G,B);
 	plot(bm, -x+x0,-y+y0, R,G,B);
	
        if (d < 0) {
            d = d + dx + r2y;
        } else {
            y--;
            d = d + dx - dy + r2y;
	    dy = dy - 2*r2x;
        }
	x++;
	dx = dx+ 2*r2y;
    } while (dx < dy);
  
    d = r2y*(2*x+1)*(2*x+1)/4 -r2x*r2y +  r2x*(y-1)*(y-1);

    do {
 	plot(bm, x+x0,y+y0, R,G,B);
 	plot(bm, -x+x0,y+y0, R,G,B);
 	plot(bm, x+x0,-y+y0, R,G,B);
 	plot(bm, -x+x0,-y+y0, R,G,B);

        if (d >= 0)
        {
	    dy = 2*r2x*y;
            d = d - dy + r2x;
        } else {
            x++;
            d = d + dx - dy + r2x;
            dx = dx + 2*r2y;
        }
	y--;
	dy = dy - 2*r2x;
    } while ( y>0);
}

void coordinate_axes(bitmap *bm, uint8_t r,
			 uint8_t g, uint8_t b){
    int i;
//    ellipse(bm, bm->width/2, bm->height/2, 300,200,r,g,b);
//    circle(bm, bm->width/2, bm->height/2, 300,r,g,b);
    plotline(bm, bm->width/2, bm->height-1,
	     bm->width/2, 0, r,g,b);
/*
    plotline(bm, bm->width-1, 0,
	     0, bm->height-1,
	     r,g,b);
    plotline(bm, 0, 0,
	     bm->width-1, bm->height-1,
	     r,g,b);
    plotline(bm, 0, bm->height-20,
	     bm->width-1, bm->height-20,
	     r,g,b);
*/
}

void clear_range_bitmap(bitmap *bm, int first, int last)
{
    int depth = bm->depth;
    int width = bm->width;
    int height = bm->height;
    int length = last-first;
//    fprintf(stderr,"first %d length %d\n",first, length);
    for (int i = first; i < last; i++){
	plotline(bm, i, 0, i, height,0,0,0);
    }
}


void get_rgb(int val, uint8_t *R, uint8_t *G, uint8_t *B)
    {
    *R = 0;
    *G = 0;
    *B = 0;

    val = val & 0x1ff;
    if (val <256){
	*R = 0;
	*G = val;
	*B = 255-val;
    } else if (val < 512){
	*R = val;
	*G = 255-val;
	*B = 0;
    } 
    
}

static double check_range(double *pow, int width, double *ma, double *mi)
{    
    double max = 0.0;
    double min = 40.0;
    double range = 0;
    for (int i=0; i < width; i++){
	if ( isfinite(pow[i]) && pow[i] > max ) max = pow[i];
	if ( isfinite(pow[i]) && pow[i] < min ) min = pow[i];
    }
    range = max-min;
    *mi = min;
    *ma = max;
    //  fprintf(stderr,"range %f min %f\n",range,min);
    return range;
}

void display_array(bitmap *bm, double *pow, int length,
		   int startpos, double ymin, double scale, double range)
{
    uint8_t R = 0;
    uint8_t G = 0;
    uint8_t B = 0;
    double min = 0, max = 0;
    int i;
    int lastx = -1;
    int lasty = -1;
    int width = bm->width;
    int height = bm->height;

    if (!range) range = check_range(pow, length, &max, &min);
    if (ymin) min = ymin;
    for (i = 0; i < length; i++){
	if (!isfinite(pow[i])) continue;
	int q = (int)((double)height*(pow[i]-min)/(1.1*range)+.05*height);
	int x = 0;
	int y = height-q;

	x = (int)((i+startpos)*scale);
	
	get_rgb(q*255/height, &R, &G, &B);
	
//	fprintf(stderr,"range %f min %f\n",range,min);
//	fprintf (stderr,"y: %d q: %d pow: %f height %d\n",y,q,pow[i],height);
	if (lasty>=0)
	    plotline(bm, lastx, lasty, x, y, R,G,B);
	else plot(bm, x, y, R,G,B);
	lastx = x;
	lasty = y;
    }
}

void write_pam (int fd, bitmap *bm)
{
    char HEAD[255];
    sprintf(HEAD,"P7\nWIDTH %d\nHEIGHT %d\nDEPTH %d\nMAXVAL 255\nTUPLTYPE RGB\nENDHDR\n",bm->width,bm->height,bm->depth);
    int headlen = strlen(HEAD);
    int size = bm->width*bm->height*bm->depth;    
    int we=0;
    we=write(fd,HEAD,headlen);
    we=write(fd,bm->data,size);
}

void write_csv (int fd, int width, uint32_t step, uint32_t start_freq,
		double *pow, int center, int64_t str, int min)
{
    FILE* fp = fdopen(fd, "w");
    int start = 0;
    int end = width;
    if (center){
	start = width/4;
	end = width/4*3;
    }
    if (min==1){
	fprintf(fp,"# start %.3f step %.2f\n",(double)(start_freq)/1000.0,
	    (double)(step)/1000.0);
    }
    for (int i = start; i < end; i++){
	if (!min){
	    if (center == 2){
		fprintf(fp,"%.3f, %.2f, %lld.%03lld\n",(double)(start_freq+step*i)/1000.0, pow[i], str/1000, abs(str%1000));
	    } else {
		fprintf(fp,"%.3f, %.2f\n",(double)(start_freq+step*i)/1000.0, pow[i]);
	    }
	} else {
	    fprintf(fp,"%.2f\n",pow[i]);
	}
    }

    fflush(fp);
}

void clear_bitmap(bitmap *bm)
{
    if (!bm){
	fprintf(stderr,"Bitmap pointer is zero in clear_bitmap\n");
	exit(1);
    }
    memset(bm->data,0,bm->width*bm->height*bm->depth*sizeof(uint8_t));
}

bitmap *init_bitmap(int width, int height, int depth)
{
    bitmap *bm;

    bm = (bitmap *) malloc(sizeof(bitmap));
    bm->width = width;
    bm->height = height;
    bm->depth = depth;
    if (!( bm->data = (uint8_t *) malloc(sizeof(uint8_t) *
						  width*height*depth)))
    {
	fprintf(stderr,"not enough memory\n");
	return NULL;
    }
    return bm;
}

void delete_bitmap(bitmap *bm)
{
    if (!bm){
	fprintf(stderr,"Bitmap pointer is in delete_bitmap zero\n");
	exit(1);
    }
    free (bm->data);
    free (bm);
}

void init_graph(graph *g, bitmap *bm, double xmin, double xmax,
	       double ymin, double ymax)
{
    g->bm = bm;
    g->xmin = xmin;
    g->xmax = xmax;
    g->ymin = ymin;
    g->ymax = ymax;
    g->lastx = xmin;
    g->lasty = ymin;
    g->xrange = xmax-xmin;
    g->yrange = ymax-ymin;
}

void plot_graph(graph *g, double x, double y, uint8_t  R, uint8_t G, uint8_t B)
{
    int ix;
    int iy;

    if ( x > g->xmax || x < g->xmin || y > g->ymax || y < g->ymin) return; 
    ix = (int)((x-g->xmin)*g->bm->width/g->xrange);
    iy = (int)((y-g->ymin)*g->bm->height/g->yrange);
    
    plot(g->bm,ix,iy,R,G,B);
    g->lastx = x;
    g->lasty = y;
}

void plotline_graph(graph *g, double x, double y, double x2, double y2, 
	      uint8_t R,
	      uint8_t G,
	      uint8_t B)
{
    bitmap *bm= g->bm;
    // maybe better clipping later
    int ix,ix2;
    int iy,iy2;
    int width = g->bm->width;
    int height = g->bm->height;

    if ( x > g->xmax || x < g->xmin || y > g->ymax || y < g->ymin || 
         x2 > g->xmax || x2 < g->xmin || y2 > g->ymax || y2 < g->ymin) return; 

    ix = (int)((x-g->xmin)*width/g->xrange);
    iy = height-(int)((y-g->ymin)*height/g->yrange);
    ix2 = (int)((x2-g->xmin)*width/g->xrange);
    iy2 = height-(int)((y2-g->ymin)*height/g->yrange);

    plotline (bm,ix,iy,ix2,iy2,R,G,B);
    g->lastx = x2;
    g->lasty = y2;
}

void plot_to_graph(graph *g, double x2, double y2, 
	      uint8_t R,
	      uint8_t G,
	      uint8_t B)
{
    bitmap *bm= g->bm;
    // maybe better clipping later
    int ix,ix2;
    int iy,iy2;
    double x, y;
    int width = g->bm->width;
    int height = g->bm->height;
    
    if ( x2 > g->xmax || x2 < g->xmin || y2 > g->ymax || y2 < g->ymin) return; 
    x = g->lastx;
    y = g->lasty;
    
    ix = (int)((x-g->xmin)*width/g->xrange);
    iy = height-(int)((y-g->ymin)*height/g->yrange);
    ix2 = (int)((x2-g->xmin)*width/g->xrange);
    iy2 = height-(int)((y2-g->ymin)*height/g->yrange);

    
    plotline (bm,ix,iy,ix2,iy2,R,G,B);
    g->lastx = x2;
    g->lasty = y2;
}

void box_graph(graph *g, double x0, double y0, double x1, double y1, 
	      uint8_t R,
	      uint8_t G,
	       uint8_t B, int f)
{
    if (!f){
	plotline_graph(g, x0, y0, x0, y1, R, G, B);
	plot_to_graph(g, x1, y1, R, G, B);
	plot_to_graph(g, x1, y0, R, G, B);
	plot_to_graph(g, x0, y0, R, G, B);
    } else {
	int width = g->bm->width;
	int height = g->bm->height;
	int ix0 = (int)((x0-g->xmin)*width/g->xrange);
	int iy0 = height-(int)((y0-g->ymin)*height/g->yrange);
	int ix1 = (int)((x1-g->xmin)*width/g->xrange);
	int iy1 = height-(int)((y1-g->ymin)*height/g->yrange);
        box(g->bm, ix0,iy0, ix1,iy1, R,G,B,f);
    }
}

void ellipse_graph(graph *g, double x, double y, double rx, double ry,
		   uint8_t R,
		   uint8_t G,
		   uint8_t B)
{
    bitmap *bm= g->bm;
    int ix, irx;
    int iy, iry;
    int width = g->bm->width;
    int height = g->bm->height;
    
    if ( x > g->xmax || x < g->xmin || y > g->ymax || y < g->ymin) return; 
    ix = (int)((x-g->xmin)*width/g->xrange);
    iy = height-(int)((y-g->ymin)*height/g->yrange);
    irx = (int)(rx*width/g->xrange);
    iry = (int)(ry*height/g->yrange);
    ellipse( bm, ix, iy , irx, iry, R, G, B);
    
    g->lastx = x;
    g->lasty = y;   
}

void circle_graph(graph *g, double x, double y, double r,
		  uint8_t R,
		  uint8_t G,
		  uint8_t B)
{
    ellipse_graph(g, x, y, r, r, R, G, B);
}

void squircle_graph(graph *g, double x, double y, double rx, double ry,
		    double n,
		    uint8_t R,
		    uint8_t G,
		    uint8_t B)
{
    bitmap *bm= g->bm;
    int ix, irx;
    int iy, iry;
    int width = g->bm->width;
    int height = g->bm->height;
    
    if ( x > g->xmax || x < g->xmin || y > g->ymax || y < g->ymin) return; 
    ix = (int)((x-g->xmin)*width/g->xrange);
    iy = height-(int)((y-g->ymin)*height/g->yrange);
    irx = (int)(rx*width/g->xrange);
    iry = (int)(ry*height/g->yrange);
    squircle( bm, ix, iy , irx, iry, n,  R, G, B);
    
    g->lastx = x;
    g->lasty = y;   
}

void clear_range_graph(graph *g, double dfirst, double dlast)
{
    int first;
    int last;
    int width = g->bm->width;
    int height = g->bm->height;

    first = (int)((dfirst-g->xmin)*width/g->xrange)+1;
    last = (int)((dlast-g->xmin)*width/g->xrange);
    clear_range_bitmap(g->bm,first,last);
}

void display_array_graph(graph *g, double *x, double *y, int first, int length, int f)
{
    uint8_t R = 0;
    uint8_t G = 0;
    uint8_t B = 0;

    if (!f){
	for (int i = first; i < first+length; i++){
	    get_rgb((int)(512*(y[i] - g->ymin)/g->yrange), &R, &G, &B);
	    plot_to_graph(g, x[i], y[i], R,G,B);
	}
    } else {
	for (int i = first; i < first+length; i++){
	    int l = (int)(512*(y[i] - g->ymin)/g->yrange);
	    for (int j = 0; j < l; j++){
		double y0 = g->yrange*j/512+g->ymin;
		double y1 = g->yrange*(j+1)/512+g->ymin;
		get_rgb(j, &R, &G, &B);	
		plotline_graph(g, x[i], y0, x[i], y1, R,G,B);
	    }
	}
	
    }
}

void display_array_graph_c(graph *g, double *x, double *y, int first,
			   int length, uint8_t R, uint8_t G, uint8_t B, int f )
{
    for (int i = first; i < first+length; i++){
	if (!f){
	    plot_to_graph(g, x[i], y[i], R,G,B);
	} else {
	    plotline_graph(g, x[i], g->ymin, x[i], y[i], R,G,B);
	}
    }
}

void display_peak(graph *g, double mid, double width, double height,
		  uint8_t R, uint8_t G, uint8_t B, int f )
{
    double y0 = g->ymin;
    box_graph( g, mid-width/2, y0, mid+width/2, height, R, G, B, f);
    if (f)
	plotline_graph( g, mid, y0, mid, height, 255-R, 255-G, 255-B);
    else 
	plotline_graph( g, mid, y0, mid, height, R, G, B);
}


void graph_range(graph *g, double *x, double *y, int width)
{    
    double range = 0;

    g->ymin = y[0];
    g->ymax = y[0];
    g->xmin = x[0];
    g->xmax = x[0];

    for (int i=0; i < width; i++){
	if ( isfinite(y[i]) && y[i] > g->ymax ) g->ymax = y[i];
	if ( isfinite(y[i]) && y[i] < g->ymin ) g->ymin = y[i];
	if ( x[i] > g->xmax ) g->xmax = x[i];
	if ( x[i] < g->xmin ) g->xmin = x[i];
    }
    g->yrange = g->ymax-g->ymin;
    g->ymin -=  g->yrange*.05;
    g->ymax +=  g->yrange*.05;
    
    g->yrange = g->ymax-g->ymin;
    g->xrange = g->xmax-g->xmin;
}
