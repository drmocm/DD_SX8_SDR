#include "pam.h"

#define WIDTH  1920
#define HEIGHT 1080

void test_bitmap()
{
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
	for (int i=20; i <40; i++){
	    squircle(bm, x, y, 500, 250, i/10.0, R, G, B);
	    //  ellipse(bm, x, y, 250, 300, R, G, B);
	    write_pam(STDOUT_FILENO, bm);
	    clear_bitmap(bm);
	}
	for (int i=40; i >20; i--){
	    squircle(bm, x, y, 500, 250, i/10.0, R, G, B);
	    //  ellipse(bm, x, y, 250, 300, R, G, B);
	    write_pam(STDOUT_FILENO, bm);
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


int main(int argc, char **argv){
    test_graph();
}
