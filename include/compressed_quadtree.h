/*
Copyright (c) 2010 Daniel Minor 

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

#ifndef COMPRESSED_QUADTREE_H_
#define COMPRESSED_QUADTREE_H_

/*
    Compressed Quadtree implementation supporting approximate nearest neighbour
    queries, based upon the description in:  
    
    Eppstein, D., Goodrich, M. T., Sun, J. Z. (2008) The Skip Quadtree:
    A Simple Dynamic Data Structure for Multidimensional Data,
    Int. Journal on Computational Geometry and Applications, 18(1/2), pp. 131 - 160
*/

#include <algorithm>
#include <limits>
#include <list>
#include <vector>
#include <iostream>
#include <cstring>

template<class Point> class CompressedQuadtree {

    public:

        //Nodes of the quadtree
        struct Node { 
            Node **nodes;       //children
            Point mid;          //midpoint
            double radius;      //half side length
            Point *pt;          //point, if data stored

            Node()
                : nodes(0), pt(0)
            {

            }

            bool in_node(const Point &pt, size_t dim)
            {

                const double locate_eps = 0.001;

                bool in = true;

                for (size_t d = 0; d < dim; ++d) { 
                    if (mid[d] - radius - pt[d] > locate_eps
                        || pt[d] - mid[d] - radius > locate_eps) { 
                        in = false; 
                        break; 
                    } 
                } 

                return in; 
            } 
        };

        //Keep track of point and priority for nearest neighbour search
        struct NodeDistance {

            Node *node;
            double distance;

            NodeDistance(Node *node, double distance) : node(node), distance(distance) {};

            //min-heap
            bool operator<(const NodeDistance &other) const
            {
                return distance > other.distance;
            }
        }; 

        struct EndBuildFn {
            virtual bool operator()(Node *, size_t depth)
            {
                return false;
            }
        }; 

        CompressedQuadtree(size_t dim, Point *pts, size_t n, double *range, EndBuildFn &fn)
            : dim(dim)
            , nnodes(1 << dim)
        { 

            //calculate mid point and half side length
            Point mid, side; 
            double radius = 0;
            for (size_t d = 0; d < dim; ++d) {
                mid[d] = (range[d*dim]+range[d*dim + 1]) / 2;
                double side = (range[d*dim + 1]-range[d*dim]) / 2;
                if (side > radius) radius = side;
            } 

            //set up points vector 
            std::vector<Point *> pts_vector;
            for (size_t i = 0; i < n; ++i) {
                pts_vector.push_back(&pts[i]);
            }

            root = worker(mid, radius, pts_vector, fn, 0);
        }

        virtual ~CompressedQuadtree()
        {
           if (root) delete_worker(root);  
        }

        std::list<std::pair<Point *, double> > knn(size_t k, const Point &pt, double eps) 
        {
            //setup query result vector
            std::list<std::pair<Point *, double> > qr; 
 
            //initialize priority queue for search
            std::vector<NodeDistance > pq; 
            pq.push_back(NodeDistance(root, 0.0));

            while (!pq.empty()) {

                std::pop_heap(pq.begin(), pq.end());
                Node *node = pq.back().node;
                double node_dist = pq.back().distance; 
                pq.pop_back();

                if (node->nodes == 0) { 
                    //calculate distance from query point to this point
                    double dist = 0.0; 
                    for (size_t d = 0; d < dim; ++d) {
                        dist += ((*node->pt)[d]-pt[d]) * ((*node->pt)[d]-pt[d]); 
                    }

                    //insert point in result
                    typename std::list<std::pair<Point *, double> >::iterator itor = qr.begin();
                    while (itor != qr.end() && itor->second < dist) {
                        ++itor;
                    }

                    qr.insert(itor, std::make_pair<Point *, double>(node->pt, dist)); 

                    if (qr.size() > k) qr.pop_back();

                } else {

                    //find k-th distance
                    double kth_dist = qr.size() < k? std::numeric_limits<double>::max() : qr.back().second;

                    //stop searching, all further nodes farther away than k-th value
                    if (kth_dist <= (1.0 + eps)*node_dist) {
                        break;
                    }

                    for (size_t n = 0; n < nnodes; ++n) { 
                        //calculate distance to each of the non-zero children
                        //if less than k-th distance, then visit 
                        if (node->nodes[n]) {

                            double min_dist = min_pt_dist_to_node(pt, node->nodes[n]);

                            //if closer than k-th distance, search
                            if (min_dist < kth_dist) { 
                                pq.push_back(NodeDistance(node->nodes[n], min_dist));
                                std::push_heap(pq.begin(), pq.end()); 
                            }
                        } 
                    }
                }
            } 

            return qr;
        }

        Node *root;
        size_t dim; 

    private:

        size_t nnodes;

        Node *worker(const Point &mid, double radius, std::vector<Point *> &pts, EndBuildFn &fn, size_t depth)
        {
            Node *node = new Node; 
            for (size_t d = 0; d < dim; ++d) {
                node->mid[d] = mid[d];
            }
            node->radius = radius; 

            if (pts.size() == 1) {
                node->nodes = 0;
                node->pt = pts[0];
                fn(node, depth);
            } else if (!fn(node, depth)) { 
                node->pt = 0;
                node->nodes = new Node *[nnodes];

                //divide points between the nodes 
                std::vector<Point *> *node_pts = new std::vector<Point *>[nnodes];
                for (typename std::vector<Point *>::iterator itor = pts.begin(); itor != pts.end(); ++itor) {

                    //determine node index based upon which which side of midpoint for each dimension
                    size_t n = 0;
                    for (size_t d = 0; d < dim; ++d) {
                        if ((*(*itor))[d] > mid[d]) n += 1 << d; 
                    } 

                    node_pts[n].push_back(*itor);
                } 

                //create new nodes recursively
                size_t ninteresting = 0;
                for (size_t n = 0; n < nnodes; ++n) {

                    if (node_pts[n].size()) {

                        Point nbounds[2];
                        Point new_mid;
                        double new_radius = radius / 2.0;
                        for (size_t d = 0; d < dim; ++d) { 
                            if (n & (1 << d)) {
                                new_mid[d] = mid[d] + new_radius;
                            } else { 
                                new_mid[d] = mid[d] - new_radius;
                            }
                        }

                        ++ninteresting;
                        node->nodes[n] = worker(new_mid, new_radius, node_pts[n], fn, depth + 1);
                    } else {
                        node->nodes[n] = 0; 
                    }
                }

                delete[] node_pts;

                //compress if less than 2 interesting nodes
                if (ninteresting < 2) {
                    for (size_t n = 0; n < nnodes; ++n) {
                        if (node->nodes[n]) {
                            Node *temp = node;
                            node = node->nodes[n];
                            delete temp;
                            break;
                        }
                    }
                } 
            } 

            return node;
        } 

        double min_pt_dist_to_node(const Point &pt, Node *node)
        {
            bool inside = true; 
            double min_dist = std::numeric_limits<double>::max();
            for (size_t d = 0; d < dim; ++d) { 
        
                double dist; 
                if (pt[d] < node->mid[d] - node->radius) { 
                    dist = node->mid[d] - node->radius - pt[d];
                    inside = false;
                } else if (pt[d] > node->mid[d] + node->radius) {
                    dist = pt[d] - (node->mid[d] + node->radius); 
                    inside = false;
                }

                if (dist < min_dist) min_dist = dist; 
            } 

            if (inside) return 0.0;
            else return min_dist*min_dist;
        }

        void delete_worker(Node *node)
        {
            if (node->nodes) {
                for (size_t n = 0; n < nnodes; ++n) { 
                    if (node->nodes[n]) delete_worker(node->nodes[n]); 
                }

                delete node->nodes;
            }

            delete node; 
        }    

};

#endif

