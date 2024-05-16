#include <QAction>
#include <QApplication>
#include <QHBoxLayout>
#include <QSettings>

#include "ovalViewer.h"

/** ----------------------------------------------------------------------------
  \fn save_window_position Saves the position of a window
---------------------------------------------------------------------------- */
static void save_window_position( QWidget *window, const QString& what )
{
  QSettings prefs( QString( "Company" ), QString( "curve_fitting" ) );

  // there are two parts to the window, the size and position

  prefs.setValue( QString( "%1Position" ).arg( what ), QVariant( window->pos() ) );
  prefs.setValue( QString( "%1Size" ).arg( what ), QVariant( window->size() ) );
}
/** ----------------------------------------------------------------------------
  \fn restore_window_position Saves the position of a window
---------------------------------------------------------------------------- */
static void restore_window_position( QWidget *window, const QString& what )
{
  QSettings prefs( QString( "Company" ), QString( "curve_fitting" ) );

  QPoint pos = prefs.value( QString( "%1Position" ).arg( what ), QPoint( 10, 50 ) ).toPoint();
  QSize size = prefs.value( QString( "%1Size" ).arg( what ), QSize( 600, 300 ) ).toSize();

  window->setGeometry( QRect( pos, size ) );
}

int main( int argc, char *argv[] )
{
  QApplication app( argc, argv );

  QWidget *window = new QWidget();
  window->setWindowTitle( QString( "Window" ) );

  QAction *quitX = new QAction( "Quit", & app );
  quitX->setShortcut( QString( "Ctrl+Q" ) );

  QObject::connect( quitX, SIGNAL( triggered() ), & app, SLOT( quit() ) );

  QHBoxLayout *wl = new QHBoxLayout( window );
  ovalViewer *oview = new ovalViewer( window );
  wl->addWidget( oview );

  restore_window_position( window, QString( "window" ) );
  window->show();

  int status = 0;
  bool done = false;

  do
    {
      status = app.exec();

      if( status == 0 )
        done = window->close();
      else
        break;

    } while( not done );

  save_window_position( window, "window" );
  delete window;

  return status;
}
