
#include <algorithm>
#include <iostream>
#include <fstream>
#include <cassert>
#include <vector>
#include <limits>

#include <sys/time.h>

#include <CGAL/basic.h> 
#include <CGAL/Simple_cartesian.h>

// includes for defining the Voronoi diagram adaptor
#include <CGAL/algorithm.h>
#include <CGAL/Exact_predicates_inexact_constructions_kernel.h>
#include <CGAL/Delaunay_triangulation_2.h>
#include <CGAL/Voronoi_diagram_2.h>
#include <CGAL/Delaunay_triangulation_adaptation_traits_2.h>
#include <CGAL/Delaunay_triangulation_adaptation_policies_2.h>
#include <CGAL/point_generators_2.h>

// typedefs for defining the adaptor
typedef CGAL::Exact_predicates_inexact_constructions_kernel                  K;
typedef CGAL::Delaunay_triangulation_2<K>                                    DT;
typedef CGAL::Delaunay_triangulation_adaptation_traits_2<DT>                 AT;
typedef CGAL::Delaunay_triangulation_caching_degeneracy_removal_policy_2<DT> AP;
typedef CGAL::Voronoi_diagram_2<DT,AT,AP>                                    VD;

// typedef for the result type of the point location
typedef AT::Site_2                    Site_2;
typedef AT::Point_2                   Point_2;

typedef VD::Locate_result             Locate_result;
typedef VD::Vertex_handle             Vertex_handle;
typedef VD::Face_handle               Face_handle;
typedef VD::Halfedge_handle           Halfedge_handle;
typedef VD::Ccb_halfedge_circulator   Ccb_halfedge_circulator;

int main(int argc, char **argv)
{


    if (argc != 3) {
        std::cout << "usage: voronoi <pts> <output>" << std::endl;
        exit(1);
    }

    //open point  file
    std::ifstream ptf(argv[1]);

    if (!ptf) {
        std::cout << "error: could not open points file: " << argv[1] << std::endl;
        exit(1); 
    }

    //read point count
    int pt_count;
    ptf >> pt_count;

    if (pt_count < 0) {
        std::cout << "error: invalid point count " << pt_count << std::endl;
        exit(1);
    }

    //read points and insert into voronoi diagram
    VD vd; 

    double x, y;
    double x1 = std::numeric_limits<double>::max(), x2 = -std::numeric_limits<double>::max();
    double y1 = std::numeric_limits<double>::max(), y2 = -std::numeric_limits<double>::max();
    for (int i = 0; i < pt_count; ++i) { 
        char c;
        ptf >> x; 
        ptf >> c;
        ptf >> y;
        vd.insert(Point_2(x, y));

        //set up bounds
        if (x < x1) x1 = x;
        if (x > x2) x2 = x;
        if (y < y1) y1 = y;
        if (y > y2) y2 = y; 
    }

    ptf.close();

    //output to postscript 
    std::ofstream of(argv[2]);

    of << "%\n";

    //define point function for later
    of << "/draw-point {\n";
    of << "    /y exch def\n";
    of << "    /x exch def\n";
    of << "    gsave\n";
    of << "    newpath\n";
    of << "    0.5 0.5 0.7 setrgbcolor\n";
    of << "    x y 2 0 360 arc\n";
    of << "    closepath\n";
    of << "    fill\n";
    of << "    newpath\n";
    of << "    0.4 setgray\n";
    of << "    x y 2 0 360 arc\n";
    of << "    closepath\n";
    of << "    stroke\n";
    of << "    grestore\n";
    of << "} def\n";

    //line
    of << "/draw-line {\n";
    of << "    /y2 exch def\n";
    of << "    /x2 exch def\n";
    of << "    /y1 exch def\n";
    of << "    /x1 exch def\n";
    of << "    gsave\n";
    of << "    0.7 setgray\n";
    of << "    newpath\n";
    of << "    x1 y1 moveto\n";
    of << "    x2 y2 lineto\n";
    of << "    closepath\n";
    of << "    stroke \n";
    of << "    grestore\n";
    of << "} def\n";

    for (VD::Edge_iterator itor = vd.edges_begin(); itor != vd.edges_end(); ++itor) {
        if (itor->has_source() && itor->has_target()) {
            of << itor->source()->point().x() << " " << itor->source()->point().y() << " ";
            of << itor->target()->point().x() << " " << itor->target()->point().y() << " draw-line\n";
        }
    }

    for (VD::Site_iterator itor = vd.sites_begin(); itor != vd.sites_end(); ++itor) {
        of << itor->x() << " " << itor->y() << " draw-point\n";
    }

    of.close();

    return 0;
}

