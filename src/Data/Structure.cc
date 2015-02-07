#include <Data/Structure.h>
#include <Data/Json.h>
#include <Data/PointSampler.h>
#include <Data/AlphaShape.h>

#include <vtkCenterOfMass.h>
#include <vtkMassProperties.h>
#include <vtkWindowedSincPolyDataFilter.h>
#include <vtkCleanPolyData.h>
#include <vtkSTLWriter.h>
#include <vtkFeatureEdges.h>
#include <vtkCellArray.h>
#include <vtkFillHolesFilter.h>
#include <vtkPolyDataNormals.h>

#include <vtkMath.h>

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
    n.graph_id = -1;

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

  std::cerr << "Found " << structure->node_map_.size() << " nodes\n";

  foreach( QVariant var, link_list ) {
    Link link;
    QMap<QString, QVariant> item = var.toMap();

    link.a = item["A"].toLongLong();
    link.b = item["B"].toLongLong();

    if ( structure->node_map_.find( link.a ) == structure->node_map_.end()
         || structure->node_map_.find( link.b ) == structure->node_map_.end() )
    {
      continue;
    }

    structure->node_map_[link.a].linked_nodes.append( link.b );
    structure->node_map_[link.b].linked_nodes.append( link.a );
    structure->links_.append( link );
  }

  std::cerr << "Found " << structure->links_.size() << " links\n";

  // identify all subgraphs

  long id = 0;

  for ( NodeMap::iterator it = structure->node_map_.begin(); it != structure->node_map_.end(); ++it )
  {
    Node n = it->second;

    if ( n.graph_id == -1 )
    {
      id++;
      n.graph_id = id;
      structure->node_map_[it->first] = n;

      QList<int> connections = n.linked_nodes;

      while ( connections.size() > 0 )
      {
        int node = connections.first();
        connections.pop_front();

        Node child = structure->node_map_[node];

        if ( child.graph_id == -1 )
        {
          child.graph_id = id;
          connections.append( child.linked_nodes );
          structure->node_map_[node] = child;  // write back
        }
      }
    }
  }

  std::cerr << "Found " << id << " graphs\n";

  // create links between graphs

  QList<int> primary_group;

  for ( NodeMap::iterator it = structure->node_map_.begin(); it != structure->node_map_.end(); ++it )
  {
    Node n = it->second;
    if ( n.graph_id == 1 )
    {
      primary_group.append( n.id );
    }
  }

  for ( int i = 2; i <= id; i++ )
  {

    // find closest pair
    double min_dist = DBL_MAX;
    int primary_id = -1;
    int child_id = -1;

    for ( NodeMap::iterator it = structure->node_map_.begin(); it != structure->node_map_.end(); ++it )
    {
      Node n = it->second;

      if ( n.graph_id == i )
      {

        for ( NodeMap::iterator it2 = structure->node_map_.begin(); it2 != structure->node_map_.end(); ++it2 )
        {
          Node pn = it2->second;
          if ( pn.graph_id >= i )
          {
            continue;
          }

          double point1[3], point2[3];
          point1[0] = n.x;
          point1[1] = n.y;
          point1[2] = n.z;
          point2[0] = pn.x;
          point2[1] = pn.y;
          point2[2] = pn.z;
          double distance = sqrt( vtkMath::Distance2BetweenPoints( point1, point2 ) );

          if ( distance < min_dist )
          {
            min_dist = distance;
            primary_id = pn.id;
            child_id = n.id;
          }
        }
      }
    }

    Link new_link;
    new_link.a = primary_id;
    new_link.b = child_id;
    structure->links_.append( new_link );
    //std::cerr << "added new link\n";
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




    // clean
    vtkSmartPointer<vtkCleanPolyData> clean = vtkSmartPointer<vtkCleanPolyData>::New();
    clean->SetInputData( poly_data );
    clean->Update();
    poly_data = clean->GetOutput();



    vtkSmartPointer<vtkFeatureEdges> features = vtkSmartPointer<vtkFeatureEdges>::New();
    features->SetInputData( poly_data );
    features->NonManifoldEdgesOn();
    features->BoundaryEdgesOff();
    features->FeatureEdgesOff();
    features->Update();

    vtkSmartPointer<vtkPolyData> nonmanifold = features->GetOutput();

    std::cerr << "Number of non-manifold points: " << nonmanifold->GetNumberOfPoints() << "\n";
    std::cerr << "Number of non-manifold cells: " << nonmanifold->GetNumberOfCells() << "\n";

    std::vector<int> remove;

    for ( int j = 0; j < poly_data->GetNumberOfPoints(); j++ )
    {
      double p2[3];
      poly_data->GetPoint( j, p2 );

      for ( int i = 0; i < nonmanifold->GetNumberOfPoints(); i++ )
      {
        double p[3];
        nonmanifold->GetPoint( i, p );

        if ( p[0] == p2[0] && p[1] == p2[1] && p[2] == p2[2] )
        {
          remove.push_back( j );
        }
      }
    }

    std::cerr << "Removing " << remove.size() << " non-manifold vertices\n";

    vtkSmartPointer<vtkPolyData> new_poly_data = vtkSmartPointer<vtkPolyData>::New();
    vtkSmartPointer<vtkPoints> vtk_pts = vtkSmartPointer<vtkPoints>::New();
    vtkSmartPointer<vtkCellArray> vtk_triangles = vtkSmartPointer<vtkCellArray>::New();

    for ( int i = 0; i < poly_data->GetNumberOfCells(); i++ )
    {
      vtkSmartPointer<vtkIdList> list = vtkIdList::New();
      poly_data->GetCellPoints( i, list );

      bool match = false;
      for ( int j = 0; j < list->GetNumberOfIds(); j++ )
      {
        int id = list->GetId( j );
        for ( int k = 0; k < remove.size(); k++ )
        {
          if ( id == remove[k] )
          {
            match = true;
          }
        }
      }

      if ( match )
      {
        poly_data->DeleteCell( i );
      }
    }

    poly_data->RemoveDeletedCells();

    features = vtkSmartPointer<vtkFeatureEdges>::New();
    features->SetInputData( poly_data );
    features->NonManifoldEdgesOn();
    features->BoundaryEdgesOff();
    features->FeatureEdgesOff();
    features->Update();

    nonmanifold = features->GetOutput();

    std::cerr << "Number of non-manifold points: " << nonmanifold->GetNumberOfPoints() << "\n";
    std::cerr << "Number of non-manifold cells: " << nonmanifold->GetNumberOfCells() << "\n";


    // fill holes
    vtkSmartPointer<vtkFillHolesFilter> fill_holes = vtkSmartPointer<vtkFillHolesFilter>::New();
    fill_holes->SetInputData( poly_data );
    //
    //fill_holes->SetHoleSize( 300000 );
    fill_holes->Update();
    poly_data = fill_holes->GetOutput();


    // Make the triangle windong order consistent
    vtkSmartPointer<vtkPolyDataNormals> normals =
      vtkSmartPointer<vtkPolyDataNormals>::New();
    normals->SetInputData(poly_data);
    normals->ConsistencyOn();
    normals->SplittingOff();
    normals->Update();
    poly_data = normals->GetOutput();


    vtkSmartPointer<vtkSTLWriter> writer = vtkSmartPointer<vtkSTLWriter>::New();
    writer->SetFileName( "Z:\\shared\\file.stl" );
    writer->SetInputData( poly_data );
    writer->Write();

/*
    // clean
    vtkSmartPointer<vtkCleanPolyData> clean = vtkSmartPointer<vtkCleanPolyData>::New();
    clean->SetInputData( poly_data );
    clean->Update();
    poly_data = clean->GetOutput();
*/
/*
    // smooth
    vtkSmartPointer<vtkWindowedSincPolyDataFilter> smooth = vtkSmartPointer<vtkWindowedSincPolyDataFilter>::New();
    smooth->SetInputData( poly_data );
    smooth->SetPassBand( 0.05 );
    smooth->SetNumberOfIterations( 20 );
    smooth->Update();
    poly_data = smooth->GetOutput();
*/

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
