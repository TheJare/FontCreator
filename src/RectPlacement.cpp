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

#include "RectPlacement.h"

// ---------------------------------
// ---------------------------------
void CRectPlacement::Init    (int w, int h)
{
  End();
  m_size = TRect(0, 0, w, h);
  m_vPositions.push_back(TPos(0,0));
}

// ---------------------------------
void CRectPlacement::End     ()
{
  m_vPositions.clear();
  m_vRects.clear();
  m_size.w = 0;
}

// ---------------------------------
bool CRectPlacement::IsFree (const TPos &p) const
{
  if (!m_size.Contains(p))
    return false;
  for (CRectArray::const_iterator it = m_vRects.begin();
       it != m_vRects.end();
       ++it)
    if (it->Contains(p))
      return false;
  return true;
}

// ---------------------------------
bool CRectPlacement::IsFree (const TRect &r) const
{
  if (!m_size.Contains(r))
    return false;
  for (CRectArray::const_iterator it = m_vRects.begin();
       it != m_vRects.end();
       ++it)
    if (it->Intersects(r))
      return false;
  return true;
}

// ---------------------------------
void CRectPlacement::AddPosition    (const TPos &p)
{
  bool bFound = false;
  CPosArray::iterator it;
  for (it = m_vPositions.begin();
       !bFound && it != m_vPositions.end();
       ++it)
  {
    if (p.x+p.y < it->x+it->y)
      bFound = true;
  }
  if (bFound)
    m_vPositions.insert(it, p);
  else
    m_vPositions.push_back(p);
}

// ---------------------------------
void CRectPlacement::AddRect  (const TRect &r)
{
  m_vRects.push_back(r);

  AddPosition(TPos(r.x, r.y+r.h));
  AddPosition(TPos(r.x+r.w, r.y));
}

// ---------------------------------
bool CRectPlacement::AddAtEmptySpot   (TRect &r)
{
  bool bFound = false;
  CPosArray::iterator it;
  for (it = m_vPositions.begin();
       !bFound && it != m_vPositions.end();
       ++it)
  {
    TRect Rect(it->x, it->y, r.w, r.h);

    if (IsFree(Rect))
    {
      r = Rect;
      bFound = true;
      break;
    }
  }
  if (bFound)
  {
    m_vPositions.erase(it);
    int x = 1;
    int y = 1;
    for (; x <= r.x; x++)
      if (!IsFree(TRect(r.x - x, r.y, r.w, r.h)))
        break;
    for (; y <= r.y; y++)
      if (!IsFree(TRect(r.x, r.y - y, r.w, r.h)))
        break;
    if (y > x)
      r.y -= y-1;
    else
      r.x -= x-1;
    AddRect(r);
  }
  return bFound;
}

// ---------------------------------
bool CRectPlacement::AddAtEmptySpotAutoGrow   (TRect *pRect, int maxW, int maxH)
{
  if (pRect->w <= 0)
    return true;

  int orgW = m_size.w;
  int orgH = m_size.h;

  // Try to add it in the existing space
  while (!AddAtEmptySpot(*pRect))
  {
    int pw = m_size.w;
    int ph = m_size.h;

    // Sanity check - if area is complete.
    if (pw >= maxW && ph >= maxH)
    {
      m_size.w = orgW;
      m_size.h = orgH;
      return false;
    }

    // Try growing the smallest dim
    if (pw < maxW && (pw < ph || ((pw == ph) && (pRect->w >= pRect->h))))
      m_size.w = pw*2;
    else
      m_size.h = ph*2;
    if (AddAtEmptySpot(*pRect))
      break;

    // Try growing the other dim instead
    if (pw != m_size.w)
    {
      m_size.w = pw;
      if (ph < maxW)
        m_size.h = ph*2;
    }
    else
    {
      m_size.h = ph;
      if (pw < maxW)
        m_size.w = pw*2;
    }

    if (pw != m_size.w || ph != m_size.h)
      if (AddAtEmptySpot(*pRect))
        break;

    // Grow both if possible, and reloop.
    m_size.w = pw;
    m_size.h = ph;
    if (pw < maxW)
      m_size.w = pw*2;
    if (ph < maxH)
      m_size.h = ph*2;
  }
  return true;
}
