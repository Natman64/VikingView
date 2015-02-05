#ifndef VIKING_DATA_ALPHASHAPE_H
#define VIKING_DATA_ALPHASHAPE_H

#include <vtkSmartPointer.h>

#include <CGAL/Exact_predicates_inexact_constructions_kernel.h>
typedef CGAL::Exact_predicates_inexact_constructions_kernel K;
typedef K::Point_3 Point;

class vtkPolyData;

class AlphaShape
{

public:
  AlphaShape();
  ~AlphaShape();

  void set_points( std::list<Point> points );

  vtkSmartPointer<vtkPolyData> get_mesh();

private:

  std::list <Point> points_;
};

#endif /* VIKING_DATA_ALPHASHAPE_H */
