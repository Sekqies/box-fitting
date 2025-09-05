#include <vector>
#include <cmath>
#include <algorithm>
#include <numeric>
#include <set>

using namespace std;
const double PI = acos(-1);

constexpr double EPSILON = 1e-9;

constexpr bool zero(double v){
    return fabs(v) < EPSILON;
}
constexpr double value(double v){
    if(v==-0.0) return 0.0;
    return zero(v)? 0.0 : v;
}

class Point {
public:
    double x, y;
    Point() = default;
    Point(double x, double y) {
        this->x = x;
        this->y = y;
    }
    Point operator+(const Point& a) const {
        return Point(this->x + a.x, this->y + a.y);
    }
    Point operator-(const Point& a) const {
        return Point(this->x - a.x, this->y - a.y);
    }
    Point operator*(double scalar) const {
        return Point(this->x * scalar, this->y * scalar);
    }
    bool operator<(const Point& other) const {
        double x1 = value(x);
        double y1 = value(y);
        double x2 = value(other.x);
        double y2 = value(other.y);
        if (!zero(fabs(x1-x2))) return x1 < x2;
        return y1 < y2;
    }
};



class Square {
public:
    Point c;
    double t;
    double l = 1.0;
    Square(Point c,double t,double l){
        this->c = c;
        this->t = t;
        this->l = l;
    }
    Square() = default;

    vector<Point> getVertices() const {
        double half_l = l / 2.0;
        vector<Point> corners = {
            Point(-half_l, -half_l),
            Point( half_l, -half_l),
            Point( half_l,  half_l),
            Point(-half_l,  half_l)
        };
        vector<Point> vertices(4);
        size_t i = 0;
        for (const auto& corner : corners) {
            double x_rot = value(corner.x * cos(t) - corner.y * sin(t));
            double y_rot = value(corner.x * sin(t) + corner.y * cos(t));
            vertices[i++] = (Point(c.x + x_rot, c.y + y_rot));
        }
        return vertices;
    }
};

bool pointInSquare(const Point& p, const Square& sq) {
    double dx = p.x - sq.c.x;
    double dy = p.y - sq.c.y;
    double cos_t = cos(-sq.t);
    double sin_t = sin(-sq.t);
    double local_x = dx * cos_t - dy * sin_t;
    double local_y = dx * sin_t + dy * cos_t;
    double half_l = sq.l / 2.0;
    return (abs(local_x) <= half_l + EPSILON) && (abs(local_y) <= half_l + EPSILON);
}

bool segmentIntersect(const Point& p1, const Point& p2, const Point& q1, const Point& q2, Point& intersection) {
    double A1 = p2.y - p1.y;
    double B1 = p1.x - p2.x;
    double C1 = A1 * p1.x + B1 * p1.y;
    double A2 = q2.y - q1.y;
    double B2 = q1.x - q2.x;
    double C2 = A2 * q1.x + B2 * q1.y;
    double det = A1 * B2 - A2 * B1;
    if (fabs(det) < EPSILON) return false; 
    double x = (B2 * C1 - B1 * C2) / det;
    double y = (A1 * C2 - A2 * C1) / det;
    auto between = [](double a, double b, double c) {
        return min(a, b) - EPSILON <= c && c <= max(a, b) + EPSILON;
    };
    if (between(p1.x, p2.x, x) && between(p1.y, p2.y, y) &&
        between(q1.x, q2.x, x) && between(q1.y, q2.y, y)) {
        intersection = Point(x, y);
        return true;
    }
    return false;
}

vector<Point> getSquareIntersections(const Square& sq1, const Square& sq2) {
    vector<Point> v1 = sq1.getVertices();
    vector<Point> v2 = sq2.getVertices();
    std::vector<Point> intersectionPoints;
    for (int i = 0; i < 4; ++i) {
        Point p1 = v1[i], p2 = v1[(i+1)%4];
        for (int j = 0; j < 4; ++j) {
            Point q1 = v2[j], q2 = v2[(j+1)%4];
            Point inter;
            if (segmentIntersect(p1, p2, q1, q2, inter)) {
                intersectionPoints.push_back(Point(value(inter.x),value(inter.y)));
            }
        }
    }
    for (Point& pt : v1) {
        if (!pointInSquare(pt, sq2)) continue; 
        pt.x = value(pt.x);
        pt.y = value(pt.y);
        intersectionPoints.push_back(pt);
    }
    for (Point& pt : v2) {
        if (!pointInSquare(pt, sq1)) continue; 
        pt.x = value(pt.x);
        pt.y = value(pt.y);
        intersectionPoints.push_back(pt);
    }
    
    return intersectionPoints;
}

double areaOfPolygon(vector<Point>& pts){
    double sum = 0.0;
    const size_t n = pts.size();
    for(size_t i=0;i<n;i++){
        Point a = pts[i];
        Point b = pts[(i+1)%n];
        sum+= a.x * b.y - b.x * a.y;
    }
    return value(fabs(sum/2.0));
}

bool squareContainedIn(const Square& inner, const Square& outer) {
    auto vertices = inner.getVertices();
    for (const auto& v : vertices) {
        if (!pointInSquare(v, outer)) return false;
    }
    return true;
}
double areaOfSquareIntersections(const Square& sq1, const Square& sq2){
    if(sq1.l > sq2.l){
        if(squareContainedIn(sq2,sq1)) return sq2.l;
    }
    else if(squareContainedIn(sq1,sq2)) return sq1.l;
    vector<Point> vertices = getSquareIntersections(sq1,sq2);
    return areaOfPolygon(vertices);
}
