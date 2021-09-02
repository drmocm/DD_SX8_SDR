#include "pam.h"

#define WIDTH  1920
#define HEIGHT 1080

int main(int argc, char **argv){
    graph g;
    uint8_t R = 255;
    uint8_t G = 255;
    uint8_t B = 255;
    int x = WIDTH/2;
    int y = HEIGHT/2;
    bitmap *bm;
    bm = init_bitmap(WIDTH, HEIGHT, 3);
    clear_bitmap(bm);
    for (int t= 0; t <100;t++){
//    circle(bm, x, y, 250, R, G, B);
	for (int i=0; i <40; i++){
	    squircle(bm, x, y, 250, 250, i/10.0, R, G, B);
	    //  ellipse(bm, x, y, 250, 300, R, G, B);
	    write_pam(STDOUT_FILENO, bm);
	    clear_bitmap(bm);
	}
	for (int i=40; i >0; i--){
	    squircle(bm, x, y, 250, 250, i/10.0, R, G, B);
	    //  ellipse(bm, x, y, 250, 300, R, G, B);
	    write_pam(STDOUT_FILENO, bm);
	    clear_bitmap(bm);
	}
    }
}
