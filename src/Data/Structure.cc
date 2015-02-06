#include <Data/Structure.h>
#include <Data/Json.h>
#include <Data/PointSampler.h>
#include <Data/AlphaShape.h>

#include <vtkCenterOfMass.h>
#include <vtkMassProperties.h>
#include <vtkWindowedSincPolyDataFilter.h>
#include <vtkCleanPolyData.h>

//-----------------------------------------------------------------------------
Structure::Structure()
{
  this->color_ = QColor( 128 + ( qrand() % 128 ), 128 + ( qrand() % 128 ), 128 + ( qrand() % 128 ) );
}

//-----------------------------------------------------------------------------
Structure::~Structure()
{}

//-----------------------------------------------------------------------------
QSharedPointer<Structure> Structure::create_structure( int id, QString location_text, QString link_text )
{

  QSharedPointer<Structure> structure = QSharedPointer<Structure>( new Structure() );
  structure->id_ = id;

  QMap<QString, QVariant> map = Json::decode( location_text );
  QList<QVariant> location_list = map["value"].toList();

  map = Json::decode( link_text );
  QList<QVariant> link_list = map["value"].toList();

  float units_per_pixel = 2.18 / 1000.0;
  float units_per_section = 90.0 / 1000.0;

  // construct nodes
  foreach( QVariant var, location_list ) {
    Node n;
    QMap<QString, QVariant> item = var.toMap();
    n.x = item["VolumeX"].toDouble();
    n.y = item["VolumeY"].toDouble();
    n.z = item["Z"].toDouble();
    n.radius = item["Radius"].toDouble();
    n.id = item["ID"].toLongLong();

    if ( n.z == 56 || n.z == 8 || n.z == 22 || n.z == 81 || n.z == 72 || n.z == 60 )
    {
      continue;
    }

    // scale
    n.x = n.x * units_per_pixel;
    n.y = n.y * units_per_pixel;
    n.z = n.z * units_per_section;
    n.radius = n.radius * units_per_pixel;

    structure->node_map_[n.id] = n;
  }

  foreach( QVariant var, link_list ) {
    Link l;
    QMap<QString, QVariant> item = var.toMap();

    l.a = item["A"].toLongLong();
    l.b = item["B"].toLongLong();
    structure->links_.append( l );
  }

  return structure;
}

//-----------------------------------------------------------------------------
NodeMap Structure::get_node_map()
{
  return this->node_map_;
}

//-----------------------------------------------------------------------------
vtkSmartPointer<vtkPolyData> Structure::get_mesh()
{
  if ( !this->mesh_ )
  {
    PointSampler ps( this );
    std::list<Point> points = ps.sample_points();

    AlphaShape alpha_shape;
    alpha_shape.set_points( points );

    vtkSmartPointer<vtkPolyData> poly_data = alpha_shape.get_mesh();

/*
    vtkSmartPointer<vtkCleanPolyData> clean = vtkSmartPointer<vtkCleanPolyData>::New();
    clean->SetInputData( poly_data );
    clean->Update();
    poly_data = clean->GetOutput();
 */

    vtkSmartPointer<vtkWindowedSincPolyDataFilter> smooth = vtkSmartPointer<vtkWindowedSincPolyDataFilter>::New();
    smooth->SetInputData( poly_data );
    smooth->SetPassBand( 0.15 );
    smooth->SetNumberOfIterations( 20 );
    smooth->Update();
    poly_data = smooth->GetOutput();

    this->mesh_ = poly_data;
  }

  return this->mesh_;
}

//-----------------------------------------------------------------------------
int Structure::get_id()
{
  return this->id_;
}

//-----------------------------------------------------------------------------
double Structure::get_volume()
{
  vtkSmartPointer<vtkPolyData> mesh = this->get_mesh();

  vtkSmartPointer<vtkMassProperties> mass_properties = vtkSmartPointer<vtkMassProperties>::New();

  mass_properties->SetInputData( mesh );
  mass_properties->Update();

  return mass_properties->GetVolume();
}

//-----------------------------------------------------------------------------
QString Structure::get_center_of_mass_string()
{
  vtkSmartPointer<vtkPolyData> mesh = this->get_mesh();

  // Compute the center of mass
  vtkSmartPointer<vtkCenterOfMass> center_of_mass =
    vtkSmartPointer<vtkCenterOfMass>::New();
  center_of_mass->SetInputData( mesh );
  center_of_mass->SetUseScalarsAsWeights( false );
  center_of_mass->Update();

  double center[3];
  center_of_mass->GetCenter( center );

  QString str = QString::number( center[0] ) + ", " + QString::number( center[1] ) + ", " + QString::number( center[2] );

  return str;
}

//-----------------------------------------------------------------------------
QList<Link> Structure::get_links()
{
  return this->links_;
}

//-----------------------------------------------------------------------------
void Structure::set_color( QColor color )
{
  this->color_ = color;
}

//-----------------------------------------------------------------------------
QColor Structure::get_color()
{
  return this->color_;
}
