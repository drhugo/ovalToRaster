//
// Created by Hugo Ayala on 5/16/24.
//

#ifndef OVALVIEWER_H
#define OVALVIEWER_H

#include <vector>

#include <QWidget>

#include "ovalRasterizer.h"

/** ----------------------------------------------------------------------------
  \class mouse_cmd
---------------------------------------------------------------------------- */
class mouse_cmd
  {
    public:
    virtual ~mouse_cmd() = default;

    virtual void update( const QPoint& pos ) = 0;
  };
/** ----------------------------------------------------------------------------
  \class ovalViewer
---------------------------------------------------------------------------- */
class ovalViewer : public QWidget
  {
      Q_OBJECT

    public:
      ovalViewer( QWidget *parent = nullptr );

      int scale() const { return scale_; }

    protected:
      void paintEvent( QPaintEvent *event ) override;

      void mousePressEvent( QMouseEvent *event ) override;
      void mouseMoveEvent( QMouseEvent *event ) override;
      void mouseReleaseEvent( QMouseEvent *event ) override;

      std::vector<ovalRecord> ovalList_;
      mouse_cmd *cmd_;

      int scale_;
  };
/** ----------------------------------------------------------------------------
  \class move_oval_cmd
---------------------------------------------------------------------------- */
class move_oval_cmd : public mouse_cmd
  {
  public:
    move_oval_cmd( ovalViewer *view, ovalRecord *oval, const QPoint& where );

    void update( const QPoint& pos ) override;

  private:
    ovalViewer *view_;
    ovalRecord *oval_;
    QPoint start_;

    float centerx_;
    float centery_;
  };

/** ----------------------------------------------------------------------------
  \class rotate_oval_cmd
---------------------------------------------------------------------------- */
class rotate_oval_cmd : public mouse_cmd
  {
  public:
    rotate_oval_cmd( ovalViewer *view, ovalRecord *oval, const QPoint& where );

    void update( const QPoint& pos ) override;

  private:
    ovalViewer *view_;
    ovalRecord *oval_;
    QPoint start_;

    float angle_;
  };

/** ----------------------------------------------------------------------------
  \class new_oval_cmd
---------------------------------------------------------------------------- */
class new_oval_cmd : public mouse_cmd
  {
  public:
    new_oval_cmd( ovalViewer *view, ovalRecord *oval, const QPoint& where );

    void update( const QPoint& pos ) override;

  private:
    ovalViewer *view_;
    ovalRecord *oval_;

    QPoint start_;
  };

#endif //OVALVIEWER_H
