/*
Copyright (c) 2011 Daniel Minor 

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
THE SOFTWARE.
*/

#include "oddson_tree_zorder.h"

#include <float.h>
#include <cstdio>
#include <cstdlib>

struct Point {

    double v[2]; 
    long id;

    double &operator[](const int &index) 
    { 
        return v[index]; 
    } 

    const double &operator[](const int &index) const 
    { 
        return v[index]; 
    } 
};

void render_tree(FILE *f, struct OddsonTree<Point>::CacheNode *tree)
{ 
    if (!tree) return;

    double &x1 = tree->a[0];
    double &x2 = tree->b[0];
    double &y1 = tree->a[1];
    double &y2 = tree->b[1]; 

    if (tree->nn) { 
        fprintf(f, "colour-site-%d\n", tree->nn->id);
        fprintf(f, "%.0f %.0f %.0f %.0f node-bounds\n", x1, x2, y1, y2); 
    } else { 
        render_tree(f, tree->left);
        render_tree(f, tree->right);
    } 
}

Point *read_points(FILE *f, int *pt_count)
{
    char buf[80];
    fscanf(f, "%d", pt_count);
    fgets(buf, 80, f);

    if (pt_count < 0) {
        fprintf(stderr, "error: invalid point count %d\n", *pt_count);
        return 0;
    }

    Point *pts = new Point[*pt_count];

    double x, y;
    for (int i = 0; i < *pt_count; ++i) {
        fscanf(f, "%lf, %lf", &x, &y);
        pts[i][0] = x;
        pts[i][1] = y; 
        pts[i].id = i;
    }

    return pts; 
}

int main(int argc, char **argv)
{ 
    if (argc != 3) {
        fprintf(stderr, "usage: render_tree <points> <sample>\n");
        exit(1);
    }

    int n, m;

    FILE *f = fopen(argv[1], "r");

    if (!f) {
        fprintf(stderr, "error: could not open points file: %s\n", argv[1]);
        exit(1); 
    }

    Point *pts = read_points(f, &n);
    if (!pts) exit(1);

    fclose(f);

    f = fopen(argv[2], "r");

    if (!f) {
        fprintf(stderr, "error: could not open sample file: %s\n", argv[2]);
        exit(1); 
    }

    Point *sample = read_points(f, &m);
    if (!sample) exit(1);

    fclose(f);

    OddsonTree<Point> oot(2, pts, n, sample, m);

    fprintf(stdout, "%%\n");

    //define point function for later
    fprintf(stdout, "/draw-point {\n");
    fprintf(stdout, "    /y exch def\n");
    fprintf(stdout, "    /x exch def\n");
    fprintf(stdout, "    gsave\n");
    fprintf(stdout, "    newpath\n");
    fprintf(stdout, "    1.0 0.5 0.7 setrgbcolor\n");
    fprintf(stdout, "    x y 1 0 360 arc\n");
    fprintf(stdout, "    closepath\n");
    fprintf(stdout, "    stroke\n");
    fprintf(stdout, "    grestore\n");
    fprintf(stdout, "} def\n");

    //node bounding box
    fprintf(stdout, "/draw-line {\n");
    fprintf(stdout, "    /y2 exch def\n");
    fprintf(stdout, "    /x2 exch def\n");
    fprintf(stdout, "    /y1 exch def\n");
    fprintf(stdout, "    /x1 exch def\n");
    fprintf(stdout, "    gsave\n");
    fprintf(stdout, "    0.7 0.1 0.1 setrgbcolor\n");
    fprintf(stdout, "    newpath\n");
    fprintf(stdout, "    x1 y1 moveto\n");
    fprintf(stdout, "    x2 y2 lineto\n");
    fprintf(stdout, "    closepath\n");
    fprintf(stdout, "    stroke \n");
    fprintf(stdout, "    grestore\n");
    fprintf(stdout, "} def\n");

    //node bounding box
    fprintf(stdout, "/node-bounds {\n");
    fprintf(stdout, "    /y2 exch def\n");
    fprintf(stdout, "    /y1 exch def\n");
    fprintf(stdout, "    /x2 exch def\n");
    fprintf(stdout, "    /x1 exch def\n");
    fprintf(stdout, "    gsave\n");
//    fprintf(stdout, "    0.7 setgray\n");
    fprintf(stdout, "    newpath\n");
    fprintf(stdout, "    x2 y2 moveto\n");
    fprintf(stdout, "    x1 y2 lineto\n");
    fprintf(stdout, "    x1 y1 lineto\n");
    fprintf(stdout, "    x2 y1 lineto\n");
    fprintf(stdout, "    closepath\n");
    fprintf(stdout, "    stroke \n");
    fprintf(stdout, "    grestore\n");
    fprintf(stdout, "} def\n");

    //setup colours
    for (int i = 0; i < n; ++i) {
        fprintf(stdout, "/colour-site-%d {%.1f %.1f %.1f setrgbcolor } def\n", i,
            (double)rand()/(double)RAND_MAX,
            (double)rand()/(double)RAND_MAX,
            (double)rand()/(double)RAND_MAX);
    }

    //draw sample points
    for (int i = 0; i < m; ++i) {
        fprintf(stdout, "%.1f %.1f draw-point\n", sample[i][0], sample[i][1]);
    }

    render_tree(stdout, oot.root);

    delete[] pts;
    delete[] sample;

}