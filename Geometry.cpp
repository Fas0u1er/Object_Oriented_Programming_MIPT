#include <iostream>
#include <vector>
#include <cmath>

const double PI = 3.14159265359;
const double EPS = 0.0000001;
const double INF = 10000000000;

bool equal(double x, double y) {
    return (x - y) < EPS && (y - x) < EPS;
}

class Line;

struct Point {
    double x;
    double y;

    Point() = default;

    Point(double x, double y) : x(x), y(y) {}

    double length() const {
        return std::sqrt(x * x + y * y);
    }

    Point& rotate(double phi) {
        return *this = Point(x * std::cos(phi) - y * std::sin(phi),
                             x * std::sin(phi) + y * std::cos(phi));
    }

    Point& rotate(double phi, const Point& center);

    Point& reflect(const Point& center);

    Point& reflect(const Line& axis);

    Point& scale(const Point& center, double coefficient);
};

Point& operator+=(Point& p1, const Point& p2) {
    p1.x += p2.x;
    p1.y += p2.y;
    return p1;
}

Point& operator-=(Point& p1, const Point& p2) {
    p1.x -= p2.x;
    p1.y -= p2.y;
    return p1;
}

Point& operator*=(Point& p1, double k) {
    p1.x *= k;
    p1.y *= k;
    return p1;
}

Point& operator/=(Point& p1, double k) {
    p1.x /= k;
    p1.y /= k;
    return p1;
}

Point operator+(const Point& p1, const Point& p2) {
    Point result(p1);
    result += p2;
    return result;
}

Point operator-(const Point& p1, const Point& p2) {
    Point result(p1);
    result -= p2;
    return result;
}

Point operator*(const Point& p1, double k) {
    Point result(p1);
    result *= k;
    return result;
}

Point operator*(double k, const Point& p1) {
    Point result(p1);
    result *= k;
    return result;
}

Point operator/(const Point& p1, double k) {
    Point result(p1);
    result /= k;
    return result;
}

double vector_prod(const Point& v, const Point& u) {
    return v.x * u.y - u.x * v.y;
}

double scalar_prod(const Point& v, const Point& u) {
    return v.x * u.x + v.y * u.y;
}

bool operator==(const Point& p1, const Point& p2) {
    return equal(p1.x, p2.x) && equal(p1.y, p2.y);
}

bool operator!=(const Point& p1, const Point& p2) {
    return !(p1 == p2);
}

double get_angle(const Point& p1, const Point& p2) {
    //aka sin/cos:
    return atan2(vector_prod(p1, p2), scalar_prod(p1, p2));
}

class Line {
//private:
public:
    //Ax + By + C = 0
    double A;
    double B;
    double C;
public:
    Line(const Point& a, const Point& b) {
        A = a.y - b.y;
        B = b.x - a.x;
        C = a.x * b.y - b.x * a.y;
        if (!equal(A, 0)) {
            B /= A;
            C /= A;
            A = 1.0;
        } else {
            C /= B;
            B = 1.0;
        }
    }

    Line(const Point& p, double k) : Line(p, Point(p.x + 1.0, p.y + k)) {}

    Line(double k, double b) : Line(Point(0.0, b), Point(1.0, k + b)) {}

    bool operator==(const Line& that) const {
        return equal(that.A, A) && equal(that.B, B) && equal(that.C, C);
    }

    bool operator!=(const Line& that) const {
        return !(*this == that);
    }

    Point intersect(const Line& that) const {
        return Point{(that.C * B - C * that.B), (that.A * C - A * that.C)} / (A * that.B - that.A * B);
    }

    double coef() const {
        if (equal(B, 0)) {
            return INF;
        }
        return -(A / B);
    }

    double normal_coef() const {
        if (equal(A, 0)) {
            return INF;
        }
        return (B / A);
    }
};

Point& Point::rotate(double phi, const Point& center) {
    Point tmp = (*this - center);
    tmp.rotate(phi);
    return (*this) = center + tmp;
}

Point& Point::reflect(const Point& center) {
    return rotate(PI, center);
}

Point& Point::reflect(const Line& axis) {
    Line normal(*this, axis.normal_coef());
    return reflect(normal.intersect(axis));
}

Point& Point::scale(const Point& center, double coefficient) {
    return *this = center + coefficient * (*this - center);
}

class Shape {
public:
    virtual double perimeter() const = 0;

    virtual double area() const = 0;

    virtual bool operator==(const Shape& another) const = 0;

    bool operator!=(const Shape& another) const {
        return !(*this == another);
    }

    virtual bool isCongruentTo(const Shape& another) const = 0;

    virtual bool isSimilarTo(const Shape& another) const = 0;

    virtual bool containsPoint(const Point& point) const = 0;

    virtual void rotate(const Point& center, double angle) = 0;

    virtual void reflect(const Point& center) = 0;

    virtual void reflect(const Line& axis) = 0;

    virtual void scale(const Point& center, double coefficient) = 0;

    virtual ~Shape() = default;
};

class Polygon : public Shape {
protected:
    std::vector<Point> vertices;
private:
    Polygon parameters() const {
        //return Polygon as a cyclic list of pair(side_length_sum, angle)
        size_t n = verticesCount();
        std::vector<Point> result(n);
        for (size_t i = 0; i < n; ++i) {
            result[i] = Point((vertices[(i + 1) % n] - vertices[i]).length() + (vertices[(i + 2) % n] - vertices[(i + 1) % n]).length(),
                              std::abs(get_angle(vertices[(i + 1) % n] - vertices[i],
                                        vertices[(i + 2) % n] - vertices[(i + 1) % n])));
        }
        return Polygon(result);
    }
public:
    Polygon() = default;

    explicit Polygon(const std::vector<Point>& points) : vertices(points) {}

    explicit Polygon(const std::initializer_list<Point>& points) : vertices(points) {}

    template<class... Args>
    Polygon(const Point& p, Args... args) : Polygon(args...) {
        vertices.push_back(p);
    }

    Polygon(const Point& p) {
        vertices.push_back(p);
    }

    size_t verticesCount() const {
        return vertices.size();
    }

    std::vector<Point> getVertices() const {
        return vertices;
    }

    bool isConvex() const {
        size_t n = verticesCount();
        bool rotating_side = vector_prod(vertices[1] - vertices[0], vertices[0] - vertices[n - 1]) > 0;
        for (size_t i = 1; i < n - 1; ++i) {
            if ((vector_prod(vertices[i + 1] - vertices[i], vertices[i] - vertices[i - 1]) > 0) ^ rotating_side)
                return false;
        }
        return !((vector_prod(vertices[0] - vertices[n - 1], vertices[n - 1] - vertices[n - 2]) > 0) ^ rotating_side);
    }

    double perimeter() const override {
        size_t n = verticesCount();
        double result = (vertices[n - 1] - vertices[0]).length();
        for (size_t i = 1; i < n; ++i) {
            result += (vertices[i] - vertices[i - 1]).length();
        }
        return result;
    }

    double area() const override {
        size_t n = verticesCount();
        double result = vector_prod(vertices[n - 1], vertices[0]);
        for (size_t i = 0; i < n - 1; ++i) {
            result += vector_prod(vertices[i], vertices[i + 1]);
        }
        return std::abs(result) / 2.0;
    }

    bool operator==(const Shape& another) const override {
        const Polygon* that = dynamic_cast<const Polygon*>(&another);
        if (that == nullptr)
            return false;
        size_t n = verticesCount();
        if (that->verticesCount() != n)
            return false;
        size_t shift = 0;
        for (; shift < n; ++shift) {
            if (vertices[0] == that->vertices[shift])
                break;
            if (shift == n - 1)
                return false;
        }

        //same order check
        for (size_t i = 0; i < n; ++i) {
            if (vertices[i] != that->vertices[(shift + i) % n])
                break;
            if (i == n - 1)
                return true;
        }

        //reversed order check
        for (size_t i = 0; i < n; ++i) {
            if (vertices[i] != that->vertices[(shift + n - i) % n])
                break;
            if (i == n - 1)
                return true;
        }

        return false;
    }

    bool isCongruentTo(const Shape& another) const override {
        const Polygon* that = dynamic_cast<const Polygon*>(&another);
        if (that == nullptr)
            return false;
        return parameters() == that->parameters();
    }

    bool isSimilarTo(const Shape& another) const override {
        const Polygon* that = dynamic_cast<const Polygon*>(&another);
        if (that == nullptr)
            return false;

        Polygon scaled_copy (*that);
        scaled_copy.scale(Point(0.0, 0.0), perimeter() / that->perimeter());
        return isCongruentTo(scaled_copy);
    }

    bool containsPoint(const Point& point) const override {
        size_t n = verticesCount();
        double angle_sum = 0.0;
        for (size_t i = 0; i < n; ++i) {
            angle_sum += get_angle(vertices[i] - point, vertices[(i + 1) % n] - point);
        }
        return !(equal(angle_sum, 0.0));
    }

    void rotate(const Point& center, double angle) override {
        angle *= PI / 180;
        for (size_t i = 0; i < verticesCount(); ++i) {
            vertices[i].rotate(angle, center);
        }
    }

    void reflect(const Point& center) override {
        for (size_t i = 0; i < verticesCount(); ++i) {
            vertices[i].reflect(center);
        }
    }

    void reflect(const Line& axis) override {
        for (size_t i = 0; i < verticesCount(); ++i) {
            vertices[i].reflect(axis);
        }
    }

    void scale(const Point& center, double coefficient) override {
        for (size_t i = 0; i < verticesCount(); ++i) {
            vertices[i].scale(center, coefficient);
        }
    }
};

class Ellipse : public Shape {
protected:
    Point focus;
    Point _center;
    double major_axis;

    double a() const {
        return major_axis / 2;
    }

    double b() const {
        return std::sqrt(a() * a() - c() * c());
    }

    double c() const {
        return (_center - focus).length();
    }

public:
    Ellipse(const Point& f1, const Point& f2, double major_axis) :
            focus(f1), _center((f1 + f2) / 2.0), major_axis(major_axis) {}

    std::pair<Point, Point> focuses() const {
        return {focus, 2 * _center - focus};
    }

    std::pair<Line, Line> directrices() const {
        Line minor_axis_line(_center, _center + (focus - _center).rotate(PI / 2));
        return {Line(_center + (focus - _center) * a() / (c() * eccentricity()), minor_axis_line.coef()),
                Line(_center - (focus - _center) * a() / (c() * eccentricity()), minor_axis_line.coef())};
    }

    double eccentricity() const {
        return c() / a();
    }

    Point center() const {
        return _center;
    }

    double perimeter() const override {
        return PI * (3 * (a() + b()) - std::sqrt((3 * a() + b()) * (a() + 3 * b())));
    }

    double area() const override {
        return PI * a() * b();
    }

    bool operator==(const Shape& another) const override {
        const Ellipse* that = dynamic_cast<const Ellipse*>(&another);
        if (that == nullptr)
            return false;
        return equal(major_axis, that->major_axis) && (_center == that->_center) &&
               (that->focus == focuses().first || that->focus == focuses().second);
    }

    bool isCongruentTo(const Shape& another) const override {
        const Ellipse* that = dynamic_cast<const Ellipse*>(&another);
        if (that == nullptr)
            return false;
        return equal(a(), that->a()) && equal(b(), that->b());
    }

    bool isSimilarTo(const Shape& another) const override {
        const Ellipse* that = dynamic_cast<const Ellipse*>(&another);
        if (that == nullptr)
            return false;
        return equal(eccentricity(), that->eccentricity());
    }

    bool containsPoint(const Point& point) const override {
        return major_axis + EPS > ((point - focuses().first).length() +
                                   (point - focuses().second).length());
    }

    void rotate(const Point& center, double angle) override {
        focus.rotate(angle, center);
        _center.rotate(angle, center);
    }

    void reflect(const Point& center) override {
        focus.reflect(center);
        _center.reflect(center);
    }

    void reflect(const Line& axis) override {
        focus.reflect(axis);
        _center.reflect(axis);
    }

    void scale(const Point& center, double coefficient) override {
        focus.scale(center, coefficient);
        _center.scale(center, coefficient);
        major_axis *= std::abs(coefficient);
    }
};

class Circle : public Ellipse {
public:
    Circle(const Point& o, double r) : Ellipse(o, o, 2.0 * r) {}

    double radius() const {
        return major_axis / 2.0;
    }
};

class Rectangle : public Polygon {
public:
    Rectangle(const Point& p1, const Point& p3, double k) {
        if (k > 1.0)
            k = 1.0 / k;
        Point p2 = (p1 + p3) / 2 + ((p1 - p3) / 2).rotate(-2 * std::atan(k));
        Point p4 = p1 + p3 - p2;
        vertices = {p1, p2, p3, p4};
    }

    Point center() const {
        return (vertices[0] + vertices[2]) / 2.0;
    }

    std::pair<Line, Line> diagonals() const {
        return {Line(vertices[0], vertices[2]), Line(vertices[1], vertices[3])};
    }
};

class Square : public Rectangle {
public:
    Square(const Point& A, const Point& C) : Rectangle(A, C, 1.0) {}

    Circle circumscribedCircle() const {
        return Circle{center(), (vertices[0] - vertices[2]).length() / 2.0};
    }

    Circle inscribedCircle() {
        return Circle{center(), (vertices[0] - vertices[1]).length() / 2.0};
    }
};

class Triangle : public Polygon {
public:
    Triangle(const Point& A, const Point& B, const Point& C) : Polygon(A, B, C) {}

    Circle circumscribedCircle() const {
        Point center = orthocenter() + (centroid() - orthocenter()) * 1.5;
        return {center, (vertices[0] - center).length()};
    }

    Circle inscribedCircle() const {
        double len0 = (vertices[2] - vertices[1]).length();
        double len1 = (vertices[0] - vertices[2]).length();
        double len2 = (vertices[1] - vertices[0]).length();

        Point bissector_base_0 = (len2 * vertices[2] + len1 * vertices[1]) / (len1 + len2);
        Point bissector_base_1 = (len2 * vertices[2] + len0 * vertices[0]) / (len0 + len2);
        Point center = Line(vertices[1], bissector_base_1).intersect
                (Line(vertices[0], bissector_base_0));
        return Circle(center, area() / perimeter() * 2.0);
    }

    Point centroid() const {
        return (vertices[0] + vertices[1] + vertices[2]) / 3.0;
    }

    Point orthocenter() const {
        Line side_0(vertices[2], vertices[1]);
        Line side_1(vertices[2], vertices[0]);
        return Line(vertices[0], side_0.normal_coef()).intersect
                (Line(vertices[1], side_1.normal_coef()));
    }

    Line EulerLine() const {
        return Line(orthocenter(), centroid());
    }

    Circle ninePointsCircle() const {
        Point mid0 = (vertices[2] + vertices[1]) / 2.0;
        Point mid1 = (vertices[0] + vertices[2]) / 2.0;
        Point mid2 = (vertices[0] + vertices[1]) / 2.0;
        return Triangle(mid0, mid1, mid2).circumscribedCircle();
    }
};
