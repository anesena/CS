/*
    Crystal Space Windowing System : grid class
    Copyright (C) 2000 by Norman Kramer <normank@lycosmail.com>
  
    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Library General Public
    License as published by the Free Software Foundation; either
    version 2 of the License, or (at your option) any later version.
  
    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Library General Public License for more details.
  
    You should have received a copy of the GNU Library General Public
    License along with this library; if not, write to the Free
    Software Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
*/

#include "cssysdef.h"
#include "csws/csapp.h"
#include "csws/cswindow.h"
#include "csws/csgrid.h"

/*******************************************************************************************************
 * csRegionTree2D
 *******************************************************************************************************/

/**
 * Tiles this rect into the tree and creates new children if needed.
 */
void csRegionTree2D::Insert (csRect &area, csSome data)
{
  if (children[0]){
    int i=0;
    while (i<5 && children[i]){
      csRect common( area );
      common.Intersect (children[i]->region);
      if (!common.IsEmpty () ) children[i]->Insert (common, data);
      i++;
    }
  }else{
    // leaf
    if (region.Intersects (area)){
      // maybe this regions equals the area, then we simply replace the data and are done
      if (region.Equal (area.xmin, area.ymin, area.xmax, area.ymax)){
	this->data = data;
      }else{
	int i=0;
	// !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
	// DO NOT CHANGE THE SEQUENCE OF THE FOLLOWING, IT ENSURES FindRegion RETURNS AREAS ORDERED LEFT TO RIGHT
	// FOR SINGLE ROW REGIONS (likewise TOP TO DOWN)
	// !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
	// does an upper stripe exist ?
	if (region.ymin < area.ymin){
	  csRect rc( region ); rc.ymax = area.ymin;
	  children[i++] = new csRegionTree2D (rc, this->data);
	}
	// does a left stripe exist ?
	if (region.xmin < area.xmin){
	  csRect rc( region.xmin, area.ymin, area.xmin, area.ymax );
	  children[i++] = new csRegionTree2D (rc, this->data);
	} 
	// the region which fully covers area
	children[i++] = new csRegionTree2D (area, data);
	// does a right stripe exist ?
	if (region.xmax > area.xmax){
	  csRect rc( area.xmax, area.ymin, region.xmax, area.ymax );
	  children[i++] = new csRegionTree2D (rc, this->data);
	}
	// does a lower stripe exist ?
	if (region.ymax > area.ymax){
	  csRect rc( region ); rc.ymin = area.ymax;
	  children[i++] = new csRegionTree2D (rc, this->data);
	}
	// now this leaf became a simple node
      }
    }
  }
}

/**
 * Returns a list of leaves that do all contain parts of area.
 */
void csRegionTree2D::FindRegion (const csRect &area, csVector &vLeafList)
{
  if (children[0]){
    int i=0;
    while (i<5 && children[i]) children[i++]->FindRegion (area, vLeafList);
  }else{
    if (region.Intersects (area)) vLeafList.Push (this);
  }
}

/**
 * Traverse the tree and call user supplied function for every node.
 */
void csRegionTree2D::Traverse (RegionTreeFunc userFunc, csSome databag)
{
  if (userFunc (this, databag)){
    int i=0;
    while (i<5 && children[i]) children[i++]->Traverse (userFunc, databag);
  }
}

/*******************************************************************************************************
 * csSparseGrid::csGridRow
 *******************************************************************************************************/

csSparseGrid::csGridRow::csGridRow (int theCol)
{ 
  col = theCol; 
}

csSparseGrid::csGridRow::~csGridRow () 
{ 
  DeleteAll (); 
}

void csSparseGrid::csGridRow::SetAt (int col, csSome data)
{
  int key = FindSortedKey ((csConstSome)col);
  if (key==-1) key=InsertSorted (new GridRowEntry (col, data));
  else Get (key)->data=data;
}

csSparseGrid::GridRowEntry* csSparseGrid::csGridRow::Get (int index)
{ 
  return (csSparseGrid::GridRowEntry*)csVector::Get (index); 
}

int csSparseGrid::csGridRow::Compare (csSome Item1, csSome Item2, int Mode=0) const{
  (void)Mode;
  csSparseGrid::GridRowEntry *e1 = (csSparseGrid::GridRowEntry*)Item1, *e2 = (csSparseGrid::GridRowEntry*)Item2;
  return (e1->col<e2->col ? -1 : e1->col>e2->col ? 1 : 0);
}

int csSparseGrid::csGridRow::CompareKey (csSome Item1, csConstSome Key, int Mode=0) const{
  (void)Mode;
  csSparseGrid::GridRowEntry *e1 = (csSparseGrid::GridRowEntry*)Item1;
  return (e1->col<(int)Key ? -1 : e1->col>(int)Key ? 1 : 0);
}

bool csSparseGrid::csGridRow::FreeItem (csSome Item)
{ 
  delete (csSparseGrid::GridRowEntry*)Item; 
  return true; 
}


/*******************************************************************************************************
 * csGridCell
 *******************************************************************************************************/

void csGridCell::DrawLine (int x1, int y1, int x2, int y2, csCellBorder& border)
{
  if (border.style == GCBS_LINE)
    Box ( MIN(x1,x2), y1, MAX(x1,x2), y2, CSPAL_GRIDCELL_BORDER_FG);
  else if (border.style != GCBS_NONE){
    int maxX, maxY, i = 0, nSegs, xcompo, ycompo;
    static const int linepattern[][13] = {
      { 2, 4, 0, 2, 0 }, // DASH
      { 4, 4, 0, 2, 0, 2, 0, 2, 0 }, // DASHPOINT
      { 6, 4, 0, 2, 0, 2, 0, 2, 0, 2, 0, 2, 0 }, // DASHPOINTPOINT
      { 6, 4, 0, 2, 0, 4, 0, 2, 0, 2, 0, 2, 0 }  // DASHDASHPOINT
    };
    if (x1<=x2) {xcompo=0; ycompo=1;}
    else {xcompo=1; ycompo=0;}
    maxX = MAX(x1,x2); maxY=MAX(y1,y2);
    x1 = MIN(x1,x2); x2 = MAX(x1,x2);
    nSegs = linepattern[ (int)border.style ][0]; // linesegments in linepattern
    while (x1<maxX && y1<maxY){
      i = (i+1)%nSegs;
      x2 = x1 + linepattern[ (int)border.style ][1+2*i+xcompo];
      y2 = y1 + linepattern[ (int)border.style ][1+2*i+ycompo];
      Box (x1, y1, MIN(x2,maxX), MIN(y2,maxY), (i&1 ? CSPAL_GRIDCELL_BORDER_FG : CSPAL_GRIDCELL_BORDER_BG));
      x1 = x2; y1=y2;
    }
  }
}

void csGridCell::Draw ()
{
  int lx=0, rx=0, ty=0, by=0; // offsets if borders are drawn;

  if (upper.style != GCBS_NONE){
    ty = upper.thick;
    DrawLine (0, 0,  bound.Width () - (right.style == GCBS_NONE ? 0 : right.thick), upper.thick, upper);
  }
  if (right.style != GCBS_NONE){
    rx = right.thick;
    DrawLine (bound.Width (), 0, bound.Width () - right.thick, 
	      bound.Height () - (lower.style == GCBS_NONE ? 0 : lower.thick), right);
  }
  if (lower.style != GCBS_NONE){
    by = lower.thick;
    DrawLine (0 + (left.style == GCBS_NONE ? 0 : left.thick), bound.Height () - lower.thick, 
	      bound.Width (), bound.Height (), lower);
  }
  if (left.style != GCBS_NONE){
    lx = left.thick;
    DrawLine (left.thick, upper.style == GCBS_NONE ? 0 : upper.thick, 0, 
	      bound.Height (), left);
  }
  // fill the canvas with bgcolor
  bound.xmin+=lx;
  bound.ymin+=ty;
  bound.xmax-=rx;
  bound.ymax-=by;
  
  Box (0, 0, bound.Width (), bound.Height (), CSPAL_GRIDCELL_BACKGROUND);
  if (data){
      int tx = (bound.Width () - TextWidth (((csString*)data)->GetData())) /2;
      int ty = (bound.Height () - TextHeight ()) /2;
      Text (tx, ty, CSPAL_GRIDCELL_DATA_FG, CSPAL_GRIDCELL_DATA_BG, ((csString*)data)->GetData() );
  }
  bound.xmin-=lx;
  bound.ymin-=ty;
  bound.xmax+=rx;
  bound.ymax+=by;
}


/*******************************************************************************************************
 * csGridView
 *******************************************************************************************************/


csGridView::csGridView (csGrid *pParent, const csRect& region, int iStyle) : csComponent (pParent)
{
  pGrid = pParent;
  area.Set (region);
  ViewStyle = iStyle;

  if (ViewStyle & CSGVS_HSCROLL)
    hscroll = new csScrollBar (this, cssfsThinRect);
  else
    hscroll = NULL;

  if (ViewStyle & CSGVS_VSCROLL)
    vscroll = new csScrollBar (this, cssfsThinRect);
  else
    vscroll = NULL;
  SetPalette (CSPAL_GRIDVIEW);
  row = col = 0;
}

csGridView::csGridView (const csGridView& view, int iStyle /*=0*/) : csComponent (view.pGrid)
{
  pGrid = view.pGrid;
  area.Set (view.area);
  ViewStyle = (iStyle ? iStyle : view.ViewStyle);

  if (ViewStyle & CSGVS_HSCROLL)
    hscroll = new csScrollBar (this, cssfsThinRect);
  else
    hscroll = NULL;

  if (ViewStyle & CSGVS_VSCROLL)
    vscroll = new csScrollBar (this, cssfsThinRect);
  else
    vscroll = NULL;
  SetPalette (view.palette, view.palettesize);
  row = view.row;
  col = view.col;
}

bool csGridView::SetRect (int xmin, int ymin, int xmax, int ymax)
{
  if (csComponent::SetRect (xmin, ymin, xmax, ymax)){
    if (hscroll)
      hscroll->SetRect (0, bound.Height () - CSSB_DEFAULTSIZE, 
			bound.Width () - (vscroll ? CSSB_DEFAULTSIZE-1 : 0), bound.Height () );
    if (vscroll)
      vscroll->SetRect (bound.Width () - CSSB_DEFAULTSIZE, 0,
			bound.Width (), bound.Height () - (hscroll ? CSSB_DEFAULTSIZE-1 : 0) );
    fPlaceItems = true;
    return true;
  }
  return false;
}

void csGridView::PlaceItems ()
{
  fPlaceItems = false;

  // count the number of cells visible in the first row (exact would be the minimum of cells in a row in the visible area)
  csVector vRegionList;
  csRect rc;
  int i=0, w1=0, w2=0;
  int nRowCells=0, nColCells=0;
  csRegionTree2D *r;

  if (hscroll){
    rc.Set (col, row, area.xmax, row+1);
    pGrid->regions->FindRegion (rc, vRegionList);

    while (i < vRegionList.Length() && w1 < bound.Width ()){
      r = (csRegionTree2D*)vRegionList.Get (i);
      w2 = (r->region.Width() - MAX(col-r->region.xmin,0)) * ((csGridCell*)r->data)->bound.Width (); // #Cells * CellLength
      if (w1+w2 < bound.Width ()){
	nRowCells += (r->region.Width() - MAX(col-r->region.xmin,0));
	w1+=w2;
      }else{
	nRowCells += (bound.Width () - w1)/((csGridCell*)r->data)->bound.Width ();
	w1=bound.Width ();
      }
      i++;
    }

    hsbstatus.value = col - area.xmin;
    hsbstatus.maxvalue = area.Width ();
    hsbstatus.size = 10;
    hsbstatus.maxsize = area.Width ();
    hsbstatus.step = 1;
    hsbstatus.pagestep = MAX( nRowCells, 1);
    hscroll->SendCommand (cscmdScrollBarSet, &hsbstatus);
  }

  if (vscroll){
    // count numbers of cells in first column (exact would be the minimum of cells in a column in the visible area)
    rc.Set (col, row, col+1, area.ymax);
    i=0, w1=0, w2=0;
    vRegionList.SetLength (0);
    
    pGrid->regions->FindRegion (rc, vRegionList);
    
    while (i < vRegionList.Length() && w1 < bound.Height ()){
      r = (csRegionTree2D*)vRegionList.Get (i);
      // #Cells * CellHeight
      w2 = (r->region.Height()-MAX(row-r->region.ymin, 0)) * ((csGridCell*)r->data)->bound.Height (); 
      if (w1+w2 < bound.Height ()){
	nColCells += (r->region.Height()-MAX(row-r->region.ymin, 0));
	w1+=w2;
      }else{
	nColCells += (bound.Height () - w1)/((csGridCell*)r->data)->bound.Height ();
	w1=bound.Height ();
      }
      i++;
    }

    vsbstatus.value = row - area.ymin;
    vsbstatus.maxvalue = area.Height ();
    vsbstatus.size = 10;
    vsbstatus.maxsize = area.Height ();
    vsbstatus.step = 1;
    vsbstatus.pagestep = MAX( nColCells, 1);
    vscroll->SendCommand (cscmdScrollBarSet, &vsbstatus);
  }

}

static bool DrawCellComponents (csComponent *child, void *param){
  (void)param;
  child->Draw ();
  return false;
}

void csGridView::Draw()
{
  if (fPlaceItems)
    PlaceItems ();

  int y=0, x, n;
  int c,actRow=row;
  csRect rc;
  csRegionTree2D *r;
  csVector vRegions;
  csGridCell *cell = NULL;

  while (y < bound.Height ()){
    rc.Set (col, actRow, area.xmax, actRow+1);
    vRegions.SetLength (0);
    pGrid->regions->FindRegion (rc, vRegions);
    if (vRegions.Length () == 0) break; // no more rows to draw
    x=0; n=0; c=col;
    while (x < bound.Width () && n < vRegions.Length ()){
      r = (csRegionTree2D*)vRegions.Get (n++);
      cell = (csGridCell*)r->data;
      Insert (cell); cell->Show (false); // show but don't focus
      for (; c < r->region.xmax && x < bound.Width (); c++){
	cell->SetPos (x, y);
	cell->row = actRow;
	cell->col = c;
	cell->data = pGrid->grid->GetAt (actRow, c);
	cell->Draw ();
	cell->ForEach (DrawCellComponents, NULL, true);
	x += cell->bound.Width ();
      }
      Delete (cell);
    }
    y += cell->bound.Height ();
    actRow++;
  }

  // fill the remainingspace with backcolor
  Box (0, y, bound.Width (), bound.Height (), CSPAL_GRIDVIEW_BACKGROUND);
  csComponent::Draw ();
}

bool csGridView::HandleEvent (csEvent& Event)
{
  switch (Event.Type){
  case csevCommand:
    switch (Event.Command.Code){
    case cscmdScrollBarValueChanged:
      {
	csScrollBar *bar = (csScrollBar*)Event.Command.Info;
	csScrollBarStatus sbs;
	if (!bar || bar->SendCommand (cscmdScrollBarGetStatus, &sbs))
	  return true;
	if (sbs.maxvalue <= 0)
	  return true;
	if (bar == hscroll){
	  hsbstatus = sbs;
	  if (col-area.xmin != sbs.value){
	    col = area.xmin + sbs.value;
	    PlaceItems ();
	    Invalidate (true);
	  }
	}else
	if (bar == vscroll){
	  vsbstatus = sbs;
	  if (row-area.ymin != sbs.value){
	    row = area.ymin + sbs.value;
	    PlaceItems ();
	    Invalidate (true);
	  }
	}
	return true;
      }
      break;
    };
    break;
  }
  return csComponent::HandleEvent (Event);
}

void csGridView::FixSize (int &newW, int &newH)
{
  if (hscroll && newH < hscroll->bound.Height () ) newH = hscroll->bound.Height ();
  if (vscroll && newW < vscroll->bound.Width () ) newW = vscroll->bound.Width ();
}

void csGridView::SuggestSize (int &w, int &h)
{
  w=h=0;
  if (hscroll) { h += CSSB_DEFAULTSIZE; }
  if (vscroll) { w += CSSB_DEFAULTSIZE; }
}

csGridView* csGridView::CreateCopy (int iStyle)
{
  return new csGridView (*this, iStyle);
}

csGridView* csGridView::SplitX (int x, int iStyle /*=0*/)
{
  csGridView *sp=NULL;
  if (x>0 && x<bound.Width ()){
    sp = CreateCopy (iStyle);
    if (sp){
      sp->areafactor = (float)x / (float)bound.Width ();
      pGrid->vViews.Push (sp);
      sp->SetRect (bound.xmin, bound.ymin, bound.xmin + x, bound.ymax);
      SetRect (bound.xmin + x, bound.ymin, bound.xmax, bound.ymax);
      pGrid->viewlayout->Insert (sp->bound, sp);
    }
  }
  return sp;
}

csGridView* csGridView::SplitY (int y, int iStyle /*=0*/)
{
  csGridView *sp=NULL;
  if (y>0 && y<bound.Height ()){
    sp = CreateCopy (iStyle);
    if (sp){
      sp->areafactor = (float)y / (float)bound.Height ();
      pGrid->vViews.Push (sp);
      sp->SetRect (bound.xmin, bound.ymin, bound.xmax, bound.ymin+y);
      SetRect (bound.xmin, bound.ymin+y, bound.xmax, bound.ymax);
      pGrid->viewlayout->Insert (sp->bound, sp);
    }
  }
  return sp;
}

/*******************************************************************************************************
 * csGrid
 *******************************************************************************************************/


csGrid::csGrid (csComponent *pParent, int nRows, int nCols, int iStyle) : csComponent (pParent)
{
  csRect rc (0, 0, nCols, nRows);
  csGridCell *gc = new csGridCell;
  gc->SetRect (0, 0, 50, 30);
  init (pParent, rc, iStyle, gc);
}

csGrid::csGrid (csComponent *pParent, int nRows, int nCols, int iStyle, csGridCell *gridpattern) : csComponent (pParent)
{
  csRect rc (0, 0, nCols, nRows);
  init (pParent, rc, iStyle, gridpattern);
}

void csGrid::init (csComponent *pParent, csRect &rc, int iStyle, csGridCell *gc)
{
  grid = new csSparseGrid;
  SetPalette (CSPAL_GRIDVIEW);
  SetState (CSS_SELECTABLE, true);
  vRegionStyles.Push (gc);
  vViews.Push (new csGridView (this, rc, (iStyle & ~(CSGS_HSPLIT|CSGS_VSPLIT))));
  regions = new csRegionTree2D (rc, vRegionStyles.Get (0) );
  // rc below is a dummy and will be recalculated when SetRect is called
  viewlayout = new csRegionTree2D (rc, vViews.Get (0) );
  sliderX=sliderY=NULL;
  if (iStyle & CSGS_HSPLIT)
    sliderX = new csSlider (this);
  if (iStyle & CSGS_VSPLIT)
    sliderY = new csSlider (this);
  if (pParent)
    pParent->SendCommand (cscmdWindowSetClient, (void*)this);
}

csGrid::~csGrid ()
{
  int i;

  for (i=0; i < grid->rows.Length (); i++){
    csSparseGrid::csGridRow *r = (csSparseGrid::csGridRow*)grid->rows.Get(i)->data;
    for(int j=0; j<r->Length (); j++){
      csString *str = (csString*)r->Get (j)->data;
      if (str) delete str;
    }
    delete r;
  }
  
  delete grid;
  delete regions;

  for (i=0; i<vRegionStyles.Length (); i++) delete (csGridCell*)vRegionStyles.Get (i);
  //  for (i=0; i<vViews.Length (); i++) delete (csGridView*)vViews.Get (i);
}

void csGrid::Draw ()
{
  Box (0, 0, bound.Width (), bound.Height (), CSPAL_GRIDVIEW_BACKGROUND);
  csComponent::Draw (); // views are children, so they are drawn here
}

bool csGrid::HandleEvent (csEvent &Event)
{
  switch (Event.Type){
  case csevCommand:
    switch (Event.Command.Code){
    case cscmdSliderPosSet:
      {
	csSlider *sl = (csSlider*)Event.Command.Info;
	// find the view containing the mouse pointer
	int x,y, x1, y1;
	sl->GetLastMousePos (x, y);
	sl->LocalToGlobal (x, y);
	x1=x; y1=y;
	GlobalToLocal (x,y);
	csRect rc (x, y, x+1, y+1);
	csVector vSpl;
	viewlayout->FindRegion (rc, vSpl);
	if (vSpl.Length () == 1){
	  csGridView *spl = (csGridView*)((csRegionTree2D*)vSpl.Get (0))->data;
	  spl->GlobalToLocal (x1, y1);
	  if (sl == sliderX){
	    spl->SplitX (x1);
	    // place the slider to its  original locations
	  }else{
	    spl->SplitY (y1);
	  }
	  PlaceItems ();
	}else
	  return false;
	return true;
      }
      break;
    }
    break;
  }
  return csComponent::HandleEvent (Event);
}

/**
 * Resize views proportional to the new size of csGrid.
 * @@@ TODO: TAKE MINIMUM SIZE INTO ACCOUNT
 */
static bool ResizeViews (csSome node, csSome /*databag*/)
{
  csRegionTree2D *t = (csRegionTree2D*)node;
  if (t->children[0] == NULL){
    // leaf - we find the new size in the region variable
    ((csGridView*)t->data)->SetRect (t->region.xmin, t->region.ymin, t->region.xmax, t->region.ymax );
    return false;
  }else{
    csGridView *sp1 = (csGridView*)t->children[0]->data;
    int newWidthSp1 = (int)(t->region.Width () * sp1->areafactor);
    int newHeightSp1 = (int)(t->region.Height () * sp1->areafactor);

    if (t->children[0]->region.xmin != t->children[1]->region.xmin){
      // views were divided along the x axis
      t->children[0]->region.Set (t->region.xmin, t->region.ymin, t->region.xmin + newWidthSp1, t->region.ymax);
      t->children[1]->region.Set (t->region.xmin + newWidthSp1, t->region.ymin, t->region.xmax, t->region.ymax);
    }else{
      // views were divided along the y axis
      t->children[0]->region.Set (t->region.xmin, t->region.ymin, t->region.xmax, t->region.ymin + newHeightSp1);
      t->children[1]->region.Set (t->region.xmin, t->region.ymin + newHeightSp1, t->region.xmax, t->region.ymax);
    }
  }
  return true;
}

/**
 * Calculate the minimal area neeed to display all views.
 */
void csGrid::CalcMinimalSize (csRegionTree2D *node, int &w, int &h)
{
  if (node->children[0] == NULL){
    // leaf
    ((csGridView*)node->data)->SuggestSize (w, h);
  }else{
    int w1, w2, h1, h2;
    csGridView *sp1 = (csGridView*)(node->children[0]->data);
    csGridView *sp2 = (csGridView*)(node->children[1]->data);
    CalcMinimalSize (node->children[0], w1, h1);
    CalcMinimalSize (node->children[1], w2, h2);
    if (sp1->bound.xmin != sp2->bound.xmin){
      w = w1 + w2;
      h = MAX (h1, h2);
    }else{
      w = MAX (w1, w2);
      h = h1 + h2;
    }
  }
    
}

bool csGrid::SetRect (int xmin, int ymin, int xmax, int ymax)
{
  if (csComponent::SetRect (xmin, ymin, xmax, ymax)){
    viewlayout->region.Set (0, 0, bound.Width ()- (sliderX?4:0), bound.Height ()-(sliderY?4:0));
    viewlayout->Traverse (ResizeViews);
    PlaceItems ();
    return true;
  }
  return false;
}

void csGrid::FixSize (int &newW, int &newH)
{
  int w, h;
  SuggestSize (w, h);
  if (newW < w) newW = w;
  if (newH < h) newH = h;
}

void csGrid::SuggestSize (int &w, int &h)
{
  CalcMinimalSize (viewlayout, w, h);
  w += (sliderX ? sliderX->bound.Width () : 0);
  h += (sliderY ? sliderY->bound.Height () : 0);
}

void csGrid::PlaceItems ()
{
  if (sliderX)
    sliderX->SetRect (bound.Width()-3, 0, bound.Width (), bound.Height ());
  if (sliderY)
    sliderY->SetRect (0, bound.Height () - 3, bound.Width (), bound.Height ());
}

void csGrid::SetStringAt (int row, int col, const char *data)
{
  csString *str = (csString*)grid->GetAt (row, col);
  if (str || data){
    if (!str){
      str = new csString (data);
      grid->SetAt (row, col, str);
    }else if (!data) {
      delete str;
      grid->SetAt (row, col, NULL);
    }else{
      str->Truncate (0).Append (data);
    }      
  }
}

void csGrid::CreateRegion (csRect& rc, csGridCell *cell) 
{
  regions->Insert (rc, cell); 
  if (!cell->IsUsed ()){
    cell->SetUsed ();
    vRegionStyles.Push (cell);
  }
  Invalidate (true);
}
