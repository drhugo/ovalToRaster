//
// Created by Hugo Ayala on 5/16/24.
//

#include <QAction>
#include <QMouseEvent>
#include <QPainter>


#include "ovalViewer.h"

/** ---------------------------------------------------------------------------
* \fn compute_sdf
* \description compute the signed distance to an oval.  The distance is positive
*     if outside, negative if inside -- lifted from ovalRasterizer.cpp
---------------------------------------------------------------------------- */
static float compute_sdf( const ovalRecord *oval, float xx, float yy )
{
  float dx = xx - oval->centerx;
  float dy = yy - oval->centery;

  float rr;

  if( dx != 0.f or dy != 0.f )
    {
      float angle = oval->angle - atan2( dy, dx );

      float sinT = sin( angle );
      float cosT = cos( angle );
      float a2 = oval->radiusx * oval->radiusx;
      float b2 = oval->radiusy * oval->radiusy;

      float r2 = ( a2 * b2 ) / ( a2 * sinT * sinT + b2 * cosT * cosT );

      rr = hypot( dx, dy ) - sqrt( r2 );
    }
  else  // the point is in the center
    {
      rr = -std::min( oval->radiusx, oval->radiusy );
    }

  return rr;
}
/** ----------------------------------------------------------------------------
  \fn ovalViewer::ovalViewer
---------------------------------------------------------------------------- */
ovalViewer::ovalViewer( QWidget *parent ) : QWidget( parent ), cmd_( nullptr ), scale_( 10 )
{
  setFocusPolicy( Qt::StrongFocus );

  QAction *dumpRenderXn = new QAction( QString( "Dump Render" ), this );
  dumpRenderXn->setShortcut( QString( "Ctrl+d" ) );
  connect( dumpRenderXn, SIGNAL( triggered(bool) ), this, SLOT( dumpOvalRender() ) );


  QAction *clearOvalsXn = new QAction( QString( "Reset Oval" ), this );
  clearOvalsXn->setShortcut( QString( "Ctrl+r" ) );
  connect( clearOvalsXn, SIGNAL( triggered(bool) ), this, SLOT( clearOvals() ) );

  QAction *writeOvalsXn = new QAction( QString( "Save Ovals" ), this );
  writeOvalsXn->setShortcut( QString( "Ctrl+s" ) );
  connect( writeOvalsXn, SIGNAL( triggered(bool) ), this, SLOT( writeOvals() ) );

  addAction( dumpRenderXn );
  addAction( clearOvalsXn );
  addAction( writeOvalsXn );
}
/** ----------------------------------------------------------------------------
  \fn ovalViewer::renderOnePixel
---------------------------------------------------------------------------- */
void ovalViewer::renderOnePixel( const QPoint& where )
{
  float xpos = floor( where.x() / scale_ );
  float ypos = floor( where.y() / scale_ );

  std::vector< ovalRecord > shifted;
  for( const auto& one : ovalList_ )
    {
      shifted.push_back( { one.centerx - xpos, one.centery - ypos, one.radiusx, one.radiusy, one.angle } );
    }

  auto rr = ovalListToRaster( shifted, 1, 1 );

  if( rr.size() == 1 )
    {
      msg_ = QString( "ypos: %1, startx: %2, endx: %3, value: %4" )
          .arg( ypos + rr[ 0 ].lineY )
          .arg( xpos + rr[ 0 ].startX )
          .arg( xpos + rr[ 0 ].endX )
          .arg( rr[ 0 ].value );
    }
  else
    {
      msg_ = QString( "Returned %1 runs" ).arg( rr.size() );
    }

  update();
}
/** ----------------------------------------------------------------------------
  \fn ovalViewer::dumpOvalRender
---------------------------------------------------------------------------- */
void ovalViewer::dumpOvalRender()
{
  if( not ovalList_.empty() )
    {
      int ww = width() / scale_;
      int hh = height() / scale_;

      auto rr = ovalListToRaster( ovalList_, ww, hh );

      int lastY = -1;

      for( const auto& one : rr )
        {
          if( one.lineY != lastY )
            {
              fprintf( stderr, "\n%d: ", one.lineY );
              lastY = one.lineY;
            }

          fprintf( stderr, "[%d-%d (%.2f)]", one.startX, one.endX, one.value );
        }

      fprintf( stderr, "\n" );
    }
}
/** ----------------------------------------------------------------------------
  \fn ovalViewer::writeOvals
---------------------------------------------------------------------------- */
void ovalViewer::writeOvals()
{
  if( not ovalList_.empty() )
    {
      for( const auto& one : ovalList_ )
        {
          printf( "ovalList_.push_back( { %.2f, %.2f, %.2f, %.2f, %.2f } );\n",
            one.centerx, one.centery, one.radiusx, one.radiusy, one.angle );
        }
    }
}
/** ----------------------------------------------------------------------------
  \fn ovalViewer::clearOvals
---------------------------------------------------------------------------- */
void ovalViewer::clearOvals()
{
  ovalList_.clear();
  msg_.clear();
  update();
}
/** ----------------------------------------------------------------------------
  \fn ovalViewer::paintEvent
---------------------------------------------------------------------------- */
void ovalViewer::paintEvent( QPaintEvent *event )
{
  QPainter paint( this );

  if( not ovalList_.empty() )
    {
      int ww = width() / scale_;
      int hh = height() / scale_;

      QImage img( ww, hh, QImage::Format_ARGB32 );

      img.fill( 0x00ffffff );   // full white

      auto rr = ovalListToRaster( ovalList_, ww, hh );

      for( const auto& one : rr )
        {
          unsigned char *scan = img.scanLine( one.lineY );

          Q_ASSERT( 0 <= one.startX );
          Q_ASSERT( one.startX < ww );
          Q_ASSERT( 0 < one.endX );
          Q_ASSERT( one.endX <= ww );

          int pp = (int)( 255.f * one.value );

          for( int ii = one.startX; ii < one.endX; ii += 1 )
            {
              int pi = 4 * ii;

              scan[ pi ] = 0xFF;
              scan[ pi + 1 ] = 0x00;
              scan[ pi + 2 ] = 0x00;
              scan[ pi + 3 ] = pp;
            }
        }

      paint.drawImage( QRect( 0, 0, ww * scale_, hh * scale_ ), img );

      if( not msg_.isEmpty() )
        {
          paint.drawText( QPoint( 15, 40 ), msg_ );
        }
    }
  else
    {
      paint.drawText( QPoint( 15, 40 ), QString( "Click and drag to add an oval" ) );
    }
}
/** ----------------------------------------------------------------------------
  \fn ovalViewer::mousePressEvent
---------------------------------------------------------------------------- */
void ovalViewer::mousePressEvent( QMouseEvent *event )
{
  if( event->buttons() == Qt::LeftButton )
    {
      if( not cmd_ )
        {
          ovalRecord *oval = nullptr;

          float xx = event->pos().x() / ((float) scale_ );
          float yy = event->pos().y() / ((float) scale_ );

          if( event->modifiers() == Qt::AltModifier )
            {
              renderOnePixel( event->pos() );
            }
          else  // see if we clicked on an oval
            {
              for( auto& one : ovalList_ )
                {
                  if( compute_sdf( & one, xx, yy ) <= 0.f )
                    {
                      oval = & one;
                      break;
                    }
                }

              if( oval )
                {
                  if( event->modifiers() == Qt::NoModifier )
                    cmd_ = new move_oval_cmd( this, oval, event->pos() );
                  else if( event->modifiers() == Qt::ControlModifier )
                    cmd_ = new rotate_oval_cmd( this, oval, event->pos() );
                }
              else    // create a new oval
                {
                  ovalRecord oval;

                  oval.centerx = event->pos().x() / (float) scale_;
                  oval.centery = event->pos().y() / (float) scale_;
                  oval.radiusx = 0.f;
                  oval.radiusy = 0.f;
                  oval.angle = 0;

                  ovalList_.push_back( oval );

                  cmd_ = new new_oval_cmd( this, & ovalList_[ ovalList_.size() - 1 ], event->pos() );
                }
            }
        }
      else
        {
          delete cmd_;
          cmd_ = nullptr;
        }
    }
}
/** ----------------------------------------------------------------------------
  \fn curveViewer::mouseMoveEvent
---------------------------------------------------------------------------- */
void ovalViewer::mouseMoveEvent( QMouseEvent *event )
{
  if( cmd_ )
    {
      cmd_->update( event->pos() );
    }
}
/** ----------------------------------------------------------------------------
  \fn curveViewer::mouseReleaseEvent
---------------------------------------------------------------------------- */
void ovalViewer::mouseReleaseEvent( QMouseEvent *event )
{
  delete cmd_;
  cmd_ = nullptr;
}
/** ----------------------------------------------------------------------------
  \fn move_oval_cmd::move_oval_cmd
---------------------------------------------------------------------------- */
move_oval_cmd::move_oval_cmd( ovalViewer *view, ovalRecord *oval, const QPoint& start ) :
view_( view ), oval_( oval ), start_( start ), centerx_( oval->centerx ), centery_( oval->centery )
{
}
/** ----------------------------------------------------------------------------
  \fn move_oval_cmd::update
---------------------------------------------------------------------------- */
void move_oval_cmd::update( const QPoint& where )
{
  float dx = ( where.x() - start_.x() ) / (float)( view_->scale() );
  float dy = ( where.y() - start_.y() ) / (float) ( view_->scale() );

  oval_->centerx = centerx_ + dx;
  oval_->centery = centery_ + dy;

  view_->update();
}
/** ----------------------------------------------------------------------------
  \fn rotate_oval_cmd::rotate_oval_cmd
---------------------------------------------------------------------------- */
rotate_oval_cmd::rotate_oval_cmd( ovalViewer *view, ovalRecord *oval, const QPoint& where ) :
view_( view ), oval_( oval ), start_( where ), angle_( oval->angle )
{
}
/** ----------------------------------------------------------------------------
  \fn rotate_oval_cmd::update
---------------------------------------------------------------------------- */
void rotate_oval_cmd::update( const QPoint& pos )
{
  int centerx = oval_->centerx * view_->scale();
  int centery = oval_->centery * view_->scale();

  float angle1 = atan2( start_.y() - centery, start_.x() - centerx );
  float angle2 = atan2( pos.y() - centery, pos.x() - centerx );

  oval_->angle = angle_ + angle2 - angle1;

  view_->update();
}
/** ----------------------------------------------------------------------------
  \fn new_oval_cmd::new_oval_cmd
---------------------------------------------------------------------------- */
new_oval_cmd::new_oval_cmd( ovalViewer *view, ovalRecord *oval, const QPoint& pos ) :
view_( view ), oval_( oval ), start_( pos )
{
}
/** ----------------------------------------------------------------------------
  \fn new_oval_cmd::update
---------------------------------------------------------------------------- */
void new_oval_cmd::update( const QPoint& pos )
{
  oval_->centerx = 0.5f * ( pos.x() + start_.x() ) / ((float) view_->scale() );
  oval_->centery = 0.5f * ( pos.y() + start_.y() ) / ((float) view_->scale() );
  oval_->radiusx = 0.5f * fabs( ( pos.x() - start_.x() ) / (float) view_->scale() );
  oval_->radiusy = 0.5f * fabs( ( pos.y() - start_.y() ) / (float) view_->scale() );

  view_->update();
}

