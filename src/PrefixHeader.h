#include <iostream>

#include <QtGui>
#include <QtCore>
#include <QMainWindow>
#include <QActionGroup>
#include <QFileDialog>
#include <QVector>
#include <QSharedPointer>
#include <QString>

#include <vtkSmartPointer.h>
#include <vtkRenderer.h>
#include <vtkMarchingCubes.h>
#include <vtkPolyDataMapper.h>
#include <vtkActor.h>
#include <vtkSphereSource.h>
#include <vtkGlyph3D.h>
#include <vtkProperty.h>
#include <vtkLookupTable.h>
#include <vtkUnsignedLongArray.h>
