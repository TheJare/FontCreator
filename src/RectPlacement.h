/*
Copyright (C) 2006 by Javier Arévalo (http://www.iguanademos.com/Jare)

This software is provided 'as-is', without any express or implied warranty.
In no event will the authors be held liable for any damages arising from the use of this software.

Permission is granted to anyone to use this software for any purpose, including commercial applications,
and to alter it and redistribute it freely, subject to the following restrictions:

 1 - The origin of this software must not be misrepresented. You must not claim that you wrote
     the original software. If you use this software in a product, an acknowledgment in the product
     documentation would be appreciated but is not required.
 2 - Altered source versions must be plainly marked as such, and must not be misrepresented as being
     the original software.
 3 - This notice may not be removed or altered from any source distribution.
*/

#ifndef _RECT_PLACEMENT_H_
#define _RECT_PLACEMENT_H_

#include <vector>

// --------------------------------------------------------------------------------
// --------------------------------------------------------------------------------

class CRectPlacement
{
  public:
    struct TPos
    {
      int x, y;

      TPos() { }
      TPos(int _x, int _y): x(_x), y(_y) { }

      bool operator ==(const TPos &p) const { return x == p.x && y == p.y; }
    };

    struct TRect: public TPos
    {
      int w, h;

      TRect() { }
      TRect(int _x, int _y, int _w, int _h): TPos(_x, _y), w(_w > 0? _w : 0), h(_h > 0? _h : 0) { }

      bool Contains   (const TPos &p)  const { return (p.x >= x && p.y >= y && p.x < (x+w) && p.y < (y+h)); }
      bool Contains   (const TRect &r) const { return (r.x >= x && r.y >= y && (r.x+r.w) <= (x+w) && (r.y+r.h) <= (y+h)); }
      bool Intersects (const TRect &r) const { return w > 0 && h > 0 && r.w > 0 && r.h > 0 && ((r.x+r.w) > x && r.x < (x+w) && (r.y+r.h) > y && r.y < (y+h)); }

    //  static bool Greater(const TRect &a, const TRect &b) { return a.w*a.h > b.w*b.h; }     // Greater rect area
      static bool Greater(const TRect &a, const TRect &b) { return (a.w > b.w && a.w > b.h) || (a.h > b.w && a.h > b.h); }  // Greater size in at least one dim, or greater area.
    };

    typedef std::vector<TPos>  CPosArray;
    typedef std::vector<TRect> CRectArray;

    CRectPlacement(): m_size(0, 0, 0, 0)              { Init(); }
    CRectPlacement(int w, int h): m_size(0, 0, 0, 0)  { Init(w, h); }
    ~CRectPlacement()                                 { End(); }

    void      Init    (int w = 1, int h = 1);
    void      End     ();
    bool      IsOk    ()                      const { return m_size.w > 0; }

    int       GetW    ()                      const { return m_size.w; }
    int       GetH    ()                      const { return m_size.h; }

    bool IsFree     (const TPos &p) const;
    bool IsFree     (const TRect &r) const;

    void AddPosition            (const TPos &p);
    void AddRect                (const TRect &r);
    bool AddAtEmptySpot         (TRect &r);
    bool AddAtEmptySpotAutoGrow (TRect *pRect, int maxW, int maxH);

  private:
    TRect       m_size;
    CRectArray  m_vRects;
    CPosArray   m_vPositions;
};
  

#endif //_RECT_PLACEMENT_H_
