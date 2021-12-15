#include "pam.h"

#define WIDTH  320
#define HEIGHT 240

void test_bitmap()
{
    uint8_t R = 255;
    uint8_t G = 255;
    uint8_t B = 255;
    int x = WIDTH/2;
    int y = HEIGHT/2;
    bitmap *bm;
    bm = init_bitmap_fb(1);
    clear_bitmap(bm);
    for (int t= 0; t <100;t++){
//    circle(bm, x, y, 250, R, G, B);
	for (int i=20; i <40; i++){
	    squircle(bm, x, y, 100, 75, i/10.0, R, G, B);
	    //  ellipse(bm, x, y, 250, 300, R, G, B);
//	    write_pam(STDOUT_FILENO, bm);
	    write_fb(bm);
	    clear_bitmap(bm);
	}
	for (int i=40; i >20; i--){
	    squircle(bm, x, y, 100, 75, i/10.0, R, G, B);
	    //  ellipse(bm, x, y, 250, 300, R, G, B);
//	    write_pam(STDOUT_FILENO, bm);
	    write_fb(bm);
	    clear_bitmap(bm);
	}
    }
}

void test_graph()
{
    graph g;
    bitmap *bm;
    double xmin = 0;
    double xmax = 10000;
    double ymin = 0;
    double ymax = 5000;
    double x = xmax/2;
    double y = ymax/2;
    uint8_t R = 0;
    uint8_t G = 255;
    uint8_t B = 0;
    

    bm = init_bitmap(WIDTH, HEIGHT, 3);
    clear_bitmap(bm);
    
    init_graph( &g, bm, xmin, xmax, ymin, ymax);
    for (int t= 0; t <100;t++){
//	circle_graph(&g, x, y, xmax/10, R, G, B);
	for (int i=20; i <40; i++){
	    squircle_graph(&g, x, y, xmax/4, ymax/3, i/10.0, R, G, B);
//	    ellipse_graph(&g, x, y, xmax/2, ymax/4, R, G, B);
	    write_pam(STDOUT_FILENO, bm);
	    clear_bitmap(bm);
	}
	for (int i=40; i >20; i--){
	    squircle_graph(&g, x, y, xmax/4, ymax/3, i/10.0, R, G, B);
//	    ellipse_graph(&g, x, y, 250, 300, R, G, B);
	    write_pam(STDOUT_FILENO, bm);
	    clear_bitmap(bm);
	}
    }

}


int main(int argc, char **argv)
{
    int fbfd = 0;
    struct fb_var_screeninfo vinfo;
    struct fb_fix_screeninfo finfo;
    long int screensize = 0;
    char *fbp = 0;
    int x = 0, y = 0;
    long int location = 0;

    test_bitmap();

#if 0
    
    // Open the file for reading and writing
    fbfd = open("/dev/fb1", O_RDWR);
    if (fbfd == -1) {
        perror("Error: cannot open framebuffer device");
        exit(1);
    }
    printf("The framebuffer device was opened successfully.\n");
    
    // Get fixed screen information
    if (ioctl(fbfd, FBIOGET_FSCREENINFO, &finfo) == -1) {
        perror("Error reading fixed information");
        exit(2);
    }
    
    // Get variable screen information
    if (ioctl(fbfd, FBIOGET_VSCREENINFO, &vinfo) == -1) {
        perror("Error reading variable information");
        exit(3);
    }
    
    printf("%dx%d, %dbpp\n", vinfo.xres, vinfo.yres, vinfo.bits_per_pixel);
    
    // Figure out the size of the screen in bytes
    screensize = vinfo.xres * vinfo.yres * vinfo.bits_per_pixel / 8;
    
    // Map the device to memory
    fbp = (char *)mmap(0, screensize, PROT_READ | PROT_WRITE, MAP_SHARED, fbfd, 0);
    if ((int)fbp == -1) {
        perror("Error: failed to map framebuffer device to memory");
        exit(4);
    }
    printf("The framebuffer device was mapped to memory successfully.\n");
    memset(fbp,0,screensize);
    
    x = 100; y = 100;       // Where we are going to put the pixel
    
    // Figure out where in memory to put the pixel
    for (y = 100; y < 200; y++)
        for (x = 100; x < 200; x++) {
	    
            location = (x+vinfo.xoffset) * (vinfo.bits_per_pixel/8) +
                       (y+vinfo.yoffset) * finfo.line_length;
	    
            if (vinfo.bits_per_pixel == 32) {
                *(fbp + location) = 100;        // Some blue
                *(fbp + location + 1) = 15+(x-100)/2;     // A little green
                *(fbp + location + 2) = 200-(y-100)/5;    // A lot of red
                *(fbp + location + 3) = 0;      // No transparency
		//location += 4;
            } else  { //assume 16bpp
                int b = 10;
                int g = (x-100)/6;     // A little green
                int r = 31-(y-100)/16;    // A lot of red
                unsigned short int t = r<<11 | g << 5 | b;
                *((unsigned short int*)(fbp + location)) = t;
            }
	    
        }
    munmap(fbp, screensize);
    close(fbfd);
#endif
}
